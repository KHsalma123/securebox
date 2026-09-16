/* ============================================================
 * stats.c - Module de statistiques de SecureBox
 * ============================================================ */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "stats.h"
#include "config.h"

static Stats  g_stats;
static FILE  *g_log_fp = NULL;

/* ============================================================
 * stats_set_logfile : appelé depuis main.c après logger_init()
 * ============================================================ */
void stats_set_logfile(FILE *fp) {
    g_log_fp = fp;
}

void stats_init(void) {
    memset(&g_stats, 0, sizeof(g_stats));
    g_stats.start_time = time(NULL);
    pthread_mutex_init(&g_stats.mutex, NULL);
}

void stats_inc_total(void) {
    pthread_mutex_lock(&g_stats.mutex);
    g_stats.total++;
    pthread_mutex_unlock(&g_stats.mutex);
}

void stats_inc_creations(void) {
    pthread_mutex_lock(&g_stats.mutex);
    g_stats.creations++;
    pthread_mutex_unlock(&g_stats.mutex);
}

void stats_inc_suppressions(void) {
    pthread_mutex_lock(&g_stats.mutex);
    g_stats.suppressions++;
    pthread_mutex_unlock(&g_stats.mutex);
}

void stats_inc_modifications(void) {
    pthread_mutex_lock(&g_stats.mutex);
    g_stats.modifications++;
    pthread_mutex_unlock(&g_stats.mutex);
}

void stats_inc_renommages(void) {
    pthread_mutex_lock(&g_stats.mutex);
    g_stats.renommages++;
    pthread_mutex_unlock(&g_stats.mutex);
}

void stats_inc_alertes(void) {
    pthread_mutex_lock(&g_stats.mutex);
    g_stats.alertes++;
    pthread_mutex_unlock(&g_stats.mutex);
}

/* ============================================================
 * do_stats : affiche sur console ET écrit dans le fichier.
 *   Appelée avec le mutex déjà verrouillé.
 * ============================================================ */
static void do_stats(void) {
    time_t now     = time(NULL);
    long   elapsed = (long)(now - g_stats.start_time);
    long   hours   = elapsed / 3600;
    long   minutes = (elapsed % 3600) / 60;
    long   seconds = elapsed % 60;

    /* Timestamp au même format que le logger */
    char ts[32];
    struct tm *tm_info = localtime(&now);
    strftime(ts, sizeof(ts), "[%Y-%m-%d %H:%M:%S]", tm_info);

    /* ---- 1. Console (1 seule fois) ---- */
    printf("\n========================================\n");
    printf("  STATISTIQUES SECUREBOX\n");
    printf("========================================\n");
    printf("  Durée de fonctionnement : %ldh %ldm %lds\n", hours, minutes, seconds);
    printf("  Total événements        : %ld\n", g_stats.total);
    printf("  Créations               : %ld\n", g_stats.creations);
    printf("  Suppressions            : %ld\n", g_stats.suppressions);
    printf("  Modifications           : %ld\n", g_stats.modifications);
    printf("  Renommages              : %ld\n", g_stats.renommages);
    printf("  Alertes générées        : %ld\n", g_stats.alertes);
    printf("========================================\n\n");
    fflush(stdout);

    /* ---- 2. Fichier log (directement, sans passer par la queue) ---- */
    if (g_log_fp != NULL) {
        fprintf(g_log_fp, "%s [STATS ] ========================================\n", ts);
        fprintf(g_log_fp, "%s [STATS ] Durée de fonctionnement : %ldh %ldm %lds\n",
                ts, hours, minutes, seconds);
        fprintf(g_log_fp, "%s [STATS ] Total événements        : %ld\n", ts, g_stats.total);
        fprintf(g_log_fp, "%s [STATS ] Créations               : %ld\n", ts, g_stats.creations);
        fprintf(g_log_fp, "%s [STATS ] Suppressions            : %ld\n", ts, g_stats.suppressions);
        fprintf(g_log_fp, "%s [STATS ] Modifications           : %ld\n", ts, g_stats.modifications);
        fprintf(g_log_fp, "%s [STATS ] Renommages              : %ld\n", ts, g_stats.renommages);
        fprintf(g_log_fp, "%s [STATS ] Alertes générées        : %ld\n", ts, g_stats.alertes);
        fprintf(g_log_fp, "%s [STATS ] ========================================\n", ts);
        fflush(g_log_fp);
    }
}

/* ============================================================
 * stats_display : console + log — appelé par SIGUSR1 et à l'arrêt
 * ============================================================ */
void stats_display(void) {
    pthread_mutex_lock(&g_stats.mutex);
    do_stats();
    pthread_mutex_unlock(&g_stats.mutex);
}

void stats_cleanup(void) {
    pthread_mutex_destroy(&g_stats.mutex);
}
