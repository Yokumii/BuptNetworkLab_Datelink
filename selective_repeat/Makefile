CC=gcc
CFLAGS=-O2 -Wall

datalink: selective_repeat.o protocol.o lprintf.o crc32.o
	gcc selective_repeat.o protocol.o lprintf.o crc32.o -o SR -lm

clean:
	${RM} *.o SR *.log