/* Part3: Copy a file from the file system to the current directory
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
	char* target_filename;	// Filename of file to be sent
	int fd;					// File descriptor of disk image
	struct stat fileStats;	// Statistics of disk image
	unsigned char* map;		// Map of the disk image as array of bytes

	if(argc != 3)
	{
		printf("Usage: $./diskget <disk.img> <copyfilename>\n");
		exit(-1);
	}

	diskimg = argv[1];
	target_filename = argv[2];

	// Open image file
	if((fd = open(diskimg, O_RDONLY)) == NULL)
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

	// Since the file is stored, close the file
	close(fd);

	// copy the file to the current directory
	printf("copying %s from %s...\n", target_filename, diskimg);
	copy_file(map, diskimg, target_filename);

	return 0;
}


//////////////////////////////////////////

