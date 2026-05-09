CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread

all: seqlock

seqlock: seqlock.c seqlock.h
	$(CC) $(CFLAGS) seqlock.c -o seqlock

clean:
	rm -f seqlock

.PHONY: all clean
