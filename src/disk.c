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
int dir_entries = 0;
int  firstRootEntryFree = -1;
int  firstRootEntryFreeBlock = -1;
struct fileSystem* fileSystem;
////////////////////////////////////////

////////////////////////////////////////
// Functions

/*
*	Determine if given status value means directory entry is in use
*/
bool dirEntryIsUsed(unsigned char status)
{
	// bit mask
	int used_mask = 0x01;
	return (status & used_mask)? true : false;
}

/*
*	Determine if given status value means directory entry is a file
*/
bool dirEntryIsFile(unsigned char status)
{
	// bit mask
	int file_mask = 0x02;
	return (status & file_mask)? true : false;
}

/*
* Initialize a Time struct with the bytes in buffer
*/
void init_timeStruct(struct Time* timeP, unsigned char* buffer, int bufsize)
{
	int offset = 0;


	struct Time timeStruct = *timeP;

	// Ensure that buffer is big enough to qualify as a directory entry
    if ( (buffer != NULL) && (bufsize >= DIR_ENTRY_MODIFY_TIME_SIZE))
    {
        // Read Time struct fields 
        // year
        memcpy(&timeStruct.year, &buffer[offset], TIME_YEAR_SIZE);
        timeStruct.year = htons(timeStruct.year);
        offset += TIME_YEAR_SIZE;
        //printf("\nyear = %d\n", timeStruct.year);
        
        // month
        timeStruct.month = buffer[offset];
        offset += TIME_MONTH_SIZE;
        //printf("month = %d\n", timeStruct.month);

        // day
        timeStruct.day = buffer[offset];
        offset += TIME_DAY_SIZE;
        //printf("day = %d\n", timeStruct.day);

        // hour
        timeStruct.hour = buffer[offset];
        offset += TIME_HOUR_SIZE;  
       //printf("hour = %d\n", timeStruct.hour);

        // minute
        timeStruct.minutes = buffer[offset];
        offset += TIME_MINUTE_SIZE;
        //printf("minutes = %d\n", timeStruct.minutes);

        // second
        timeStruct.seconds = buffer[offset];
        offset += TIME_SECOND_SIZE;
        //printf("seconds = %d\n", timeStruct.seconds);
    }
    else
    {
        printf("ERROR: Invalid time buffer info\n");
        exit(-1);
    }

    *timeP = timeStruct;
}

/* Store the superblock's fields into the correct struct, 
* then print the values if flagged to do so
*/
void read_superblock(unsigned char* map, int print)
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

	// Copy bytes 8 & 9 as the block size field
	memcpy(&fileSystem->block_size, &superblock[offset], 2);
	fileSystem->block_size = htons(fileSystem->block_size);
	offset += 2;

	// Copy bytes 10-13 as number of blocks in file system
	memcpy(&fileSystem->num_blocks, &superblock[offset], 4);
	fileSystem->num_blocks = htonl(fileSystem->num_blocks);
	offset += 4;

	// Copy bytes 14-17 as FAT start block
	memcpy(&FAT->start_block, &superblock[offset], 4);
	FAT->start_block = htonl(FAT->start_block);
	offset += 4;

	// Copy bytes 18-21 as number of blocks in FAT
	memcpy(&FAT->num_blocks, &superblock[offset], 4);
	FAT->num_blocks = htonl(FAT->num_blocks);
	offset += 4;

	// Copy bytes 22-25 as FDT start block
	memcpy(&FDT->start_block, &superblock[offset], 4);
	FDT->start_block = htonl(FDT->start_block);
	offset += 4;

	// Copy bytes 26-29 as number of blocks in FDT
	memcpy(&FDT->num_blocks, &superblock[offset], 4);
	FDT->num_blocks = htonl(FDT->num_blocks);
	offset += 4;

	// Store the FAT and FDT structs in the fileSystem struct
	fileSystem->FAT = *FAT;
	fileSystem->FDT = *FDT;

	if(print)
	{
		printf("\nSuper block information:\n");
		printf("Block size: %d\n", fileSystem->block_size);
		printf("Block count: %d\n", fileSystem->num_blocks);
		printf("FAT starts: %d\n", FAT->start_block);
		printf("FAT blocks: %d\n",FAT->num_blocks);
		printf("Root directory start: %d\n", FDT->start_block);	
		printf("Root directory blocks: %d\n", FDT->num_blocks);
	}

}

