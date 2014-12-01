///////////////////////////////////////
// Headers
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <endian.h>
#include <unistd.h>
#include <time.h>
///////////////////////////////////////

///////////////////////////////////////
// Prototypes
void free_fileSystem();
void read_superblock(unsigned char*,int);
void read_FAT(unsigned char*,int);
void read_FDT(unsigned char*, int);
void get_file(unsigned char*, char*,char*);
void put_file(char*, char*);
///////////////////////////////////////
