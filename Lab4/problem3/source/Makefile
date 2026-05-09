all: forkjoin_demo

BKTPOOL_DIR ?= ../../problem1
CFLAGS = -pthread -fcommon -Wall -Wextra -I. -I$(BKTPOOL_DIR)
CC = gcc $(CFLAGS)

forkjoin_demo: example.o forkjoin.o bkwrk.o bktask.o bktpool.o
	$(CC) -o forkjoin_demo example.o forkjoin.o bkwrk.o bktask.o bktpool.o

example.o: example.c forkjoin.h
	$(CC) -c -o example.o example.c

forkjoin.o: forkjoin.c forkjoin.h
	$(CC) -c -o forkjoin.o forkjoin.c

bkwrk.o: $(BKTPOOL_DIR)/bkwrk.c $(BKTPOOL_DIR)/bktpool.h
	$(CC) -c -o bkwrk.o $(BKTPOOL_DIR)/bkwrk.c

bktask.o: $(BKTPOOL_DIR)/bktask.c $(BKTPOOL_DIR)/bktpool.h
	$(CC) -c -o bktask.o $(BKTPOOL_DIR)/bktask.c

bktpool.o: $(BKTPOOL_DIR)/bktpool.c $(BKTPOOL_DIR)/bktpool.h
	$(CC) -c -o bktpool.o $(BKTPOOL_DIR)/bktpool.c

clean:
	rm -f *.o forkjoin_demo
