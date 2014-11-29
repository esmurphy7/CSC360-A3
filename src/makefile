CC = gcc
CFLAGS = -c -Wall
SOURCE = diskinfo.c disk.c testmain.c
OBJECTS = diskinfo.o testmain.o
PART1 = diskinfo
TEST = testmain

all: part1 test

part1: diskinfo.o disk.o
	$(CC) diskinfo.o disk.o -o $(PART1)

test: testmain.o
	$(CC) testmain.o -o $(TEST)

source: $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE)

clean:
	rm *.o $(PART1) $(TEST)

