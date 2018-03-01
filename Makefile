CC = gcc

CCFLAGS = -Wall -pedantic -g

LD = gcc

LDFLAGS =

all: mytar

mytar: mytar.o acreate.o aextract.o  dirnode.o util.o header.o listing.o
	$(LD) $(LDFLAGS) -o mytar mytar.o acreate.o aextract.o dirnode.o util.o header.o listing.o

mytar.o: mytar.c
	$(CC) $(CCFLAGS) -c -o mytar.o mytar.c

acreate.o: acreate.c
	$(CC) $(CCFLAGS) -c -o acreate.o acreate.c

aextract.o: aextract.c
	$(CC) $(CCFLAGS) -c -o aextract.o aextract.c

dirnode.o: dirnode.c
	$(CC) $(CCFLAGS) -c -o dirnode.o dirnode.c

util.o: util.c
	$(CC) $(CCFLAGS) -c -o util.o util.c

header.o: header.c
	$(CC) $(CCFLAGS) -c -o header.o header.c

listing.o: listing.c
	$(CC) $(CCFLAGS) -c -o listing.o listing.c

clean:
	rm mytar.o aextract.o acreate.o dirnode.o util.o header.o listing.o