/*
* Traverse the FAT and record entry statistics if flagged to do so
*/
void read_FAT(unsigned char* map, int print)
{
	int current_block;
	int current_index;
	int end_index;
	int status;
	int i,j;

	// Initialize the pointer for the FAT's list of entries
	FAT->entries = malloc(sizeof(int)*FAT->num_blocks*FAT_ENTRIES_PER_BLOCK);

	current_block = FAT->start_block;
	end_index = (FAT->start_block + FAT->num_blocks)*BLOCK_SIZE;

	// for every FAT block
	for(i=0; i < FAT->num_blocks; i++)
	{
		current_index = current_block*BLOCK_SIZE;
		//printf("Block %d\n", current_block);
		// Double check to make sure we're still indexing the FAT table
		if(current_index == end_index)
			break;
		// for every entry in the FAT block
		for(j=0; j< FAT_ENTRIES_PER_BLOCK; j++)
		{
			// read the entry's first 4 bytes as status value
			memcpy(&status, &map[current_index], FAT_ENTRY_SIZE);
			status = htonl(status);
			//printf("\tIndex: %d Status: %.4x\n",current_index,status);

			// increment FAT statistics based on status found
			if(status == BLOCK_AVAILABLE)
			{
				//printf("index %d is free\n",current_index);
				FAT->free_blocks++;
			}
			else if(status == BLOCK_RESERVED)
			{
				//printf("index %d is reserved\n",current_index);
				FAT->reserved_blocks++;
			}
			else
			{
				//printf("index %d is allocated\n",current_index);
				FAT->allocated_blocks++;
			}

			// Store the entry for later use
			FAT->entries[i*FAT_ENTRIES_PER_BLOCK+j] = status;

			// Go to next entry
			current_index += FAT_ENTRY_SIZE;
		}

		current_block++;
	}

	// Print FAT statistics if flagged to do so
	if(print)
	{
		printf("\nFAT information:\n");
	    printf("Free Blocks: %d\n", FAT->free_blocks);
	    printf("Reserved Blocks: %d\n", FAT->reserved_blocks);
	    printf("Allocated Blocks: %d\n", FAT->allocated_blocks);
	}
}

