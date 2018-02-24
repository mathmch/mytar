CC = gcc

CCFLAGS = -Wall -pedantic -g

LD = gcc

LDFLAGS =

all: mytar

mytar: mytar.o acreate.o dirnode.o util.o
	$(LD) $(LDFLAGS) -o mytar mytar.o acreate.o dirnode.o util.o

mytar.o: mytar.c
	$(CC) $(CCFLAGS) -c -o mytar.o mytar.c

acreate.o: acreate.c
	$(CC) $(CCFLAGS) -c -o acreate.o acreate.c

dirnode.o: dirnode.c
	$(CC) $(CCFLAGS) -c -o dirnode.o dirnode.c

util.o: util.c
	$(CC) $(CCFLAGS) -c -o util.o util.c

clean:
	rm mytar.o acreate.o dirnode.o util.o
