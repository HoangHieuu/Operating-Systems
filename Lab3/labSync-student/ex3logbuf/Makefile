CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread

all: logbuf

logbuf: logbuf.c
	$(CC) $(CFLAGS) logbuf.c -o logbuf

clean:
	rm -f logbuf

.PHONY: all clean
