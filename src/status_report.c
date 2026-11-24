
#include "logger.h"
#include "process_manager.h"
#include <stdio.h>

void log_children_status(void)
{
    int count = 0;
    ChildProcess *children = get_child_processes(&count);

    if (children == NULL || count <= 0) {
        log_event("INFO", "No child processes registered.");
        return;
    }

    int running_count = 0;

    for (int i = 0; i < count; i++) {
        if (children[i].is_running != 1 ||
            children[i].pid <= 0) {
            continue;
        }

        char details[128];

        snprintf(details, sizeof(details),
                 "name=%s state=RUNNING restarts=%d/%d",
                 children[i].name,
                 children[i].restart_count,
                 MAX_RESTARTS);

        logger_status(children[i].pid, details);
        running_count++;
    }

    if (running_count == 0) {
        log_event("INFO", "No running child processes.");
    }
}
