CC = gcc
CFLAGS = -c -Wall
SOURCE = diskinfo.c disklist.c disk.c testmain.c
OBJECTS = diskinfo.o disklist.o testmain.o
PART1 = diskinfo
PART2 = disklist
TEST = testmain

all: part1 part2 test

part1: diskinfo.o disk.o
	$(CC) diskinfo.o disk.o -o $(PART1)

part2: disklist.o disk.o
	$(CC) disklist.o disk.o -o $(PART2)

test: testmain.o
	$(CC) testmain.o -o $(TEST)

source: $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE)

clean:
	rm *.o $(PART1) $(PART2) $(TEST)

