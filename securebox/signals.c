/* ============================================================
 * signals.c - Gestion des signaux système de SecureBox
 *
 *  SIGINT  (Ctrl+C)  → arrêt propre
 *  SIGTERM           → arrêt propre
 *  SIGUSR1           → affichage des statistiques sans arrêt
 * ============================================================ */

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include "signals.h"
#include "stats.h"
#include "logger.h"
#include "config.h"

/* Flag global d'arrêt — volatile pour être vu de tous les threads */
static volatile sig_atomic_t g_stop_flag = 0;

/* ============================================================
 * handler_stop : gestionnaire pour SIGINT et SIGTERM.
 * ============================================================ */
static void handler_stop(int signum) {
    (void)signum;
    g_stop_flag = 1;

    /* Écriture async-signal-safe */
    const char *msg = "\n[SecureBox] Signal d'arrêt reçu. Fermeture en cours...\n";
    /* write() est async-signal-safe, printf ne l'est pas */
    write(STDOUT_FILENO, msg, strlen(msg));
}

/* ============================================================
 * handler_sigusr1 : affiche les statistiques sans arrêter.
 * ============================================================ */
static void handler_sigusr1(int signum) {
    (void)signum;
    /* stats_display() n'est pas strictement async-signal-safe
     * (utilise printf/mutex), mais acceptable en pratique pour
     * ce projet pédagogique. */
    stats_display();
}

/* ============================================================
 * signals_init : installe les gestionnaires via sigaction().
 * ============================================================ */
int signals_init(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));

    /* SIGINT + SIGTERM → arrêt */
    sa.sa_handler = handler_stop;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; /* relancer les appels système interrompus */

    if (sigaction(SIGINT, &sa, NULL) != 0) {
        perror("sigaction SIGINT");
        return -1;
    }
    if (sigaction(SIGTERM, &sa, NULL) != 0) {
        perror("sigaction SIGTERM");
        return -1;
    }

    /* SIGUSR1 → statistiques */
    sa.sa_handler = handler_sigusr1;
    if (sigaction(SIGUSR1, &sa, NULL) != 0) {
        perror("sigaction SIGUSR1");
        return -1;
    }

    /* Ignorer SIGPIPE (écriture sur pipe fermé) */
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, NULL);

    return 0;
}

/* ============================================================
 * signals_stop_requested : renvoie 1 si SIGINT/SIGTERM reçu.
 * ============================================================ */
int signals_stop_requested(void) {
    return (int)g_stop_flag;
}
