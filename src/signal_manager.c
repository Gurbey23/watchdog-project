#define _POSIX_C_SOURCE 200809L

#include "signal_manager.h"

#include <stdio.h>
#include <string.h>

volatile sig_atomic_t g_shutdown_flag = 0;
volatile sig_atomic_t g_status_report_flag = 0;

static void handle_shutdown_signal(int sig) {
    (void)sig;
    g_shutdown_flag = 1;
}

static void handle_status_signal(int sig) {
    (void)sig;
    g_status_report_flag = 1;
}

int setup_signal_handlers(void) {
    struct sigaction sa_shutdown;
    struct sigaction sa_status;

    memset(&sa_shutdown, 0, sizeof(sa_shutdown));
    sa_shutdown.sa_handler = handle_shutdown_signal;
    sigemptyset(&sa_shutdown.sa_mask);
    sa_shutdown.sa_flags = 0;

    if (sigaction(SIGINT, &sa_shutdown, NULL) == -1) {
        perror("sigaction SIGINT");
        return -1;
    }

    if (sigaction(SIGTERM, &sa_shutdown, NULL) == -1) {
        perror("sigaction SIGTERM");
        return -1;
    }

    memset(&sa_status, 0, sizeof(sa_status));
    sa_status.sa_handler = handle_status_signal;
    sigemptyset(&sa_status.sa_mask);
    sa_status.sa_flags = 0;

    if (sigaction(SIGUSR1, &sa_status, NULL) == -1) {
        perror("sigaction SIGUSR1");
        return -1;
    }

    return 0;
}
