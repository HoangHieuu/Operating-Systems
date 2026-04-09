CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -pthread

TARGETS = msgrcv msgsnd

all: $(TARGETS)

msgrcv: msgrcv.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

msgsnd: msgsnd.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGETS)

.PHONY: all clean
