#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include <sys/types.h>

#define MAX_CHILDREN 2
#define MAX_RESTARTS 3

typedef struct {
    pid_t pid;
    int is_running;
    int restart_count;
    char name[32];
} ChildProcess;

int start_child_processes(void);
void monitor_and_restart_children(void);
ChildProcess *get_child_processes(int *count);

#endif
