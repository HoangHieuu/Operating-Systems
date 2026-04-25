CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread

all: periodic_detector

periodic_detector: periodic_detector.c
	$(CC) $(CFLAGS) periodic_detector.c -o periodic_detector

clean:
	rm -f periodic_detector

.PHONY: all clean
