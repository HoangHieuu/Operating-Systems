CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread

all: lockfree_stack

lockfree_stack: lockfree_stack.c
	$(CC) $(CFLAGS) lockfree_stack.c -o lockfree_stack

clean:
	rm -f lockfree_stack

.PHONY: all clean
