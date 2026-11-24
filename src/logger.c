
#include "logger.h"
#include <stdio.h>
#include <time.h>
#include <unistd.h>

static FILE *log_file = NULL;

int logger_init(const char *filename)
{
    if (filename == NULL) {
        return -1;
    }

    log_file = fopen(filename, "a");

    if (log_file == NULL) {
        perror("Failed to open log file");
        return -1;
    }

    return 0;
}

void logger_close(void)
{
    if (log_file != NULL) {
        fclose(log_file);
        log_file = NULL;
    }
}

void logger_event(const char *event, pid_t pid,
                  const char *detail)
{
    if (log_file == NULL || event == NULL) {
        return;
    }

    time_t now = time(NULL);
    struct tm local_time;
    char timestamp[32];

    if (now == (time_t)-1 ||
        localtime_r(&now, &local_time) == NULL ||
        strftime(timestamp, sizeof(timestamp),
                 "%Y-%m-%d %H:%M:%S", &local_time) == 0) {
        snprintf(timestamp, sizeof(timestamp), "unknown");
    }

    fprintf(log_file,
            "[%s] %s pid=%ld detail=%s\n",
            timestamp,
            event,
            (long)pid,
            detail != NULL ? detail : "-");

    fflush(log_file);
}

void logger_status(pid_t pid, const char *status)
{
    logger_event("STATUS", pid, status);
}

int init_logger(const char *filename)
{
    return logger_init(filename);
}

void log_event(const char *level, const char *message)
{
    logger_event(level, getpid(), message);
}

void close_logger(void)
{
    logger_close();
}
