
# Watchdog - Process Monitoring System

## Project Description

This project was developed for the System Programming course
at Sakarya University.

The project implements a POSIX signal-based process monitoring
and management system using the C programming language.

The watchdog creates, monitors, restarts, and safely terminates
child processes.

## Features

- Creates two child processes using fork() and execv().
- Monitors child termination using SIGCHLD and waitpid().
- Performs periodic health checks using SIGALRM.
- Restarts unexpectedly terminated child processes.
- Limits automatic restarts to a maximum of 3 attempts per child.
- Generates status reports when SIGUSR1 is received.
- Handles SIGINT and SIGTERM for graceful shutdown.
- Records process events with timestamps and PIDs in watchdog.log.

## Requirements

- Linux operating system (Ubuntu or WSL2)
- GCC compiler
- GNU Make
- Bash
- procps (for pgrep in automated tests)

Install the required tools on Ubuntu:

```bash
sudo apt update
sudo apt install build-essential procps
```

## Compilation

Compile the complete project:

```bash
make
```

This builds:

- watchdog
- workers/worker

## Execution

Start the watchdog:

```bash
./watchdog
```

The watchdog starts two worker processes:

- worker-1 (normal mode): exits successfully after 15 seconds.
- worker-2 (daemon mode): continues running until terminated.

## Signal Handling

### SIGUSR1 - Status Report

Start the watchdog in the background:

```bash
./watchdog &
WATCHDOG_PID=$!
```

Request a child process status report:

```bash
kill -USR1 "$WATCHDOG_PID"
```

The watchdog records running child process information,
including PID, name, state, and restart count.

### SIGTERM - Graceful Shutdown

Stop the watchdog safely:

```bash
kill -TERM "$WATCHDOG_PID"
wait "$WATCHDOG_PID"
```

The watchdog sends termination signals to running children
and waits for them to exit.

SIGINT can also be used to request graceful shutdown.

## Restart Mechanism

When a child terminates unexpectedly, the watchdog attempts
to restart it with a new PID.

Each child can be restarted at most 3 times.

After the restart limit is reached, the watchdog records
an error message and does not restart that child again.

A child that exits normally with exit code 0 is not restarted.

## Logging

All log messages are saved to:

```text
watchdog.log
```

Example log messages:

```text
[2026-11-24 14:45:53] INFO pid=1083 detail=Watchdog started. Monitoring child processes...
[2026-11-24 14:46:39] STATUS pid=1089 detail=name=worker-2 state=RUNNING restarts=0/3
```

## Testing

### Logger Test

Run the independent logger test:

```bash
make test_logger
```

### Automated Integration Tests

Run all automated tests:

```bash
bash tests/test_scenarios.sh
```

The test script verifies:

1. Logger event recording
2. SIGUSR1 status reporting
3. Automatic restart after unexpected termination
4. Maximum 3 restart attempts
5. Normal exit without restart
6. Graceful shutdown using SIGTERM
7. Restart after a non-zero exit code

All seven checks passed during local testing on Ubuntu.

## Project Structure

```text
include/
    logger.h
    process_manager.h
    signal_manager.h

src/
    main.c
    logger.c
    process_manager.c
    signal_manager.c
    status_report.c

workers/
    worker.c

tests/
    test_logger.c
    test_scenarios.sh

Makefile
README.md
```

## Cleaning Build Files

Remove generated build files:

```bash
make clean
```
