
CC = gcc
CFLAGS = -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Wpedantic -Iinclude

SOURCES = src/main.c src/signal_manager.c src/process_manager.c src/logger.c

all: watchdog

watchdog: $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o watchdog

test_logger: src/logger.c tests/test_logger.c include/logger.h
	$(CC) $(CFLAGS) src/logger.c tests/test_logger.c -o test_logger

test-logger: test_logger
	./test_logger

clean:
	rm -f watchdog test_logger *.o

.PHONY: all clean test-logger
