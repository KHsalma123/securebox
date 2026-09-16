/* ============================================================
 * main.c - Point d'entrée de SecureBox
* ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "config.h"
#include "utils.h"
#include "logger.h"
#include "analyzer.h"
#include "monitor.h"
#include "stats.h"
#include "signals.h"

static void print_banner(const char *path) {
    printf("\033[1;36m");
    printf("╔══════════════════════════════════════════════════╗\n");
    printf("║           SecureBox — Surveillance Linux         ║\n");
    printf("║         ENSA — Programmation Système  GI S8      ║\n");
    printf("╚══════════════════════════════════════════════════╝\n");
    printf("\033[0m");
    printf("  Répertoire surveillé : \033[1;33m%s\033[0m\n", path);
    printf("  Fichier log          : \033[1;33m%s\033[0m\n", LOG_FILE);
    printf("  Arrêt propre         : Ctrl+C  ou  kill -SIGTERM <pid>\n");
    printf("  Statistiques         : kill -SIGUSR1 <pid>\n");
    printf("  PID                  : \033[1;33m%d\033[0m\n\n", getpid());
}

int main(int argc, char *argv[]) {

    /* ---- 1. Vérification des arguments ---- */
    if (argc != 2) {
        fprintf(stderr,
            "Usage : %s <chemin_répertoire>\n"
            "Exemple : %s /home/user/documents\n",
            argv[0], argv[0]);
        return EXIT_ERR;
    }

    const char *watch_path = argv[1];

    /* ---- 2. Vérification du répertoire cible ---- */
    if (check_directory(watch_path) != 0)
        return EXIT_ERR;

    /* ---- 3. Installation des gestionnaires de signaux ---- */
    if (signals_init() != 0) {
        fprintf(stderr, "[ERREUR] Impossible d'installer les gestionnaires de signaux.\n");
        return EXIT_ERR;
    }

    /* ---- 4. Initialisation du logger ---- */
    if (logger_init(LOG_FILE) != 0) {
        fprintf(stderr, "[ERREUR] Impossible d'initialiser le logger.\n");
        return EXIT_ERR;
    }

    /* ---- 4b. Donner le fichier log à stats.c ---- */
    /* stats_display() écrira directement dans ce fichier,
     * sans passer par la queue du logger → pas de doublon console */
    stats_set_logfile(logger_get_file());

    /* ---- 5. Initialisation des statistiques ---- */
    stats_init();

    /* ---- 6. Initialisation du module d'analyse ---- */
    if (analyzer_init() != 0) {
        fprintf(stderr, "[ERREUR] Impossible d'initialiser l'analyseur.\n");
        logger_cleanup();
        return EXIT_ERR;
    }

    /* ---- 7. Initialisation du module de surveillance ---- */
    if (monitor_init(watch_path) != 0) {
        fprintf(stderr, "[ERREUR] Impossible d'initialiser la surveillance.\n");
        analyzer_cleanup();
        logger_cleanup();
        return EXIT_ERR;
    }

    /* ---- 8. Affichage de la bannière ---- */
    print_banner(watch_path);

    /* ---- 9. Création des threads ---- */
    pthread_t tid_logger, tid_analyzer, tid_monitor;

    if (pthread_create(&tid_logger, NULL, logger_thread, NULL) != 0) {
        perror("pthread_create logger");
        goto cleanup;
    }
    if (pthread_create(&tid_analyzer, NULL, analyzer_thread, NULL) != 0) {
        perror("pthread_create analyzer");
        goto cleanup;
    }
    if (pthread_create(&tid_monitor, NULL, monitor_thread, NULL) != 0) {
        perror("pthread_create monitor");
        goto cleanup;
    }

    /* ---- 10. Boucle principale ---- */
    printf("\033[1;32m[SecureBox] Surveillance active. En attente d'événements...\033[0m\n\n");
    logger_push_raw(LEVEL_START, "SecureBox démarré avec succès.");

    while (!signals_stop_requested()) {
        usleep(200000);
    }

    /* ---- 11. Arrêt propre ---- */
    printf("\n\033[1;36m[SecureBox] Arrêt en cours...\033[0m\n");

    /* Arrêter monitor et analyzer d'abord */
    monitor_stop();
    pthread_join(tid_monitor, NULL);

    analyzer_stop();
    pthread_join(tid_analyzer, NULL);

    /* Afficher stats sur console ET écrire dans le log
     * Le logger thread est encore actif → le fichier est ouvert
     * stats_display() écrit directement dans le fichier (pas via queue)
     * donc pas de doublon sur la console */
    stats_display();

    /* Message de fin puis arrêt du logger */
    logger_push_raw(LEVEL_STOP, "SecureBox arrêté proprement.");
    logger_stop();
    pthread_join(tid_logger, NULL);

cleanup:
    monitor_cleanup();
    analyzer_cleanup();
    logger_cleanup();
    stats_cleanup();

    printf("\033[1;32m[SecureBox] Ressources libérées. Au revoir.\033[0m\n");
    return EXIT_OK;
}
