CC=arm-linux-androideabi-gcc
CFLAGS=-O3 -Wall -fPIE
#CFLAGS=-Wall -g -ggdb -fPIE
LIBS=-llog -pthread
LDFLAGS=-fPIE -pie -shared

all: abootimg.o transmitter.o
	$(CC) $(LDFLAGS) -o libhookril.so hookril.o transmitter.o $(LIBS)

abootimg.o:
	$(CC) $(CFLAGS) -c -o hookril.o hookril.c -I./
transmitter.o:
	$(CC) $(CFLAGS) -c -o transmitter.o transmitter.c

clean:
	rm -f libhookril.so *.o

.PHONY:	clean all