# ============================================================
# Makefile - SecureBox
# Compilation : make
# Nettoyage   : make clean
# Debug       : make debug
# ============================================================

CC      = gcc
TARGET  = securebox

# Sources et objets
SRCS    = main.c monitor.c analyzer.c logger.c stats.c signals.c utils.c
OBJS    = $(SRCS:.c=.o)

# Flags de compilation
CFLAGS  = -Wall -Wextra -std=gnu11 -pthread -D_GNU_SOURCE
LDFLAGS = -pthread

# Flags debug (avec adresse sanitizer)
DEBUG_FLAGS = -g -DDEBUG -fsanitize=address,undefined

# ---- Règle principale ----
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Compilation réussie → ./$(TARGET)"

# ---- Compilation de chaque .c en .o ----
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# ---- Cible debug ----
debug: CFLAGS += $(DEBUG_FLAGS)
debug: LDFLAGS += -fsanitize=address,undefined
debug: $(TARGET)
	@echo "Build debug prêt."

# ---- Nettoyage ----
clean:
	rm -f $(OBJS) $(TARGET)
	@echo "Nettoyage terminé."

# ---- Création du répertoire logs ----
logs:
	mkdir -p logs

# ---- Dépendances explicites ----
main.o:     main.c config.h utils.h logger.h analyzer.h monitor.h stats.h signals.h
monitor.o:  monitor.c monitor.h analyzer.h logger.h config.h utils.h
analyzer.o: analyzer.c analyzer.h logger.h stats.h utils.h config.h
logger.o:   logger.c logger.h config.h utils.h
stats.o:    stats.c stats.h logger.h config.h
signals.o:  signals.c signals.h stats.h logger.h config.h
utils.o:    utils.c utils.h config.h

.PHONY: all debug clean logs
