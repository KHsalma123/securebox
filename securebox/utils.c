/* ============================================================
 * utils.c - Fonctions utilitaires de SecureBox
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#include "utils.h"
#include "config.h"

/* ----------------------------------------------------------------
 * get_timestamp : remplit buf avec la date/heure au format
 *                 [YYYY-MM-DD HH:MM:SS]
 * ---------------------------------------------------------------- */
void get_timestamp(char *buf, size_t size) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    if (tm_info == NULL) {
        strncpy(buf, "[0000-00-00 00:00:00]", size - 1);
        buf[size - 1] = '\0';
        return;
    }
    strftime(buf, size, "[%Y-%m-%d %H:%M:%S]", tm_info);
}

/* ----------------------------------------------------------------
 * check_directory : vérifie l'existence et les droits sur le
 *                   répertoire cible. Retourne 0 si OK, -1 sinon.
 * ---------------------------------------------------------------- */
int check_directory(const char *path) {
    struct stat st;

    if (path == NULL || path[0] == '\0') {
        fprintf(stderr, "[ERREUR] Chemin vide ou NULL.\n");
        return -1;
    }

    if (stat(path, &st) != 0) {
        fprintf(stderr, "[ERREUR] Impossible d'accéder à '%s' : %s\n",
                path, strerror(errno));
        return -1;
    }

    if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "[ERREUR] '%s' n'est pas un répertoire.\n", path);
        return -1;
    }

    if (access(path, R_OK | X_OK) != 0) {
        fprintf(stderr, "[ERREUR] Droits insuffisants sur '%s' : %s\n",
                path, strerror(errno));
        return -1;
    }

    return 0;
}

/* ----------------------------------------------------------------
 * ensure_log_dir : crée le répertoire de logs si absent.
 *                  Retourne 0 si OK, -1 sinon.
 * ---------------------------------------------------------------- */
int ensure_log_dir(const char *dir) {
    struct stat st;

    if (stat(dir, &st) == 0) {
        if (S_ISDIR(st.st_mode))
            return 0; 
        fprintf(stderr, "[ERREUR] '%s' existe mais n'est pas un répertoire.\n", dir);
        return -1;
    }

    /* Créer avec rwxr-xr-x */
    if (mkdir(dir, 0755) != 0) {
        fprintf(stderr, "[ERREUR] Impossible de créer '%s' : %s\n",
                dir, strerror(errno));
        return -1;
    }

    return 0;
}

/* ----------------------------------------------------------------
 * get_filename : retourne le pointeur vers le nom de fichier dans
 *                le chemin complet (après le dernier '/').
 * ---------------------------------------------------------------- */
const char *get_filename(const char *path) {
    if (path == NULL) return "";
    const char *slash = strrchr(path, '/');
    return (slash != NULL) ? slash + 1 : path;
}

/* ----------------------------------------------------------------
 * get_extension : retourne le pointeur vers l'extension du fichier
 *                 (le dernier '.'), ou "" s'il n'y en a pas.
 * ---------------------------------------------------------------- */
const char *get_extension(const char *filename) {
    if (filename == NULL) return "";
    const char *dot = strrchr(filename, '.');
    /* Ne pas confondre les fichiers cachés (.bashrc) avec une extension */
    if (dot == NULL || dot == filename) return "";
    return dot;
}

/* ----------------------------------------------------------------
 * is_hidden_file : retourne 1 si le nom commence par '.'.
 * ---------------------------------------------------------------- */
int is_hidden_file(const char *filename) {
    if (filename == NULL) return 0;
    const char *name = get_filename(filename);
    return (name[0] == '.');
}

/* ----------------------------------------------------------------
 * is_forbidden_extension : retourne 1 si l'extension est interdite.
 * ---------------------------------------------------------------- */
int is_forbidden_extension(const char *filename) {
    if (filename == NULL) return 0;

    const char *forbidden[] = { FORBIDDEN_EXTENSIONS };
    const char *ext = get_extension(get_filename(filename));

    if (ext[0] == '\0') return 0;

    for (int i = 0; forbidden[i] != NULL; i++) {
        if (strcasecmp(ext, forbidden[i]) == 0)
            return 1;
    }
    return 0;
}

/* ----------------------------------------------------------------
 * is_executable : retourne 1 si le fichier a le bit exécutable.
 * ---------------------------------------------------------------- */
int is_executable(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) ? 1 : 0;
}

/* ----------------------------------------------------------------
 * print_error : affiche un message d'erreur formaté sur stderr.
 * ---------------------------------------------------------------- */
void print_error(const char *msg) {
    char ts[32];
    get_timestamp(ts, sizeof(ts));
    fprintf(stderr, "%s [ERREUR] %s\n", ts, msg);
}
