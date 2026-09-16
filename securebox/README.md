# SecureBox — Système de surveillance Linux

## Description
SecureBox est un outil en langage C qui surveille un répertoire en temps réel grâce à inotify.

Il détecte :
- création / suppression / modification / renommage
- activités suspectes (fichiers cachés, extensions interdites, etc.)

## Compilation
make

## Exécution
./securebox <répertoire>

Exemple :
./securebox ./testdir

## Logs
logs/securebox.log

## Statistiques
kill -SIGUSR1 $(pgrep securebox)

## Arrêt
Ctrl+C ou kill -SIGTERM <pid>

## Structure
- monitor : surveillance inotify
- analyzer : analyse des événements
- logger : journalisation
- stats : statistiques
- signals : gestion des signaux

## Améliorations futures
- surveillance récursive
- interface graphique
- export CSV
