CC = gcc
CFLAGS = -Wall -Wextra -O2

all: writer reader

writer: writer.c
	$(CC) $(CFLAGS) writer.c -o writer

reader: reader.c
	$(CC) $(CFLAGS) reader.c -o reader

clean:
	rm -f writer reader mmap_shared.bin
