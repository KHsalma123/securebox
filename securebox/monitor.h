#ifndef MONITOR_H
#define MONITOR_H

/* ============================================================
 * monitor.h - Module de surveillance inotify
 * ============================================================ */

/* Initialise inotify sur le répertoire cible */
int  monitor_init(const char *watch_path);

/* Fonction du thread de surveillance */
void *monitor_thread(void *arg);

/* Demande l'arrêt du thread */
void monitor_stop(void);

/* Libère les ressources inotify */
void monitor_cleanup(void);

#endif /* MONITOR_H */
