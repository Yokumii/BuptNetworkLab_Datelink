CC=gcc
CFLAGS=-O2 -Wall

datalink: gobackN.o protocol.o lprintf.o crc32.o
	gcc gobackN.o protocol.o lprintf.o crc32.o -o gobackN -lm

clean:
	${RM} *.o gobackN *.log