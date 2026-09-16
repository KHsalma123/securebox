#ifndef ANALYZER_H
#define ANALYZER_H

#include <time.h>
#include <pthread.h>

typedef enum {
    EVT_CREATE_FILE,
    EVT_CREATE_DIR,
    EVT_DELETE_FILE,
    EVT_DELETE_DIR,
    EVT_MODIFY_FILE,
    EVT_RENAME_FILE,
    EVT_UNKNOWN
} EventType;

typedef struct {
    EventType   type;
    char        path[1024];
    char        path_dest[1024];  
    time_t      timestamp;
} FsEvent;

#define QUEUE_SIZE 256
typedef struct {
    FsEvent  events[QUEUE_SIZE];
    int      head;
    int      tail;
    int      count;
    pthread_mutex_t mutex;
    pthread_cond_t  not_empty;
    pthread_cond_t  not_full;
} EventQueue;

const char *event_type_to_str(EventType type);
int   analyzer_init(void);
void  analyzer_push(FsEvent *evt);
void *analyzer_thread(void *arg);
void  analyzer_stop(void);
void  analyzer_cleanup(void);

#endif
