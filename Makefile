CC=gcc
CFLAGS=-c -Wall -Werror

all: fscheck

fscheck: fscheck.o
	$(CC) fscheck.o -o fscheck

fscheck.o: fscheck.c
	$(CC) $(CFLAGS) fscheck.c

clean:
	rm *o fscheck
