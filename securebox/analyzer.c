/* ============================================================
 * analyzer.c - Module d'analyse de sécurité de SecureBox
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include "analyzer.h"
#include "logger.h"
#include "stats.h"
#include "utils.h"
#include "config.h"

static EventQueue    g_queue;
static volatile int  g_running = 1;

#define HISTORY_SIZE 64

static time_t g_delete_times[HISTORY_SIZE];
static int    g_delete_idx        = 0;
static time_t g_delete_alert_time = 0;

typedef struct { char path[1024]; time_t ts; } ModRecord;
static ModRecord g_mod_records[HISTORY_SIZE];
static int       g_mod_idx = 0;

#define MODIFY_ALERT_TRACK 16
typedef struct { char path[1024]; time_t alert_ts; } ModAlertRecord;
static ModAlertRecord g_mod_alerts[MODIFY_ALERT_TRACK];

const char *event_type_to_str(EventType type) {
    switch (type) {
        case EVT_CREATE_FILE: return "CREATE_FILE";
        case EVT_CREATE_DIR:  return "CREATE_DIR ";
        case EVT_DELETE_FILE: return "DELETE_FILE";
        case EVT_DELETE_DIR:  return "DELETE_DIR ";
        case EVT_MODIFY_FILE: return "MODIFY_FILE";
        case EVT_RENAME_FILE: return "RENAME_FILE";
        default:              return "UNKNOWN    ";
    }
}

int analyzer_init(void) {
    memset(&g_queue, 0, sizeof(g_queue));
    if (pthread_mutex_init(&g_queue.mutex, NULL) != 0)    return -1;
    if (pthread_cond_init(&g_queue.not_empty, NULL) != 0) return -1;
    if (pthread_cond_init(&g_queue.not_full,  NULL) != 0) return -1;
    memset(g_delete_times, 0, sizeof(g_delete_times));
    memset(g_mod_records,  0, sizeof(g_mod_records));
    memset(g_mod_alerts,   0, sizeof(g_mod_alerts));
    g_delete_alert_time = 0;
    g_delete_idx = 0;
    g_mod_idx    = 0;
    g_running = 1;
    return 0;
}

void analyzer_push(FsEvent *evt) {
    pthread_mutex_lock(&g_queue.mutex);
    while (g_queue.count >= QUEUE_SIZE)
        pthread_cond_wait(&g_queue.not_full, &g_queue.mutex);
    g_queue.events[g_queue.tail] = *evt;
    g_queue.tail  = (g_queue.tail + 1) % QUEUE_SIZE;
    g_queue.count++;
    pthread_cond_signal(&g_queue.not_empty);
    pthread_mutex_unlock(&g_queue.mutex);
}

static int check_delete_burst(time_t now) {
    g_delete_times[g_delete_idx % HISTORY_SIZE] = now;
    g_delete_idx++;

    int count = 0;
    for (int i = 0; i < HISTORY_SIZE; i++) {
        if (g_delete_times[i] != 0 &&
            (now - g_delete_times[i]) <= ALERT_DELETE_WINDOW)
            count++;
    }
    if (count <= ALERT_DELETE_COUNT) return 0;

    if (g_delete_alert_time != 0 &&
        (now - g_delete_alert_time) < ALERT_DELETE_WINDOW)
        return 0;

    /* Reset historique après alerte */
    memset(g_delete_times, 0, sizeof(g_delete_times));
    g_delete_idx        = 0;
    g_delete_alert_time = now;
    return 1;
}

static void reset_mod_history(const char *path) {
    for (int j = 0; j < HISTORY_SIZE; j++) {
        if (strcmp(g_mod_records[j].path, path) == 0)
            g_mod_records[j].ts = 0;
    }
}

static int check_modify_burst(const char *path, time_t now) {
    g_mod_records[g_mod_idx % HISTORY_SIZE].ts = now;
    strncpy(g_mod_records[g_mod_idx % HISTORY_SIZE].path, path,
            sizeof(g_mod_records[0].path) - 1);
    g_mod_idx++;

    int count = 0;
    for (int i = 0; i < HISTORY_SIZE; i++) {
        if (g_mod_records[i].ts != 0 &&
            strcmp(g_mod_records[i].path, path) == 0 &&
            (now - g_mod_records[i].ts) <= ALERT_MODIFY_WINDOW)
            count++;
    }
    if (count <= ALERT_MODIFY_COUNT) return 0;

    for (int i = 0; i < MODIFY_ALERT_TRACK; i++) {
        if (strcmp(g_mod_alerts[i].path, path) == 0) {
            if ((now - g_mod_alerts[i].alert_ts) < ALERT_MODIFY_WINDOW)
                return 0;
            g_mod_alerts[i].alert_ts = now;
            reset_mod_history(path);
            return 1;
        }
    }
    for (int i = 0; i < MODIFY_ALERT_TRACK; i++) {
        if (g_mod_alerts[i].path[0] == '\0') {
            strncpy(g_mod_alerts[i].path, path, sizeof(g_mod_alerts[i].path) - 1);
            g_mod_alerts[i].alert_ts = now;
            reset_mod_history(path);
            return 1;
        }
    }
    strncpy(g_mod_alerts[0].path, path, sizeof(g_mod_alerts[0].path) - 1);
    g_mod_alerts[0].alert_ts = now;
    reset_mod_history(path);
    return 1;
}

