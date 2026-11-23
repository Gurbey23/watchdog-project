
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    const char *mode = "loop";

    if (argc > 1) {
        mode = argv[1];
    }

    printf("Worker started. PID: %ld, Mode: %s\n",
           (long)getpid(), mode);

    if (strcmp(mode, "normal") == 0) {
        sleep(2);
        return EXIT_SUCCESS;
    }

    if (strcmp(mode, "error") == 0) {
        sleep(2);
        return EXIT_FAILURE;
    }

    if (strcmp(mode, "loop") == 0) {
        while (1) {
            pause();
        }
    }

    fprintf(stderr, "Unknown worker mode: %s\n", mode);
    return EXIT_FAILURE;
}
