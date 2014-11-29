///////////////////////////////////////
// Headers
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <endian.h>
///////////////////////////////////////

///////////////////////////////////////
// Definitions
#define BLOCK_SIZE 512					// Block size in bytes
#define DIR_ENTRY_SIZE 64				// Directory entry size in bytes
#define FAT_ENTRY_SIZE 4 				// FAT entry size in bytes 
#define FAT_ENTRIES_PER_BLOCK 128		// BLOCK_SIZE/FAT_ENTRY_SIZE

// Special hex values for FAT entries
#define BLOCK_AVAILABLE 0x00000000		// 0
#define BLOCK_RESERVED 0x00000001		// 1
#define BLOCK_MIN_ALLOCATED 0x00000002	// 2
#define BLOCK_MAX_ALLOCATED 0xFFFFFF00	// 4294967040
#define BLOCK_END 0xFFFFFFFF			//  4294967295

struct FAT
{
	uint32_t start_block;
	uint32_t num_blocks;
	int free_blocks;
	int reserved_blocks;
	int allocated_blocks;
};

struct FDT
{
	uint32_t start_block;
	uint32_t num_blocks;
};

struct fileSystem
{
	uint64_t id;
	uint16_t block_size;
	uint32_t num_blocks;
	struct FAT FAT;
	struct FDT FDT;
};
///////////////////////////////////////

///////////////////////////////////////
// Prototypes
void read_superblock(unsigned char*,int);
void read_FAT(unsigned char*,int);
///////////////////////////////////////
