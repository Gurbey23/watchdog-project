
#include <stdio.h>
#include <unistd.h>
#include "logger.h"

int main(void)
{
    pid_t pid = getpid();

    if (logger_init("watchdog.log") != 0) {
        return 1;
    }

    logger_event("START", pid, "Worker started");
    logger_status(pid, "RUNNING");
    logger_event("EXIT", pid, "Worker terminated");

    logger_close();

    printf("Logger test completed.\n");

    return 0;
}
