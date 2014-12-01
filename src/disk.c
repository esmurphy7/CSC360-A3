/*
*	Network order is Big Endian (BE)
*	Host order is Little Endian (LE)
*/

////////////////////////////////////////
// Headers
#include "disk.h"
////////////////////////////////////////

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
* Returns the directory entry in the FDT that has the filename
*/
struct dirEntry* findEntryInFDT(char* filename)
{
	struct dirEntry* entry = NULL;
	struct dirEntry cur_entry;

	int i;
	for(i=0; i<dir_entries; i++)
	{
		cur_entry = FDT->root[i];
		if((!strcmp(filename, cur_entry.filename)) &&
			dirEntryIsFile(cur_entry.status))
		{
			entry = &cur_entry;
			break;
		}
	}
	return entry;
}
/*
*	Return the id of the next FAT block. -1 On end of FAT
*/
int getNextFatBlock(int current_block)
{
	// for each FAT entry
	int i = 0;
	while(FAT->entries[i] != NULL)
	{
		// if it is reserved, return the entry id
		if(FAT->entries[i] == BLOCK_RESERVED) return i;
	}

    return -1;
}
/*
*	Searches the FAT for the next avaiable entry. 
*	Stores the entry offset in "position"
*/
int getNextFATEntry(int fp, int* position)
{
	unsigned char fat_entry[FAT_ENTRY_SIZE];
	int current_index;
    int current_block = FAT->start_block;
    int entryId = 0;
    int status, auxInt;
    int i, j;

    if (position == NULL)
    {
        return -1;
    }

    // for each FAT block
    for (i=0; i < FAT->num_blocks; i++)
    {
    	// go to next block
    	//current_index = current_block*BLOCK_SIZE;
    	lseek(fp, current_block*BLOCK_SIZE, SEEK_SET);

        /* Stop when no more blocks */
        if(current_block < 0)  
        {
            /* printf("No more FAT blocks\n"); */
            break;
        }

        /* we move to the start of next FAT block: N bytes = blockSize * currentBlock  */
        /* currentBlock = fat_start for the first block */
        /* currentBlock = block obtained after looking into FAT */

        /* Read al FAT entries for current block to obtain statistics (blocks used, reserved, free) */

        // for each FAT entry
        for (j=0; j < FAT_ENTRIES_PER_BLOCK; j++)
        {
        	

            /* we read FAT entries as follows: */
            /* 1) Read 4 bytes into array */
            /* 2) memcpy() 4 bytes into int */
            /* 3) Apply htonl() to int obtained  */
            read(fp, fat_entry, FAT_ENTRY_SIZE);
            memcpy(&auxInt, &fat_entry, FAT_ENTRY_SIZE);
            status = htonl(auxInt);

            if(status == BLOCK_AVAILABLE)
            {
                /* we return blockId = entryID and offset within file = position */
                //*position = (current_index - FAT_ENTRY_SIZE);
            	*position = ((lseek(fp, 0, SEEK_CUR)) - FAT_ENTRY_SIZE);

                return entryId;
            }

            // go to next FAT entry
        	//current_index += FAT_ENTRY_SIZE;

            entryId++;

        } /* End for(;;) FAT entries of this block */

        /* now move to next FAT block using current */
        current_block = getNextFatBlock(current_block);
    }

    /* when no FAT entry available - return -1 */
    return -1;
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

/*
* Free allocated memory
*/
void free_fileSystem()
{
	free(fileSystem);
}
void free_FAT()
{
	free(FAT);
}
void free_FDT()
{
	free(FDT);
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
	FAT->entries = (int*)malloc(sizeof(FAT->entries)*FAT->num_blocks*FAT_ENTRIES_PER_BLOCK);

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
	//printf("starting at block %d, current_index %d\n", current_block, current_block*BLOCK_SIZE);
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

			// read each field of the current dir entry into FDT's list of dir entries
			//printf("FDT->root[dir_entries].status: %.2x\n", FDT->root[dir_entries].status);
			//printf("map[current_index + offset]: %.2x\n", map[current_index + offset]);
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
			//printf("file_size %d\n", FDT->root[dir_entries].file_size);

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

/*
* Copy the file specified from the file system to the current directory by
*	reading then writing each block
*/
void get_file(unsigned char* map, char* imageFileName, char* outFileName)
{
    struct dirEntry* fileEntry = NULL; /* Entry at ROOT dir for requested file name */
    unsigned char buffer[BLOCK_SIZE];
    int fileBlock = -1;
    int fileSize = -1;
    int blocks2Read = 0;     /* Blocks to read completely */
    int remainingBytes = 0;  /* Bytes to read at last block */
    int wfp;        /* write file pointer - to write into current directory */
    int current_index;
 

	read_superblock(map, 0);
	read_FAT(map, 0);  
	read_FDT(map, 0);

	if((fileEntry = findEntryInFDT(outFileName)) == NULL)
	{
		printf("null file entry\n");
		exit(-1);
	}

	/* Get sizes and how many blocks we have to read */
    fileBlock = fileEntry->start_block;
    fileSize  = fileEntry->file_size;

    blocks2Read    = (fileSize / BLOCK_SIZE);
    remainingBytes = (fileSize % BLOCK_SIZE);

    // init buffer to all 0's
    memset(&buffer, 0, BLOCK_SIZE);

    // open the file to write to
    wfp = open(outFileName, O_RDWR | O_CREAT, S_IRUSR | S_IRGRP | S_IROTH);

    if (wfp < 0)
    {
        printf("ERROR: Could not open file <%s>\n", outFileName);
        exit(-1);
    }

    /* Read blocks that will read entire */
    int i;
    for (i=0; i < blocks2Read; i++)
    {
    	// go to next block
    	current_index = fileBlock*BLOCK_SIZE;

        /* Read full block */
        /* printf("Reading full block: %d\n", fileBlock); *//* debug */
        memcpy(&buffer, &map[current_index], BLOCK_SIZE);

        /* write it into output file */
        write(wfp, buffer, BLOCK_SIZE);

        /* obtain next block from FAT */
        fileBlock = FAT->entries[fileBlock];
    }

	if (remainingBytes > 0)
    {
        // go to beginning of remaining block
    	current_index = fileBlock*BLOCK_SIZE;

    	// read the remaining bytes
        memcpy(&buffer, &map[current_index], remainingBytes);

        // write remaining bytes to output file
        write(wfp, buffer, remainingBytes);
    }

    // close and free the file
    close(wfp);
    wfp = -1;
    
}
/*
* Copy file from current directory to file system
*/

void put_file(char* imageFileName, char *inFileName)
{
    unsigned char buffer[BLOCK_SIZE];
    int rootEntryPosition = -1;
    struct tm* timeInfo = NULL;
    struct stat st;
    time_t now;
    int fp;
    int rfp;
    int currentBlock = -1;
    int previousBlock = -1;
    int blocksRequired = 0;
    int totalBytesRead = 0;
    int bytesRead = 0;
    int auxShort = 0;
    int auxInt = 0;
    int auxWriteOffset = 0;
    int writeOffset = 0;
    int i;

    printf("opening infile and diskimage...\n");
    if (imageFileName == NULL)
    {
        printf("ERROR: Could not open NULL img file\n");
        exit(-1);
    }

    if (inFileName == NULL)
    {
        printf("ERROR: Could not open NULL input file\n");
        exit(-1);
    }

    if (access(inFileName, R_OK))
    {
        printf("File not found.\n");
        exit(-1);
    }


    // open the disk image file
    if((fp = open(imageFileName, O_RDWR)) < 0)
    {
        printf("ERROR: Could not open file <%s>\n", imageFileName);
        exit(-1);
    }

    // open the input file
    if((rfp = open(inFileName, O_RDONLY)) < 0)
    {
        printf("ERROR: Could not open file <%s>\n", inFileName);
        exit(-1);
    }

    if((fstat(fp, &st)) == 1)
	{
		perror("fstat()\n");
		exit(-1);
	}

    // get the disk as a map
	unsigned char* map = mmap((caddr_t)0, 
				st.st_size,
				 PROT_READ, 
				 MAP_SHARED, 
				 fp, 
				 0);
	if(map == (caddr_t)-1)
	{
		perror("mmap()\n");
		exit(-1);
	}

	// read evreything
	printf("reading superblock...\n");
    read_superblock(map, 0);
    printf("reading FAT...\n");
    read_FAT(map, 0);  
    printf("reading FDT...\n");
    read_FDT(map, 0);

    printf("initing buffer...\n");
    // init buffer to 0's
    memset(&buffer, 0, BLOCK_SIZE);

    // make sure file system isn't already full
    if (firstRootEntryFree == -1 )
    {
        printf("ERROR: Could not add file <%s>, filesystem is full\n", inFileName);
        exit(-1);
    }

    // find the entry point
    rootEntryPosition = (firstRootEntryFreeBlock * BLOCK_SIZE) + 
                        (firstRootEntryFree * DIR_ENTRY_SIZE);
    
    // get next block of diskimage
    if ((currentBlock = getNextFATEntry(fp, &writeOffset)) == -1 )
    {
        printf("ERROR: Could not add file <%s>, filesystem is full\n", inFileName);
        exit(-1);
    }

    printf("reading start block of diskimage...\n");
    // go to diskimage entry point
    lseek(fp , rootEntryPosition , SEEK_SET);

    // set status byte
    buffer[0] = (0x01 | 0x02); 
    // write status field to disk image
    write(fp, buffer, DIR_ENTRY_STATUS_SIZE);

    // read start block size field
    auxInt  = htonl(currentBlock);
    memcpy(&buffer[0], &auxInt, DIR_ENTRY_START_BLOCK_SIZE);
    // write start block size field to disk image
    write(fp, buffer, DIR_ENTRY_START_BLOCK_SIZE);


    printf("reading block number of %s...\n", inFileName);
    // determine number of blocks required for the infile
    blocksRequired = st.st_size / BLOCK_SIZE;
    // add an extra for remaining blocks
    if(st.st_size % BLOCK_SIZE) blocksRequired++;

    // read the number of blocks field
    auxInt  = htonl(blocksRequired);
    memcpy(&buffer[0], &auxInt, DIR_ENTRY_NUM_BLOCKS_SIZE);
    // write the number of blocks field
    write(fp, buffer, DIR_ENTRY_NUM_BLOCKS_SIZE);

    // read the file size field
    auxInt = htonl(st.st_size);
    memcpy(&buffer[0], &auxInt, DIR_ENTRY_FILE_SIZE_B_SIZE);
    // write the file size filed
    write(fp, buffer, DIR_ENTRY_FILE_SIZE_B_SIZE);

    printf("getting time info...\n");
    // get the time of infile's creation
    now = time(NULL);
    printf("got current time\n");
    timeInfo = gmtime(&now);
    printf("stored current time\n");
    auxShort = timeInfo->tm_year + 1900; 
    auxShort = htons(auxShort);

    // store the creation time data in time struct
    int offset = 0;
    // store year
    memcpy(&buffer[offset], &auxShort, sizeof(short));
    offset += TIME_YEAR_SIZE;

    // store month
    buffer[offset] = timeInfo->tm_mon + 1; 
    offset += TIME_MONTH_SIZE;

    // store day
    buffer[offset] = timeInfo->tm_mday;
    offset += TIME_DAY_SIZE;

    // store hour
    buffer[offset] = timeInfo->tm_hour;
    offset += TIME_HOUR_SIZE;

    // enable daylight savings time
    if(!timeInfo->tm_isdst) buffer[offset]++; 

    // store minute
    buffer[offset] = timeInfo->tm_min;
    offset += TIME_MINUTE_SIZE;

    // store second 
    buffer[offset] = timeInfo->tm_sec;
    offset += TIME_SECOND_SIZE;

    printf("writing time info...\n");
    // write the creation time data to the diskimage
    write(fp, buffer, DIR_ENTRY_CREATE_TIME_SIZE);
    
    // write the modify time (same as creaion time) to the diskimage
    write(fp, buffer, DIR_ENTRY_CREATE_TIME_SIZE);
   
    // read filename field
    memset(&buffer, 0, DIR_ENTRY_FILE_NAME_SIZE);
    memcpy(&buffer, inFileName, strlen(inFileName));
    // write filename field to diskimage
    write(fp, buffer, DIR_ENTRY_FILE_NAME_SIZE);
    
    // ignore unused bytes
    memset(&buffer, 0xFF, DIR_ENTRY_UNUSED_SIZE);

    // go to beginning of input file
    lseek(rfp, 0, SEEK_SET);
    
    for (i=0; i < blocksRequired; i++)
    {
    	// go to next block in disk image
    	lseek(fp, currentBlock*BLOCK_SIZE, SEEK_SET);

    	// read a block from input file
        //bytesRead = fread(&buffer[0], 1, myFileSystem.blockSize, rfp);
        bytesRead = read(rfp, buffer, BLOCK_SIZE);
        totalBytesRead += bytesRead;

        // write the block to diskimage
        write(fp, buffer, bytesRead);

        // if this is the last FAT entry
        if(totalBytesRead >= st.st_size)
        {
        	// update the FAT entry
            FAT->entries[currentBlock] = BLOCK_END;

            // go to write offset
            lseek(fp , writeOffset, SEEK_SET);
            // read the last entry
            memcpy(&buffer, &FAT->entries[currentBlock], FAT_ENTRY_SIZE);
            // write the last entry to the diskimage
            write(fp, buffer, FAT_ENTRY_SIZE);
        }
        else
        {
        	// update the FAT entry
            previousBlock  = currentBlock;
            auxWriteOffset = writeOffset;

            // go to offset in diskimage
            lseek(fp , auxWriteOffset, SEEK_SET);
            auxInt = htonl(currentBlock);
            // read the FAT entry
            memcpy(&buffer[0], &auxInt, FAT_ENTRY_SIZE);
            // write the entry to the diskimage
            write(fp, buffer, FAT_ENTRY_SIZE);

            // get the next block
            currentBlock = getNextFATEntry(fp, &writeOffset);

            if (currentBlock == -1 )
            {
                printf("ERROR: Could not add file <%s>, filesystem is full\n", inFileName);
                exit(-1);
            }

            // go to offset in diskimage
            lseek(fp , auxWriteOffset, SEEK_SET);
            auxInt = htonl(currentBlock);
            // read the entry
            memcpy(&buffer[0], &auxInt, FAT_ENTRY_SIZE);
            // write the entry to the diskaimge
            write(fp, buffer, FAT_ENTRY_SIZE);
            
            // update the FAT entries
            FAT->entries[previousBlock] = currentBlock;
        }
    }



}

////////////////////////////////////////