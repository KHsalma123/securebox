/* ============================================================
 * logger.c - Module de journalisation de SecureBox
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "logger.h"
#include "config.h"
#include "utils.h"

static FILE       *g_log_file = NULL;
static LogQueue    g_queue;
static volatile int g_running = 1;

/* ============================================================
 * logger_get_file : retourne le pointeur fichier log.
 *   Utilisé par stats.c pour écrire directement sans queue.
 * ============================================================ */
FILE *logger_get_file(void) {
    return g_log_file;
}

int logger_init(const char *log_path) {
    if (ensure_log_dir(LOG_DIR) != 0)
        return -1;

    g_log_file = fopen(log_path, "a");
    if (g_log_file == NULL) {
        fprintf(stderr, "[ERREUR] Impossible d'ouvrir '%s' : %s\n",
                log_path, strerror(errno));
        return -1;
    }

    setvbuf(g_log_file, NULL, _IONBF, 0);

    memset(&g_queue, 0, sizeof(g_queue));

    if (pthread_mutex_init(&g_queue.mutex, NULL) != 0) {
        fclose(g_log_file); return -1;
    }
    if (pthread_cond_init(&g_queue.not_empty, NULL) != 0) {
        pthread_mutex_destroy(&g_queue.mutex);
        fclose(g_log_file); return -1;
    }
    if (pthread_cond_init(&g_queue.not_full, NULL) != 0) {
        pthread_cond_destroy(&g_queue.not_empty);
        pthread_mutex_destroy(&g_queue.mutex);
        fclose(g_log_file); return -1;
    }

    g_running = 1;
    return 0;
}

void logger_push(const char *level, const char *event_type, const char *resource) {
    char ts[32], msg[512];
    get_timestamp(ts, sizeof(ts));
    snprintf(msg, sizeof(msg), "%s [%s] %s : %s", ts, level, event_type, resource);

    pthread_mutex_lock(&g_queue.mutex);
    while (g_queue.count >= QUEUE_SIZE)
        pthread_cond_wait(&g_queue.not_full, &g_queue.mutex);
    strncpy(g_queue.entries[g_queue.tail].message, msg,
            sizeof(g_queue.entries[0].message) - 1);
    g_queue.tail  = (g_queue.tail + 1) % QUEUE_SIZE;
    g_queue.count++;
    pthread_cond_signal(&g_queue.not_empty);
    pthread_mutex_unlock(&g_queue.mutex);
}

void logger_push_raw(const char *level, const char *message) {
    char ts[32], msg[512];
    get_timestamp(ts, sizeof(ts));
    snprintf(msg, sizeof(msg), "%s [%s] %s", ts, level, message);

    pthread_mutex_lock(&g_queue.mutex);
    while (g_queue.count >= QUEUE_SIZE)
        pthread_cond_wait(&g_queue.not_full, &g_queue.mutex);
    strncpy(g_queue.entries[g_queue.tail].message, msg,
            sizeof(g_queue.entries[0].message) - 1);
    g_queue.tail  = (g_queue.tail + 1) % QUEUE_SIZE;
    g_queue.count++;
    pthread_cond_signal(&g_queue.not_empty);
    pthread_mutex_unlock(&g_queue.mutex);
}

/* ============================================================
 * logger_thread : écrit dans le fichier ET sur la console.
 *   Le printf ici affiche les événements en temps réel.
 *   Les stats N'utilisent PAS cette queue → pas de doublon.
 * ============================================================ */
void *logger_thread(void *arg) {
    (void)arg;
    LogEntry entry;

    while (1) {
        pthread_mutex_lock(&g_queue.mutex);
        while (g_queue.count == 0 && g_running)
            pthread_cond_wait(&g_queue.not_empty, &g_queue.mutex);
        if (!g_running && g_queue.count == 0) {
            pthread_mutex_unlock(&g_queue.mutex);
            break;
        }
        entry = g_queue.entries[g_queue.head];
        g_queue.head  = (g_queue.head + 1) % QUEUE_SIZE;
        g_queue.count--;
        pthread_cond_signal(&g_queue.not_full);
        pthread_mutex_unlock(&g_queue.mutex);

        /* Fichier log */
        if (g_log_file != NULL)
            fprintf(g_log_file, "%s\n", entry.message);

        /* Console — affiche événements en temps réel */
        printf("%s\n", entry.message);
        fflush(stdout);
    }
    return NULL;
}

void logger_stop(void) {
    pthread_mutex_lock(&g_queue.mutex);
    g_running = 0;
    pthread_cond_broadcast(&g_queue.not_empty);
    pthread_mutex_unlock(&g_queue.mutex);
}

void logger_cleanup(void) {
    if (g_log_file != NULL) {
        fflush(g_log_file);
        fclose(g_log_file);
        g_log_file = NULL;
    }
    pthread_mutex_destroy(&g_queue.mutex);
    pthread_cond_destroy(&g_queue.not_empty);
    pthread_cond_destroy(&g_queue.not_full);
}
