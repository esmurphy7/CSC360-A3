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
///////////////////////////////////////

///////////////////////////////////////
// Definitions
#define BLOCK_SIZE 				512			// Block size in bytes

// FAT entry values
#define FAT_ENTRY_SIZE 			4 			// FAT entry size in bytes 
#define FAT_ENTRIES_PER_BLOCK 	128			// BLOCK_SIZE/FAT_ENTRY_SIZE
// Status values for FAT entries
#define BLOCK_AVAILABLE 		0x00000000	// 0
#define BLOCK_RESERVED 			0x00000001	// 1
#define BLOCK_MIN_ALLOCATED 	0x00000002	// 2
#define BLOCK_MAX_ALLOCATED 	0xFFFFFF00	// 4294967040
#define BLOCK_END 				0xFFFFFFFF	//  4294967295

// Directory entry fields
#define DIR_ENTRY_SIZE 				64		
#define DIR_ENTRY_STATUS_SIZE  		 1
#define DIR_ENTRY_START_BLOCK_SIZE   4
#define DIR_ENTRY_NUM_BLOCKS_SIZE    4
#define DIR_ENTRY_FILE_SIZE_B_SIZE   4
#define DIR_ENTRY_CREATE_TIME_SIZE   7
#define DIR_ENTRY_MODIFY_TIME_SIZE   7
#define DIR_ENTRY_FILE_NAME_SIZE    31 
#define DIR_ENTRY_UNUSED_SIZE        6
// Offsets for directory entry time values
#define TIME_YEAR_SIZE 		2
#define TIME_MONTH_SIZE		1
#define TIME_DAY_SIZE 		1
#define TIME_HOUR_SIZE 		1
#define TIME_MINUTE_SIZE 	1
#define TIME_SECOND_SIZE 	1

struct Time
{
	unsigned char seconds;
	unsigned char minutes;
	unsigned char hour;
	unsigned char day;
	unsigned char month;
	short year;
};

struct dirEntry
{
	unsigned char status;
	uint32_t start_block;
	uint32_t num_blocks;
	uint32_t file_size;
	struct Time create_time;
	struct Time modify_time;
	char filename[DIR_ENTRY_FILE_NAME_SIZE];
};

struct FAT
{
	uint32_t start_block;
	uint32_t num_blocks;
	int free_blocks;
	int reserved_blocks;
	int allocated_blocks;
	int* entries;
};

struct FDT
{
	uint32_t start_block;
	uint32_t num_blocks;
	struct dirEntry* root;
};

struct fileSystem
{
	uint64_t id;
	uint16_t block_size;
	int num_blocks;
};
///////////////////////////////////////

///////////////////////////////////////
// Prototypes
void free_fileSystem();
void read_superblock(unsigned char*,int);
void read_FAT(unsigned char*,int);
void read_FDT(unsigned char*, int);
void copy_file(unsigned char*, char*,char*);
///////////////////////////////////////
