CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2
PTHREAD_FLAGS = -pthread

.PHONY: all clean sum_multi-thread cpu-info

all: sum_serial sum_multi_thread

sum_serial: sum_serial.c
	$(CC) $(CFLAGS) -o $@ $<

sum_multi_thread: sum_multi_thread.c
	$(CC) $(CFLAGS) $(PTHREAD_FLAGS) -o $@ $<

# Alias target to match requirement naming in the handout.
sum_multi-thread: sum_multi_thread

cpu-info:
	@command -v lscpu >/dev/null 2>&1 && lscpu || echo "lscpu not found on this machine."

clean:
	rm -f sum_serial sum_multi_thread
