#ifndef STATS_H
#define STATS_H

/* ============================================================
 * stats.h - Module de statistiques
 * ============================================================ */

#include <stdio.h>
#include <time.h>
#include <pthread.h>

typedef struct {
    long total;
    long creations;
    long suppressions;
    long modifications;
    long renommages;
    long alertes;
    time_t start_time;
    pthread_mutex_t mutex;
} Stats;

void  stats_init(void);
void  stats_set_logfile(FILE *fp);   /* donne accès direct au fichier log */
void  stats_inc_total(void);
void  stats_inc_creations(void);
void  stats_inc_suppressions(void);
void  stats_inc_modifications(void);
void  stats_inc_renommages(void);
void  stats_inc_alertes(void);
void  stats_display(void);           /* console + log en une seule fois */
void  stats_cleanup(void);

#endif /* STATS_H */
