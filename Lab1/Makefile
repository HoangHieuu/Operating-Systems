CC = gcc
CFLAGS = -Wall -g

all: calc

calc: calc.o math_logic.o
	$(CC) $(CFLAGS) -o calc calc.o math_logic.o -lm

calc.o: calc.c math_logic.h
	$(CC) $(CFLAGS) -c calc.c

math_logic.o: math_logic.c math_logic.h
	$(CC) $(CFLAGS) -c math_logic.c

.PHONY: clean
clean:
	rm -f calc *.o
