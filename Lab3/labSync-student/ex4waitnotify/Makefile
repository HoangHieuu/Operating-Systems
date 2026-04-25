CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread

all: waitnotify

waitnotify: waitnotify.c
	$(CC) $(CFLAGS) waitnotify.c -o waitnotify

clean:
	rm -f waitnotify

.PHONY: all clean
