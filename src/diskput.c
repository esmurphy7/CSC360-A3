
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disk.h"

void showUsage(char *programName)
{
    printf("USAGE: %s imageFileName putFileName\n"
           "Where:\n"
           "\timageFileName : Disk image file\n"
           "\tputFileName   : File we want to copy into disk\n",
           programName);
}

int main(int argc, char * argv[])
{
    /* Check input parameters */
    if (argc != 3)
    {
        printf("ERROR: Invalid parameters!!!\n");
        showUsage(argv[0]);
        exit(-1);
    }

    diskPut(argv[1], argv[2]);

    return 0;
}