/*
* Read the root directory as a file data table (FDT) 
* and print its information if flagged to do so
*/
void read_FDT(unsigned char* map, int print)
{
	int current_block;
	int current_index;
	int end_index;
	int entries_per_block;
	int num_entries;
	int offset;				// use this to jump to each field in the entry
	int i,j;

	current_block = FDT->start_block;
	printf("starting at block %d, current_index %d\n", current_block, current_block*BLOCK_SIZE);
	end_index = (FDT->start_block + FDT->num_blocks)*BLOCK_SIZE;
	entries_per_block = BLOCK_SIZE / DIR_ENTRY_SIZE;
	num_entries = entries_per_block * FDT->num_blocks;

	// Allocate memory for the list of root entries
	FDT->root = (struct dirEntry*)malloc(sizeof(FDT->root)*num_entries);

	// for each block in the FDT
	for(i=0; i < FDT->num_blocks; i++)
	{
		current_index = current_block*BLOCK_SIZE;
		// ensure current index is not at the end
		if(current_index == end_index)
			break;

		// for each entry in the current FDT block
		for(j=0; j < entries_per_block; j++)
		{
			// reset the offset for new entries
			offset = 0;
			//printf("current_index: %d\n", current_index);

			// read each field of the current dir entry into FDT's list of dir entries

			// read status field
			memcpy(&FDT->root[dir_entries].status, &map[current_index + offset], DIR_ENTRY_STATUS_SIZE);
			//printf("status = %.2x\n",FDT->root[dir_entries].status);
			offset += DIR_ENTRY_STATUS_SIZE;

			// read starting block field
			memcpy(&FDT->root[dir_entries].start_block, &map[current_index + offset], DIR_ENTRY_START_BLOCK_SIZE);
			FDT->root[dir_entries].start_block = htonl(FDT->root[dir_entries].start_block);
			offset += DIR_ENTRY_START_BLOCK_SIZE;
			//printf("start block %10d\n", FDT->root[dir_entries].start_block);

			// read number of blocks field
			memcpy(&FDT->root[dir_entries].num_blocks, &map[current_index + offset], DIR_ENTRY_NUM_BLOCKS_SIZE);
			FDT->root[dir_entries].num_blocks = htonl(FDT->root[dir_entries].num_blocks);
			offset += DIR_ENTRY_NUM_BLOCKS_SIZE;
			//printf("num_blocks %10d\n", FDT->root[dir_entries].num_blocks);

			// read file size field
			memcpy(&FDT->root[dir_entries].file_size, &map[current_index + offset], DIR_ENTRY_FILE_SIZE_B_SIZE);
			FDT->root[dir_entries].file_size = htonl(FDT->root[dir_entries].file_size);
			offset += DIR_ENTRY_FILE_SIZE_B_SIZE;
			//printf("file_size %10d\n", FDT->root[dir_entries].file_size);

			// read create time field
			// buffer to hold time fields
			unsigned char buffer[DIR_ENTRY_FILE_NAME_SIZE+1];
			memcpy(&buffer, &map[current_index + offset], DIR_ENTRY_CREATE_TIME_SIZE);
			init_timeStruct(&FDT->root[dir_entries].create_time, buffer, DIR_ENTRY_CREATE_TIME_SIZE);
			offset += DIR_ENTRY_CREATE_TIME_SIZE;
			//printf("create_time %d\n", FDT->root[dir_entries].create_time);

			// read modify time field
			memcpy(&buffer, &map[current_index + offset], DIR_ENTRY_MODIFY_TIME_SIZE);
			init_timeStruct(&FDT->root[dir_entries].modify_time, buffer, DIR_ENTRY_MODIFY_TIME_SIZE);
			offset += DIR_ENTRY_MODIFY_TIME_SIZE;

			// read filename field
			memcpy(&FDT->root[dir_entries].filename, &map[current_index + offset], DIR_ENTRY_FILE_NAME_SIZE);
			offset += DIR_ENTRY_FILE_NAME_SIZE;

			if(j<5 && i<2)
			{
				//printf("dir entry %d %s use\n", dir_entries, dirEntryIsUsed(FDT->root[dir_entries].status)?"in" : "not in");
			}
			/* update first entry available to add new files in the future */
            if (( !dirEntryIsUsed(FDT->root[dir_entries].status) ) && (firstRootEntryFree == -1) )
            {
                firstRootEntryFree = j;
                firstRootEntryFreeBlock = current_block;
            }

			// print the fields read if flagged
			if (print)
            {
                // print information on entries in use
                if( dirEntryIsUsed(FDT->root[dir_entries].status) )
                {
                    printf("%c %10d %30s %4d/%02d/%02d %02d:%02d:%02d\n",
                           dirEntryIsFile(FDT->root[dir_entries].status)?'F':'D',
                           FDT->root[dir_entries].file_size,
                           FDT->root[dir_entries].filename,
                           FDT->root[dir_entries].modify_time.year,
                           FDT->root[dir_entries].modify_time.month,
                           FDT->root[dir_entries].modify_time.day,
                           FDT->root[dir_entries].modify_time.hour,
                           FDT->root[dir_entries].modify_time.minutes,
                           FDT->root[dir_entries].modify_time.seconds);
                }
            }

            // go to next entry
			current_index += DIR_ENTRY_SIZE;
			// increase number of directory entries read so far
			dir_entries++;
		}

		// go to next block
		current_block++;
	}

}
////////////////////////////////////////