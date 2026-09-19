# 🔒 SecureBox — Real-Time File System Monitoring & Security Tool

**SecureBox** is a lightweight real-time file system monitoring and security analysis tool developed in **C for Linux**.

The application continuously monitors a target directory using the Linux **`inotify` API**, detects file system activities, analyzes potentially suspicious behavior, and generates security alerts according to configurable detection rules.

The project combines **real-time monitoring, asynchronous logging, anomaly detection, statistics, and signal handling** in a multi-threaded architecture.

---

## 📋 Table of Contents

* [Overview](#-overview)
* [Features](#-features)
* [Detection Rules](#-detection-rules)
* [Architecture](#-architecture)
* [Project Structure](#-project-structure)
* [Requirements](#-requirements)
* [Installation](#-installation)
* [Build](#-build)
* [Usage](#-usage)
* [Testing](#-testing)
* [Runtime Statistics](#-runtime-statistics)
* [Logging](#-logging)
* [Signal Handling](#-signal-handling)
* [Configuration](#-configuration)
* [Technical Concepts](#-technical-concepts)
* [Limitations](#-limitations)
* [Future Improvements](#-future-improvements)
* [Author](#-author)

---

## 📌 Overview

SecureBox is designed to monitor file system activity in real time and identify events that may indicate suspicious or unauthorized behavior.

The application watches a user-defined directory and detects operations such as:

* File creation
* File modification
* File deletion
* File renaming
* Permission changes

Each detected event can be logged, analyzed, and included in runtime statistics.

When an event matches one of the configured security rules, SecureBox generates an alert identifying the type of detected anomaly.

### Main objectives

* Monitor file system activity in real time
* Detect potentially suspicious file operations
* Generate security alerts
* Maintain detailed event logs
* Provide live monitoring statistics
* Demonstrate Linux system programming concepts
* Apply multi-threading and asynchronous processing in C

---

# ✨ Features

## 👁️ Real-Time File System Monitoring

SecureBox uses the Linux **`inotify` API** to monitor a target directory and detect file system events as they occur.

Supported events include:

* File creation
* File modification
* File deletion
* File renaming
* Permission changes

---

## 🚨 Anomaly Detection

SecureBox analyzes file system events using configurable security rules.

Current detection mechanisms include:

* Hidden file creation
* Forbidden or suspicious file extensions
* Executable permission changes
* Deletion bursts
* Rapid repeated modifications

Each detected anomaly generates a corresponding security alert.

---

## 📝 Event Logging

File system events and generated alerts are written to a log file.

The logging system records information such as:

* Timestamp
* Event type
* File name/path
* Security alert
* Relevant event information

Log files are generated at runtime and are excluded from version control.

---

## 📊 Live Statistics

SecureBox maintains runtime statistics, including:

* Total number of events
* Number of creations
* Number of modifications
* Number of deletions
* Number of renames
* Number of triggered alerts

Statistics can be requested while the application is running.

---

## 🧵 Multi-Threaded Architecture

The application uses multiple threads to separate its main responsibilities.

The architecture includes dedicated processing for:

* File system monitoring
* Event analysis
* Event logging

This allows monitoring, analysis, and logging operations to be handled independently.

---

## 🛑 Graceful Shutdown

SecureBox handles system signals to allow the application to terminate cleanly.

Supported controls include:

* `Ctrl+C`
* `SIGTERM`
* `SIGUSR1`

Before shutting down, the application can complete its cleanup operations and release allocated resources.

---

# 🚨 Detection Rules

SecureBox currently implements the following anomaly detection rules:

| Detection                    | Trigger                                                       | Alert               |
| ---------------------------- | ------------------------------------------------------------- | ------------------- |
| Hidden file creation         | Creation of a hidden file                                     | `HIDDEN_FILE`       |
| Forbidden extension          | Creation/use of suspicious extensions such as `.sh` or `.exe` | `FORBIDDEN_EXT`     |
| Executable permission change | File permissions changed to executable                        | `EXEC_CHMOD`        |
| Deletion burst               | More than 5 deletions within 10 seconds                       | Deletion burst      |
| Rapid modifications          | Same file modified more than 3 times within 5 seconds         | Rapid modifications |

### Example

Creating a hidden file:

```bash
touch ./testdir/.secret
```

can trigger:

```text
HIDDEN_FILE
```

Creating a file with a forbidden extension:

```bash
touch ./testdir/malware.sh
```

can trigger:

```text
FORBIDDEN_EXT
```

Making a script executable:

```bash
chmod +x ./testdir/script.sh
```

can trigger:

```text
EXEC_CHMOD
```

---

# 🏗️ Architecture

SecureBox follows a modular and multi-threaded architecture.

```text
                         ┌─────────────────────┐
                         │      SecureBox      │
                         └──────────┬──────────┘
                                    │
                    ┌───────────────┼───────────────┐
                    │               │               │
                    ▼               ▼               ▼
             ┌────────────┐  ┌────────────┐  ┌────────────┐
             │  Monitor   │  │  Analyzer  │  │   Logger   │
             │   Thread   │  │   Thread   │  │   Thread   │
             └─────┬──────┘  └─────┬──────┘  └─────┬──────┘
                   │               │               │
                   ▼               ▼               ▼
              inotify API      Detection        Log File
                   │               Rules
                   │               │
                   └───────┬───────┘
                           ▼
                    ┌─────────────┐
                    │    Stats    │
                    │   Module    │
                    └─────────────┘
                           │
                           ▼
                    Runtime Output
```

### Event processing flow

```text
File System Event
       │
       ▼
    inotify
       │
       ▼
   Monitor
       │
       ▼
   Event Queue
       │
       ├───────────────┐
       ▼               ▼
  Analyzer          Logger
       │               │
       ▼               ▼
Detection Rules    Log File
       │
       ▼
Security Alert
       │
       ▼
    Statistics
```

---

# 📁 Project Structure

```text
SecureBox/
│
├── securebox/
│   │
│   ├── main.c
│   ├── config.h
│   │
│   ├── monitor.c
│   ├── monitor.h
│   │
│   ├── analyzer.c
│   ├── analyzer.h
│   │
│   ├── logger.c
│   ├── logger.h
│   │
│   ├── stats.c
│   ├── stats.h
│   │
│   ├── signals.c
│   ├── signals.h
│   │
│   ├── utils.c
│   ├── utils.h
│   │
│   ├── testdir/
│   │
│   ├── logs/
│   │
│   └── Makefile
│
├── rapport-administration.pdf
│
└── README.md
```

### Module responsibilities

| Module         | Responsibility                             |
| -------------- | ------------------------------------------ |
| `main.c`       | Application entry point and initialization |
| `config.h`     | Global configuration parameters            |
| `monitor.c/h`  | File system monitoring using `inotify`     |
| `analyzer.c/h` | Event analysis and anomaly detection       |
| `logger.c/h`   | Asynchronous event logging                 |
| `stats.c/h`    | Runtime statistics management              |
| `signals.c/h`  | Linux signal handling                      |
| `utils.c/h`    | Shared utility functions                   |
| `Makefile`     | Compilation and build configuration        |

---

# 💻 Requirements

SecureBox is designed for **Linux** because it relies on the Linux-specific `inotify` API.

### Required software

* Linux operating system
* GCC
* GNU Make
* POSIX Threads (`pthread`)

Check the installed tools:

```bash
gcc --version
make --version
```

---

# 📥 Installation

Clone the repository:

```bash
git clone https://github.com/KHsalma123/SecureBox.git
```

Navigate to the project:

```bash
cd SecureBox
```

If the source code is located in the `securebox` directory:

```bash
cd securebox
```

---

# 🔨 Build

Compile the project using the provided `Makefile`:

```bash
make
```

The compilation process produces the executable:

```text
securebox
```

To remove generated object files and the executable:

```bash
make clean
```

---

# ▶️ Usage

SecureBox requires a directory path to monitor.

```bash
./securebox <directory_path>
```

### Example

```bash
./securebox ./testdir
```

Once started, SecureBox monitors the specified directory and processes file system events in real time.

---

# 🧪 Testing

The `testdir/` directory can be used to test SecureBox's detection mechanisms.

## 1. Hidden File Detection

Create a hidden file:

```bash
touch ./testdir/.secret
```

Expected alert:

```text
HIDDEN_FILE
```

---

## 2. Forbidden Extension Detection

Create a file with a suspicious extension:

```bash
touch ./testdir/malware.sh
```

or:

```bash
touch ./testdir/virus.exe
```

Expected alert:

```text
FORBIDDEN_EXT
```

> The `malware.sh` and `virus.exe` test files are empty and harmless. They are included only to test extension-based detection.

---

## 3. Executable Permission Detection

Create a script:

```bash
touch ./testdir/script.sh
```

Then make it executable:

```bash
chmod +x ./testdir/script.sh
```

Expected alert:

```text
EXEC_CHMOD
```

---

## 4. Deletion Burst Detection

Create several test files:

```bash
touch ./testdir/file1
touch ./testdir/file2
touch ./testdir/file3
touch ./testdir/file4
touch ./testdir/file5
touch ./testdir/file6
```

Delete more than five files within ten seconds:

```bash
rm ./testdir/file1
rm ./testdir/file2
rm ./testdir/file3
rm ./testdir/file4
rm ./testdir/file5
rm ./testdir/file6
```

This can trigger the deletion-burst detection rule.

---

## 5. Rapid Modification Detection

Modify the same file repeatedly:

```bash
echo "change 1" >> ./testdir/test.txt
echo "change 2" >> ./testdir/test.txt
echo "change 3" >> ./testdir/test.txt
echo "change 4" >> ./testdir/test.txt
```

More than three modifications within five seconds can trigger the rapid-modification detection rule.

---

# 📊 Runtime Statistics

SecureBox continuously maintains statistics about monitored activity.

The following information is tracked:

```text
Total Events
Creations
Modifications
Deletions
Renames
Alerts Triggered
```

Statistics can be displayed while the application is running using:

```bash
kill -SIGUSR1 <pid>
```

Replace `<pid>` with the process ID of the running SecureBox instance.

---

# 📝 Logging

SecureBox records monitored events and security alerts in log files.

The log location is configured through:

```text
config.h
```

The runtime logs directory is:

```text
logs/
```

Generated log files are intentionally excluded from Git version control.

Example:

```text
logs/
└── securebox_YYYY-MM-DD_HH-MM-SS.log
```

---

# 🛑 Signal Handling

SecureBox supports several system signals.

| Signal    | Action                                             |
| --------- | -------------------------------------------------- |
| `SIGINT`  | Graceful shutdown, typically generated by `Ctrl+C` |
| `SIGTERM` | Graceful shutdown                                  |
| `SIGUSR1` | Display current statistics                         |

### Stop the application

Press:

```text
Ctrl+C
```

or use:

```bash
kill -SIGTERM <pid>
```

### Display statistics

```bash
kill -SIGUSR1 <pid>
```

---

# ⚙️ Configuration

Global configuration parameters are defined in:

```text
config.h
```

This file can be used to configure application-level settings such as:

* Log file location
* Detection thresholds
* Monitoring parameters
* Security rules

The configuration is kept separate from the main monitoring and analysis logic to make the application easier to maintain and modify.

---

# 🧠 Technical Concepts

SecureBox demonstrates several important Linux and C programming concepts.

### `inotify`

Linux's `inotify` subsystem allows applications to receive notifications when files or directories are modified.

SecureBox uses it as the foundation of its real-time monitoring system.

### POSIX Threads

The application uses the `pthread` library to execute monitoring, analysis, and logging tasks concurrently.

### System Signals

Linux signals provide a mechanism for controlling the application without requiring additional user interfaces.

### File Permissions

SecureBox analyzes permission changes to identify when a file becomes executable.

### Time-Based Detection

Some anomaly detection rules rely on temporal thresholds, such as:

```text
More than 5 deletions / 10 seconds
More than 3 modifications / 5 seconds
```

These rules allow SecureBox to identify unusually frequent file system activity.

---

# ⚠️ Limitations

SecureBox is primarily an educational and experimental security monitoring project.

Current limitations include:

* Linux-specific implementation
* Directory-based monitoring
* Rule-based anomaly detection
* No persistent database for security events
* No graphical monitoring interface
* No remote alerting system
* Detection rules are based on predefined thresholds

The presence of an alert indicates that an event matched a configured rule; it does not by itself prove that malicious activity occurred.

---

# 🔮 Future Improvements

Potential improvements include:

* [ ] Recursive monitoring of subdirectories
* [ ] More advanced anomaly detection
* [ ] Configurable detection rules through an external configuration file
* [ ] JSON-formatted logs
* [ ] Centralized event storage
* [ ] Web-based monitoring dashboard
* [ ] Email or messaging notifications
* [ ] IP/network activity correlation
* [ ] Improved alert severity levels
* [ ] Unit and integration tests
* [ ] Docker-based development environment
* [ ] CI/CD integration
* [ ] Extended Linux security monitoring

---

# 👩‍💻 Author

**Salma Khaliqi**

* GitHub: [@KHsalma123](https://github.com/KHsalma123)
* LinkedIn: [Salma Khaliqi](https://www.linkedin.com/in/salma-khaliqi-1087182a9/)

---

## 📄 License

This project was developed for educational and academic purposes.
