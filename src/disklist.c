/* Part2: Print the root directory and its containing file's information
*/

//////////////////////////////////////////
// Headers
#include "disk.h"
//////////////////////////////////////////


//////////////////////////////////////////
// Prototypes

//////////////////////////////////////////


//////////////////////////////////////////
// Globals

//////////////////////////////////////////


//////////////////////////////////////////
// Functions
int main(int argc, char* argv[])
{
	char* diskimg;			// Filename of disk image
	int fd;					// File descriptor of disk image
	struct stat fileStats;	// Statistics of disk image
	unsigned char* map;		// Map of the disk image as array of bytes

	if(argc != 2)
	{
		printf("Usage: $./disklist <disk.img>");
		exit(-1);
	}

	diskimg = argv[1];

	// Open image file
	if((fd = open(diskimg, O_RDONLY)) == -1)
	{
		perror("fopen()\n");
		exit(-1);
	}

	// Insert information on the disk image into the stats struct
	if((fstat(fd, &fileStats)) == 1)
	{
		perror("fstat()\n");
		exit(-1);
	}
	//printf("%s file size: %d bytes\n",diskimg,fileStats.st_size);
	
	// Insert the bytes of the disk image into the map
	if((map = mmap((caddr_t)0, 
					fileStats.st_size,
					 PROT_READ, 
					 MAP_SHARED, 
					 fd, 
					 0)) == (caddr_t)-1)
	{
		perror("mmap()\n");
		exit(-1);
	}


	// Read the superblock
	read_superblock(map, 0);

	// Traverse the FAT
	read_FAT(map, 0);

	// traverse the root (FDT) and print its information
	read_FDT(map, 1);

	close(fd);
	
	// free the file system after usage
	free_fileSystem();

	return 0;
}


//////////////////////////////////////////
