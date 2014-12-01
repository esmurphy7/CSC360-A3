CC = gcc
CFLAGS = -c -Wall
SOURCE = diskinfo.c disklist.c diskget.c diskput.c disk.c testmain.c
OBJECTS = diskinfo.o disklist.o diskget.o diskput.o testmain.o
PART1 = diskinfo
PART2 = disklist
PART3 = diskget
PART4 = diskput
TEST = testmain

all: part1 part2 part3 part4 test

part1: diskinfo.o disk.o
	$(CC) diskinfo.o disk.o -o $(PART1)

part2: disklist.o disk.o
	$(CC) disklist.o disk.o -o $(PART2)

part3: diskget.o disk.o
	$(CC) diskget.o disk.o -o $(PART3)

part4: diskput.o disk.o
	$(CC) diskput.o disk.o -o $(PART4)

test: testmain.o
	$(CC) testmain.o -o $(TEST)

source: $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE)

clean:
	rm *.o $(PART1) $(PART2) $(PART3) $(PART4) $(TEST)

