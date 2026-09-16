#ifndef SIGNALS_H
#define SIGNALS_H

/* ============================================================
 * signals.h - Gestion des signaux système
 * ============================================================ */

/* Installe les gestionnaires de signaux (SIGINT, SIGTERM, SIGUSR1) */
int signals_init(void);

/* Retourne 1 si un signal d'arrêt a été reçu */
int signals_stop_requested(void);

#endif /* SIGNALS_H */
