CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2
TARGET = mmap

.PHONY: all clean

all: $(TARGET)

$(TARGET): mmap.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(TARGET) mmap.bin
