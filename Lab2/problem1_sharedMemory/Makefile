CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2

.PHONY: all clean

all: rating

rating: rating.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f rating
