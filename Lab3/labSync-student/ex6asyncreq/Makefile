CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread

all: async_resources

async_resources: async_resources.c
	$(CC) $(CFLAGS) async_resources.c -o async_resources

clean:
	rm -f async_resources

.PHONY: all clean
