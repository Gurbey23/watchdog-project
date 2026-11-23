#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "process_manager.h"
#include "logger.h"
#include "signal_manager.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#define PERIODIC_CHECK_SEC 2

static ChildProcess g_children[MAX_CHILDREN];
static const char *g_worker_modes[MAX_CHILDREN] = {"normal", "daemon"};

static volatile sig_atomic_t g_sigchld_flag = 0;
static volatile sig_atomic_t g_sigalrm_flag = 0;

static void handle_sigchld(int sig) {
    (void)sig;
    g_sigchld_flag = 1;
}

static void handle_sigalrm(int sig) {
    (void)sig;
    g_sigalrm_flag = 1;
}

static int setup_process_signals_and_timer(void) {
    struct sigaction sa_chld;
    struct sigaction sa_alrm;
    struct itimerval timer;

    memset(&sa_chld, 0, sizeof(sa_chld));
    sa_chld.sa_handler = handle_sigchld;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_NOCLDSTOP;

    if (sigaction(SIGCHLD, &sa_chld, NULL) == -1) {
        perror("sigaction SIGCHLD");
        return -1;
    }

    memset(&sa_alrm, 0, sizeof(sa_alrm));
    sa_alrm.sa_handler = handle_sigalrm;
    sigemptyset(&sa_alrm.sa_mask);
    sa_alrm.sa_flags = 0;

    if (sigaction(SIGALRM, &sa_alrm, NULL) == -1) {
        perror("sigaction SIGALRM");
        return -1;
    }

    memset(&timer, 0, sizeof(timer));
    timer.it_value.tv_sec = PERIODIC_CHECK_SEC;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = PERIODIC_CHECK_SEC;
    timer.it_interval.tv_usec = 0;

    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        perror("setitimer");
        return -1;
    }

    return 0;
}

static int spawn_single_child(int index) {
    pid_t pid = fork();

    if (pid < 0) {
        char err_msg[128];
        snprintf(err_msg, sizeof(err_msg),
                 "fork() failed for '%s': %s",
                 g_children[index].name, strerror(errno));
        log_event("ERROR", err_msg);
        return -1;
    }

    if (pid == 0) {
        char *args[3];
        args[1] = (char *)g_worker_modes[index];
        args[2] = NULL;

        if (access("./worker", X_OK) == 0) {
            args[0] = "./worker";
            execv("./worker", args);
        } else if (access("./workers/worker", X_OK) == 0) {
            args[0] = "./workers/worker";
            execv("./workers/worker", args);
        }

        perror("execv worker failed");
        _exit(127);
    }

    g_children[index].pid = pid;
    g_children[index].is_running = 1;
    return 0;
}

static int find_child_index_by_pid(pid_t pid) {
    for (int i = 0; i < MAX_CHILDREN; i++) {
        if (g_children[i].pid == pid) {
            return i;
        }
    }
    return -1;
}

static void attempt_restart_child(int index) {
    char msg[256];

    if (g_shutdown_flag) {
        return;
    }

    if (g_children[index].restart_count >= MAX_RESTARTS) {
        snprintf(msg, sizeof(msg),
                 "Restart limit (%d) reached for child '%s'. Will not restart.",
                 MAX_RESTARTS, g_children[index].name);
        log_event("ERROR", msg);
        return;
    }

    g_children[index].restart_count++;
    snprintf(msg, sizeof(msg),
             "Restarting child '%s' (Attempt %d/%d)...",
             g_children[index].name,
             g_children[index].restart_count,
             MAX_RESTARTS);
    log_event("INFO", msg);

    if (spawn_single_child(index) == 0) {
        snprintf(msg, sizeof(msg),
                 "Child '%s' restarted successfully with new PID: %d.",
                 g_children[index].name, (int)g_children[index].pid);
        log_event("INFO", msg);
    }
}

int start_child_processes(void) {
    char msg[256];

    if (setup_process_signals_and_timer() != 0) {
        return -1;
    }

    for (int i = 0; i < MAX_CHILDREN; i++) {
        memset(&g_children[i], 0, sizeof(ChildProcess));
        snprintf(g_children[i].name, sizeof(g_children[i].name),
                 "worker-%d", i + 1);
        g_children[i].restart_count = 0;

        if (spawn_single_child(i) != 0) {
            return -1;
        }

        snprintf(msg, sizeof(msg),
                 "Started child '%s' (mode: %s) with PID: %d.",
                 g_children[i].name, g_worker_modes[i], (int)g_children[i].pid);
        log_event("INFO", msg);
    }

    return 0;
}

static void reap_terminated_children(void) {
    int status = 0;
    pid_t pid;
    char msg[256];

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        int idx = find_child_index_by_pid(pid);
        if (idx == -1) {
            continue;
        }

        g_children[idx].is_running = 0;

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            snprintf(msg, sizeof(msg),
                     "Child '%s' (PID: %d) terminated normally (exit code 0). No restart needed.",
                     g_children[idx].name, (int)pid);
            log_event("INFO", msg);
        } else if (WIFSIGNALED(status)) {
            snprintf(msg, sizeof(msg),
                     "Child '%s' (PID: %d) terminated unexpectedly by signal %d.",
                     g_children[idx].name, (int)pid, WTERMSIG(status));
            log_event("WARN", msg);
            attempt_restart_child(idx);
        } else if (WIFEXITED(status)) {
            snprintf(msg, sizeof(msg),
                     "Child '%s' (PID: %d) exited unexpectedly with error code %d.",
                     g_children[idx].name, (int)pid, WEXITSTATUS(status));
            log_event("WARN", msg);
            attempt_restart_child(idx);
        }
    }
}

static void perform_periodic_health_check(void) {
    char msg[256];

    for (int i = 0; i < MAX_CHILDREN; i++) {
        if (g_children[i].is_running == 1 && g_children[i].pid > 0) {
            if (kill(g_children[i].pid, 0) == -1 && errno == ESRCH) {
                g_children[i].is_running = 0;
                snprintf(msg, sizeof(msg),
                         "Periodic check: child '%s' (PID: %d) no longer exists.",
                         g_children[i].name, (int)g_children[i].pid);
                log_event("WARN", msg);
                attempt_restart_child(i);
            }
        }
    }
}

void monitor_and_restart_children(void) {
    if (g_shutdown_flag) {
        return;
    }

    if (g_sigchld_flag) {
        g_sigchld_flag = 0;
        reap_terminated_children();
    }

    if (g_sigalrm_flag) {
        g_sigalrm_flag = 0;
        reap_terminated_children();
        perform_periodic_health_check();
    }
}

ChildProcess *get_child_processes(int *count) {
    if (count != NULL) {
        *count = MAX_CHILDREN;
    }
    return g_children;
}