static void analyze_event(FsEvent *evt) {
    const char *type_str = event_type_to_str(evt->type);
    const char *filename = get_filename(evt->path);
    char alert_msg[1200];

    /* Statistiques */
    stats_inc_total();
    switch (evt->type) {
        case EVT_CREATE_FILE:
        case EVT_CREATE_DIR:  stats_inc_creations();     break;
        case EVT_DELETE_FILE:
        case EVT_DELETE_DIR:  stats_inc_suppressions();  break;
        case EVT_MODIFY_FILE: stats_inc_modifications(); break;
        case EVT_RENAME_FILE: stats_inc_renommages();    break;
        default: break;
    }

    /* Cas spécial chmod+x via IN_ATTRIB */
    if (evt->type == EVT_MODIFY_FILE &&
        strcmp(evt->path_dest, "ATTRIB_EXEC") == 0) {
        logger_push(LEVEL_ALERT, "EXEC_CHMOD", evt->path);
        stats_inc_alertes();
        return;
    }

    /* Journalisation de l'événement */
    if (evt->type == EVT_RENAME_FILE && evt->path_dest[0] != '\0') {
        char rename_msg[2200];
        snprintf(rename_msg, sizeof(rename_msg), "%s -> %s",
                 evt->path, evt->path_dest);
        logger_push(LEVEL_INFO, type_str, rename_msg);
    } else {
        logger_push(LEVEL_INFO, type_str, evt->path);
    }

    /* Règle 1 : fichier caché */
    if ((evt->type == EVT_CREATE_FILE || evt->type == EVT_RENAME_FILE)
         && is_hidden_file(filename)) {
        logger_push(LEVEL_ALERT, "HIDDEN_FILE", evt->path);
        stats_inc_alertes();
    }

    /* Règle 2 : extension interdite */
    if ((evt->type == EVT_CREATE_FILE || evt->type == EVT_RENAME_FILE)
         && is_forbidden_extension(filename)) {
        logger_push(LEVEL_ALERT, "FORBIDDEN_EXT", evt->path);
        stats_inc_alertes();
    }

    /* Règle 3 : exécutable créé */
    if (evt->type == EVT_CREATE_FILE && is_executable(evt->path)) {
        logger_push(LEVEL_ALERT, "EXEC_CREATED", evt->path);
        stats_inc_alertes();
    }

    /* Règle 4 : rafale de suppressions */
    if (evt->type == EVT_DELETE_FILE || evt->type == EVT_DELETE_DIR) {
        if (check_delete_burst(evt->timestamp)) {
            snprintf(alert_msg, sizeof(alert_msg),
                     "Rafale de suppressions detectee (>%d en %ds)",
                     ALERT_DELETE_COUNT, ALERT_DELETE_WINDOW);
            logger_push_raw(LEVEL_ALERT, alert_msg);
            stats_inc_alertes();
        }
    }

    /* Règle 5 : modifications rapides */
    if (evt->type == EVT_MODIFY_FILE) {
        if (check_modify_burst(evt->path, evt->timestamp)) {
            snprintf(alert_msg, sizeof(alert_msg),
                     "Modifications rapides sur : %s (>%d en %ds)",
                     evt->path, ALERT_MODIFY_COUNT, ALERT_MODIFY_WINDOW);
            logger_push_raw(LEVEL_ALERT, alert_msg);
            stats_inc_alertes();
        }
    }
}

void *analyzer_thread(void *arg) {
    (void)arg;
    FsEvent evt;
    while (1) {
        pthread_mutex_lock(&g_queue.mutex);
        while (g_queue.count == 0 && g_running)
            pthread_cond_wait(&g_queue.not_empty, &g_queue.mutex);
        if (!g_running && g_queue.count == 0) {
            pthread_mutex_unlock(&g_queue.mutex);
            break;
        }
        evt = g_queue.events[g_queue.head];
        g_queue.head  = (g_queue.head + 1) % QUEUE_SIZE;
        g_queue.count--;
        pthread_cond_signal(&g_queue.not_full);
        pthread_mutex_unlock(&g_queue.mutex);
        analyze_event(&evt);
    }
    return NULL;
}

void analyzer_stop(void) {
    pthread_mutex_lock(&g_queue.mutex);
    g_running = 0;
    pthread_cond_broadcast(&g_queue.not_empty);
    pthread_mutex_unlock(&g_queue.mutex);
}

void analyzer_cleanup(void) {
    pthread_mutex_destroy(&g_queue.mutex);
    pthread_cond_destroy(&g_queue.not_empty);
    pthread_cond_destroy(&g_queue.not_full);
}
