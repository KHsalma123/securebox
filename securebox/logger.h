#ifndef LOGGER_H
#define LOGGER_H

/* ============================================================
 * logger.h - Module de journalisation
 * ============================================================ */

#include <stdio.h>
#include <pthread.h>

typedef struct {
    char message[512];
} LogEntry;

typedef struct {
    LogEntry  entries[256];
    int       head;
    int       tail;
    int       count;
    pthread_mutex_t mutex;
    pthread_cond_t  not_empty;
    pthread_cond_t  not_full;
} LogQueue;

int   logger_init(const char *log_path);
void  logger_push(const char *level, const char *event_type, const char *resource);
void  logger_push_raw(const char *level, const char *message);
FILE *logger_get_file(void);   /* accès direct au fichier pour stats */
void *logger_thread(void *arg);
void  logger_stop(void);
void  logger_cleanup(void);

#endif /* LOGGER_H */
