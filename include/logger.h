
#ifndef LOGGER_H
#define LOGGER_H

#include <sys/types.h>

// Opening the log file
int logger_init(const char *filename);

// Saving to a log file
void logger_event(const char *event,
                  pid_t pid,
                  const char *detail);

// Child process status
void logger_status(pid_t pid,
                   const char *status);

// Closing the log file
void logger_close(void);

#endif
