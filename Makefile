
CC = gcc
CFLAGS = -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Wpedantic -Iinclude

SOURCES = src/main.c src/signal_manager.c src/process_manager.c src/logger.c src/status_report.c

all: watchdog workers/worker

watchdog: $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o watchdog


workers/worker: workers/worker.c
	$(CC) $(CFLAGS) workers/worker.c -o workers/worker

worker: workers/worker


test_logger: src/logger.c tests/test_logger.c include/logger.h
	$(CC) $(CFLAGS) src/logger.c tests/test_logger.c -o test_logger

test-logger: test_logger
	./test_logger

clean:
	rm -f watchdog test_logger workers/worker *.o

.PHONY: all clean test-logger worker
