#ifndef UTILS_H
#define UTILS_H

/* ============================================================
 * utils.h - Fonctions utilitaires
 * ============================================================ */

#include <time.h>

/* Formate la date/heure courante dans le buffer fourni */
void get_timestamp(char *buf, size_t size);

/* Vérifie si un fichier/dossier existe et est accessible */
int check_directory(const char *path);

/* Crée le répertoire de logs s'il n'existe pas */
int ensure_log_dir(const char *dir);

/* Retourne le nom de fichier à partir d'un chemin complet */
const char *get_filename(const char *path);

/* Retourne l'extension d'un fichier (avec le point), ou "" */
const char *get_extension(const char *filename);

/* Retourne 1 si le fichier est caché (commence par '.') */
int is_hidden_file(const char *filename);

/* Retourne 1 si l'extension est dans la liste noire */
int is_forbidden_extension(const char *filename);

/* Retourne 1 si le fichier a le bit exécutable */
int is_executable(const char *path);

/* Affiche un message d'erreur formaté sur stderr */
void print_error(const char *msg);

#endif /* UTILS_H */
