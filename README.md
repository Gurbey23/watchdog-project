
# Watchdog - Process Monitoring System

## Project Description

This project is developed for the System Programming
course at Sakarya University.

The aim is to implement a POSIX signal-based process
monitoring and management system using C.

The watchdog program is designed to create, monitor,
and manage child processes.

## Planned Features

- Create at least two child processes using fork() and exec().
- Monitor child termination using SIGCHLD and waitpid().
- Restart unexpectedly terminated processes up to 3 times.
- Generate child process status reports using SIGUSR1.
- Handle SIGINT and SIGTERM for graceful shutdown.
- Record important process events in a log file.

## Requirements

- Linux operating system
- GCC compiler
- GNU Make

On Ubuntu, install the required development tools:

```bash
sudo apt install build-essential
```

## Compilation

Build the complete project with:

```bash
make
```

The expected executable is named `watchdog`.

Note: The main watchdog modules are still under development.

## Execution

After the complete project has been built:

```bash
./watchdog
```

## Logging

The logging module records events with timestamps,
process IDs (PIDs), and event descriptions.

Example log format:

```text
[YYYY-MM-DD HH:MM:SS] START pid=1234 detail=Worker started
[YYYY-MM-DD HH:MM:SS] STATUS pid=1234 detail=RUNNING
```

## Logger Test

The logger module can be tested independently:

```bash
make test-logger
```

Expected output:

```text
Logger test completed.
```

## Cleaning Build Files

```bash
make clean
```

## Development Status

The basic logging module and logger test are implemented.
The remaining modules and integration tests are in progress.
