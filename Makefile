CC=arm-linux-androideabi-gcc
CFLAGS=-O3 -Wall -fPIE
#CFLAGS=-Wall -g -ggdb -DHAS_BLKID
LIBS= -llog -fPIE -pie -shared

all: abootimg.o
	$(CC) $(LDLAGS) -o libhookril.so hookril.o $(LIBS)

abootimg.o:
	$(CC) $(CFLAGS) -c -o hookril.o hookril.c -I./

clean:
	rm -f libhookril.so *.o

.PHONY:	clean all