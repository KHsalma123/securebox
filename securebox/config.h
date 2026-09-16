#ifndef CONFIG_H
#define CONFIG_H

/* ============================================================
 * config.h - Constantes de configuration de SecureBox
 * ============================================================ */

/* Chemins */
#define LOG_DIR         "logs"
#define LOG_FILE        "logs/securebox.log"

/* Taille des buffers */
#define PATH_MAX_LEN    1024
#define BUF_SIZE        4096
#define EVENT_BUF_LEN   (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))
#define MSG_MAX_LEN     512
#define QUEUE_SIZE      256

/* Règles de détection d'anomalies */
#define ALERT_DELETE_COUNT      5       /* nb suppressions suspectes */
#define ALERT_DELETE_WINDOW     10      /* en secondes */
#define ALERT_MODIFY_COUNT      3       /* nb modifications suspectes */
#define ALERT_MODIFY_WINDOW     5       /* en secondes */

/* Extensions interdites */
#define FORBIDDEN_EXTENSIONS    ".sh", ".exe", ".bat", ".bin", ".cmd", ".vbs", NULL

/* Niveaux de log */
#define LEVEL_INFO      "INFO  "
#define LEVEL_ALERT     "ALERTE"
#define LEVEL_ERROR     "ERREUR"
#define LEVEL_START     "START "
#define LEVEL_STOP      "STOP  "

/* Codes de retour */
#define EXIT_OK         0
#define EXIT_ERR        1

#endif /* CONFIG_H */
