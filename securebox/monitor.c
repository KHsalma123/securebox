/* ============================================================
 * monitor.c - Module de surveillance inotify de SecureBox
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/select.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <limits.h>

#include "monitor.h"
#include "analyzer.h"
#include "logger.h"
#include "config.h"
#include "utils.h"

#define INOTIFY_BUF          (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))
#define CREATE_SUPPRESS_MS   500
#define RENAME_PAIR_TTL_MS   200
#define RECENT_CREATE_SIZE    32
#define PENDING_RENAME_SIZE   16

typedef struct {
    char           path[PATH_MAX];
    struct timeval ts;
    int            used;
} RecentCreate;
static RecentCreate g_recent_creates[RECENT_CREATE_SIZE];

typedef struct {
    uint32_t       cookie;
    char           path_from[PATH_MAX];
    struct timeval ts;
    int            used;
} PendingRename;
static PendingRename g_pending_renames[PENDING_RENAME_SIZE];

static int  g_inotify_fd      = -1;
static int  g_watch_fd        = -1;
static char g_watch_path[PATH_MAX_LEN];
static volatile int g_running = 1;
static int  g_pipe_fds[2]     = {-1, -1};

static long timeval_diff_ms(const struct timeval *a, const struct timeval *b) {
    return (long)(a->tv_sec  - b->tv_sec)  * 1000L
         + (long)(a->tv_usec - b->tv_usec) / 1000L;
}

static void build_full_path(char *dest, size_t size,
                             const char *dir, const char *name) {
    if (name && name[0] != '\0')
        snprintf(dest, size, "%s/%s", dir, name);
    else
        strncpy(dest, dir, size - 1);
    dest[size - 1] = '\0';
}

static void recent_create_add(const char *path) {
    struct timeval now;
    gettimeofday(&now, NULL);
    for (int i = 0; i < RECENT_CREATE_SIZE; i++) {
        if (!g_recent_creates[i].used) {
            strncpy(g_recent_creates[i].path, path, PATH_MAX - 1);
            g_recent_creates[i].ts   = now;
            g_recent_creates[i].used = 1;
            return;
        }
    }
    strncpy(g_recent_creates[0].path, path, PATH_MAX - 1);
    g_recent_creates[0].ts   = now;
    g_recent_creates[0].used = 1;
}

static int recent_create_check(const char *path) {
    struct timeval now;
    gettimeofday(&now, NULL);
    for (int i = 0; i < RECENT_CREATE_SIZE; i++) {
        if (!g_recent_creates[i].used) continue;
        if (strcmp(g_recent_creates[i].path, path) != 0) continue;
        long age = timeval_diff_ms(&now, &g_recent_creates[i].ts);
        g_recent_creates[i].used = 0;  
        return (age < CREATE_SUPPRESS_MS) ? 1 : 0;
    }
    return 0;
}

static void pending_rename_flush_expired(void) {
    struct timeval now;
    gettimeofday(&now, NULL);
    for (int i = 0; i < PENDING_RENAME_SIZE; i++) {
        if (!g_pending_renames[i].used) continue;
        if (timeval_diff_ms(&now, &g_pending_renames[i].ts) <= RENAME_PAIR_TTL_MS)
            continue;
        FsEvent orphan;
        memset(&orphan, 0, sizeof(orphan));
        orphan.type      = EVT_RENAME_FILE;
        orphan.timestamp = (time_t)g_pending_renames[i].ts.tv_sec;
        strncpy(orphan.path,      g_pending_renames[i].path_from, sizeof(orphan.path) - 1);
        strncpy(orphan.path_dest, "(destination inconnue)",       sizeof(orphan.path_dest) - 1);
        if (strstr(orphan.path, "securebox.log") == NULL)
            analyzer_push(&orphan);
        g_pending_renames[i].used = 0;
    }
}

static void pending_rename_add(uint32_t cookie, const char *path_from) {
    pending_rename_flush_expired();
    for (int i = 0; i < PENDING_RENAME_SIZE; i++) {
        if (!g_pending_renames[i].used) {
            g_pending_renames[i].cookie = cookie;
            strncpy(g_pending_renames[i].path_from, path_from, PATH_MAX - 1);
            gettimeofday(&g_pending_renames[i].ts, NULL);
            g_pending_renames[i].used = 1;
            return;
        }
    }
    g_pending_renames[0].cookie = cookie;
    strncpy(g_pending_renames[0].path_from, path_from, PATH_MAX - 1);
    gettimeofday(&g_pending_renames[0].ts, NULL);
    g_pending_renames[0].used = 1;
}

static int pending_rename_match(uint32_t cookie, char *out_from, size_t size) {
    for (int i = 0; i < PENDING_RENAME_SIZE; i++) {
        if (!g_pending_renames[i].used) continue;
        if (g_pending_renames[i].cookie != cookie) continue;
        strncpy(out_from, g_pending_renames[i].path_from, size - 1);
        out_from[size - 1] = '\0';
        g_pending_renames[i].used = 0;
        return 1;
    }
    return 0;
}

static void process_inotify_buffer(const char *buf, ssize_t len) {
    const char *ptr = buf;
    const char *end = buf + len;

    while (ptr < end) {
        const struct inotify_event *e = (const struct inotify_event *)ptr;
        ptr += sizeof(struct inotify_event) + e->len;

        if (e->len == 0) continue;

        char full_path[PATH_MAX];
        build_full_path(full_path, sizeof(full_path), g_watch_path, e->name);

        if (strstr(full_path, "securebox.log") != NULL) continue;

        FsEvent evt;
        memset(&evt, 0, sizeof(evt));
        evt.timestamp = time(NULL);

        if (e->mask & IN_CREATE) {
            if (!(e->mask & IN_ISDIR))
                recent_create_add(full_path);
            evt.type = (e->mask & IN_ISDIR) ? EVT_CREATE_DIR : EVT_CREATE_FILE;
            strncpy(evt.path, full_path, sizeof(evt.path) - 1);
            analyzer_push(&evt);

        } else if (e->mask & IN_DELETE) {
            evt.type = (e->mask & IN_ISDIR) ? EVT_DELETE_DIR : EVT_DELETE_FILE;
            strncpy(evt.path, full_path, sizeof(evt.path) - 1);
            analyzer_push(&evt);

        } else if (e->mask & IN_CLOSE_WRITE) {
            if (e->mask & IN_ISDIR) continue;
            if (recent_create_check(full_path)) continue;  
            evt.type = EVT_MODIFY_FILE;
            strncpy(evt.path, full_path, sizeof(evt.path) - 1);
            analyzer_push(&evt);

        } else if (e->mask & IN_ATTRIB) {
            if (!(e->mask & IN_ISDIR) && is_executable(full_path)) {
                evt.type = EVT_MODIFY_FILE;
                strncpy(evt.path,      full_path,     sizeof(evt.path) - 1);
                strncpy(evt.path_dest, "ATTRIB_EXEC", sizeof(evt.path_dest) - 1);
                analyzer_push(&evt);
            }

        } else if (e->mask & IN_MOVED_FROM) {
            pending_rename_add(e->cookie, full_path);

        } else if (e->mask & IN_MOVED_TO) {
            char path_from[PATH_MAX] = "";
            if (pending_rename_match(e->cookie, path_from, sizeof(path_from))) {
                evt.type = EVT_RENAME_FILE;
                strncpy(evt.path,      path_from, sizeof(evt.path) - 1);
                strncpy(evt.path_dest, full_path, sizeof(evt.path_dest) - 1);
                analyzer_push(&evt);
            } else {
                evt.type = EVT_CREATE_FILE;
                strncpy(evt.path, full_path, sizeof(evt.path) - 1);
                analyzer_push(&evt);
            }
        }
    }
}

int monitor_init(const char *watch_path) {
    strncpy(g_watch_path, watch_path, PATH_MAX_LEN - 1);
    g_watch_path[PATH_MAX_LEN - 1] = '\0';

    memset(g_recent_creates,  0, sizeof(g_recent_creates));
    memset(g_pending_renames, 0, sizeof(g_pending_renames));

    g_inotify_fd = inotify_init();
    if (g_inotify_fd < 0) {
        fprintf(stderr, "[ERREUR] inotify_init() : %s\n", strerror(errno));
        return -1;
    }

   
    uint32_t mask = IN_CREATE      |
                    IN_DELETE      |
                    IN_CLOSE_WRITE |
                    IN_ATTRIB      |
                    IN_MOVED_FROM  |
                    IN_MOVED_TO;

    g_watch_fd = inotify_add_watch(g_inotify_fd, watch_path, mask);
    if (g_watch_fd < 0) {
        fprintf(stderr, "[ERREUR] inotify_add_watch() sur '%s' : %s\n",
                watch_path, strerror(errno));
        close(g_inotify_fd);
        return -1;
    }

    if (pipe(g_pipe_fds) != 0) {
        fprintf(stderr, "[ERREUR] pipe() : %s\n", strerror(errno));
        inotify_rm_watch(g_inotify_fd, g_watch_fd);
        close(g_inotify_fd);
        return -1;
    }

    g_running = 1;

    char msg[256];
    snprintf(msg, sizeof(msg), "Surveillance démarrée sur : %s", watch_path);
    logger_push_raw(LEVEL_START, msg);
    printf("\033[1;32m[SecureBox] %s\033[0m\n", msg);
    return 0;
}

void *monitor_thread(void *arg) {
    (void)arg;

    char buf[INOTIFY_BUF]
        __attribute__((aligned(__alignof__(struct inotify_event))));

    fd_set read_fds;
    int maxfd = (g_inotify_fd > g_pipe_fds[0]) ? g_inotify_fd : g_pipe_fds[0];

    while (g_running) {
        struct timeval tv = {0, 300000};  /* timeout 300ms */
        FD_ZERO(&read_fds);
        FD_SET(g_inotify_fd, &read_fds);
        FD_SET(g_pipe_fds[0], &read_fds);

        int ret = select(maxfd + 1, &read_fds, NULL, NULL, &tv);
        if (ret < 0) {
            if (errno == EINTR) continue;
            perror("select");
            break;
        }

        pending_rename_flush_expired();

        if (ret == 0) continue;
        if (FD_ISSET(g_pipe_fds[0], &read_fds)) break;
        if (!FD_ISSET(g_inotify_fd, &read_fds)) continue;

        ssize_t len = read(g_inotify_fd, buf, sizeof(buf));
        if (len < 0) {
            if (errno == EINTR) continue;
            perror("read inotify");
            break;
        }

        process_inotify_buffer(buf, len);
    }
    return NULL;
}

void monitor_stop(void) {
    g_running = 0;
    if (g_pipe_fds[1] >= 0) {
        char c = 'x';
        write(g_pipe_fds[1], &c, 1);
    }
}

void monitor_cleanup(void) {
    if (g_watch_fd >= 0 && g_inotify_fd >= 0) {
        inotify_rm_watch(g_inotify_fd, g_watch_fd);
        g_watch_fd = -1;
    }
    if (g_inotify_fd >= 0) { close(g_inotify_fd); g_inotify_fd = -1; }
    if (g_pipe_fds[0] >= 0) { close(g_pipe_fds[0]); g_pipe_fds[0] = -1; }
    if (g_pipe_fds[1] >= 0) { close(g_pipe_fds[1]); g_pipe_fds[1] = -1; }
}
