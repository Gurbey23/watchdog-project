#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static volatile sig_atomic_t g_keep_running = 1;

static void handle_term(int sig) {
    (void)sig;
    g_keep_running = 0;
}

int main(int argc, char *argv[]) {
    struct sigaction sa;
    const char *mode = (argc > 1) ? argv[1] : "daemon";

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_term;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    if (strcmp(mode, "normal") == 0) {
        int remaining = 15;
        while (g_keep_running && remaining > 0) {
            sleep(1);
            remaining--;
        }
        return EXIT_SUCCESS;
    }

    if (strcmp(mode, "crash") == 0) {
        sleep(3);
        return EXIT_FAILURE;
    }

    while (g_keep_running) {
        sleep(1);
    }

    return EXIT_SUCCESS;
}