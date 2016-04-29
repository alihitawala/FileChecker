//
// Created by alihitawala on 4/29/16.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/mman.h>
#include <limits.h>
#include "def.h"


int main(int argc, char *argv[]) {
    int fd = open("fs.img", O_RDONLY);
    assert(fd> -1);
    int rc;
    struct stat sbuf;
    rc = fstat(fd, &sbuf);
    assert(rc ==0);
    void *img_ptr = mmap(NULL,sbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    assert(img_ptr != MAP_FAILED);
    struct superblock *sb;
    sb = (struct superblock *) (img_ptr + BSIZE);

    printf("YO bro %d, %d, %d \n", sb->size, sb->ninodes, sb->nblocks);

    struct dinode *inode = (struct dinode*) (img_ptr + 2 *BSIZE);
    int i=0;
    for (i=0;i<sb->ninodes;i++) {
        inode++;
        if (inode->type != T_NONE) {
            printf("INODE info - %d %d, %d \n", i, inode->type, inode->size);
            int num_blocks = (inode->size / BSIZE) + 1;
            printf("Num- blocks %d \n", num_blocks);
            if (inode->type != T_DIR)
                continue;
            // this will run only for dir structure
            int j = 0;
            for (j=0;j<num_blocks && j<NDIRECT+1;j++) {
                printf("Address of addrs :: %ud \n", (uint)inode->addrs);
            }
        }
    }
    return 0;
}
