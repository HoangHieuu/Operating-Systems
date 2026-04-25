CPP = g++
CC = gcc
LD = ld
AR = ar
GFLAGS = -std=c++11 -g -pthread -I./
CFLAGS = -g -pthread -I./
LFLAGS = -g -pthread -I./
ARFLAGS = rcs

COBJECT = main utils
CEXEC = aggsum
LIBS =

COBJECTO = $(addsuffix .o,$(COBJECT))
CLIBS = $(addprefix lib,$(addsuffix .a,$(LIBS)))

%.o: %.cpp
	$(CPP) $(GFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

lib%.a: %.o
	$(AR) $(ARFLAGS) $@ $^

all: $(COBJECTO) $(CLIBS)
	$(CC) $(CFLAGS) $(COBJECTO) -o $(CEXEC) -pthread

clean:
	rm -f *.o $(CEXEC)

.PHONY: all clean
