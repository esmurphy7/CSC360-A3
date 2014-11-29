/*
*	Network order is Big Endian (BE)
*	Host order is Little Endian (LE)
*/

////////////////////////////////////////
// Headers
#include "disk.h"
////////////////////////////////////////

////////////////////////////////////////
// Globals
struct FAT* FAT;
struct FDT* FDT;
struct fileSystem* fileSystem;
////////////////////////////////////////

////////////////////////////////////////
// Functions

/* Store the superblock's fields into the correct struct, 
* printing the values along the way
*/
void read_superblock(unsigned char* map)
{

	unsigned char* superblock;
	int offset = 8;

	// Allocate memory for the file system structs
	FAT = (struct FAT*)malloc(sizeof(FAT));
	FDT = (struct FDT*)malloc(sizeof(FDT));
	fileSystem = (struct fileSystem*)malloc(sizeof(fileSystem));

	// Allocate 512 bytes to superblock
	superblock = (unsigned char*)calloc(BLOCK_SIZE, 1);
	// Read 512 bytes into the superblock
	memcpy(superblock, map, BLOCK_SIZE);

	printf("Super block information:\n");

	// Copy bytes 8 & 9 as the block size field
	memcpy(&fileSystem->block_size, &superblock[offset], 2);
	fileSystem->block_size = be16toh(fileSystem->block_size);
	printf("Block size: %d\n", fileSystem->block_size);
	offset += 2;

	// Copy bytes 10-13 as number of blocks in file system
	memcpy(&fileSystem->num_blocks, &superblock[offset], 4);
	fileSystem->num_blocks = be32toh(fileSystem->num_blocks);
	printf("Block count: %d\n", fileSystem->num_blocks);
	offset += 4;

	// Copy bytes 14-17 as FAT start block
	memcpy(&FAT->start_block, &superblock[offset], 4);
	FAT->start_block = be32toh(FAT->start_block);
	printf("FAT starts: %d\n", FAT->start_block);
	offset += 4;

	// Copy bytes 18-21 as number of blocks in FAT
	memcpy(&FAT->num_blocks, &superblock[offset], 4);
	FAT->num_blocks = be32toh(FAT->num_blocks);
	printf("FAT blocks: %d\n",FAT->num_blocks);
	offset += 4;

	// Copy bytes 22-25 as FDT start block
	memcpy(&FDT->start_block, &superblock[offset], 4);
	FDT->start_block = be32toh(FDT->start_block);
	printf("Root directory start: %d\n", FDT->start_block);	
	offset += 4;

	// Copy bytes 26-29 as number of blocks in FDT
	memcpy(&FDT->num_blocks, &superblock[offset], 4);
	FDT->num_blocks = be32toh(FDT->num_blocks);
	printf("Root directory blocks: %d\n", FDT->num_blocks);
	offset += 4;

	// Store the FAT and FDT structs in the fileSystem struct
	fileSystem->FAT = *FAT;
	fileSystem->FDT = *FDT;

}

/*
* Traverse the FAT and record entry statistics
*/
void read_FAT(unsigned char* map)
{
	int current_block;
	int current_entry;
	int current_index;
	int end_index;
	uint32_t status;
	int i,j;

	current_block = FAT->start_block;
	end_index = (FAT->start_block + FAT->num_blocks)*BLOCK_SIZE;

	// for every FAT block
	for(i=0; i < FAT->num_blocks; i++)
	{
		current_index = current_block*BLOCK_SIZE;
		current_entry = 0;

		if(current_index == end_index)
			break;
		// for every entry in the FAT block
		for(j=0; j< FAT_ENTRIES_PER_BLOCK; j++)
		{
			current_index += current_entry*FAT_ENTRY_SIZE;

			// read the entry's first 4 bytes as status value
			memcpy(&status, &map[current_index], FAT_ENTRY_SIZE);
			status = be32toh(status);
			printf("Block %d, Entry %d, Index %d\n\tStatus: %.4x\n", current_block, current_entry, current_index, status);
			// incremenet FAT statistics based on status found
			switch(status)
			{
				case BLOCK_AVAILABLE:
					FAT->free_blocks++;
					break;
				case BLOCK_RESERVED:					
					FAT->reserved_blocks++;
					break;
				default:
					FAT->allocated_blocks++;
					break;
			}

			current_entry++;
		}

		current_block++;
	}

	// Print FAT statistics
	printf("FAT information:\n");
    printf("Free Blocks: %d\n", FAT->free_blocks);
    printf("Reserved Blocks: %d\n", FAT->reserved_blocks);
    printf("Allocated Blocks: %d\n", FAT->allocated_blocks);
}
////////////////////////////////////////