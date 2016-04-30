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

void errorOutput(char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

int get_num_blocks(struct dinode* inode) {
    int num_blocks = inode->size % BSIZE == 0 ? (inode->size / BSIZE) :
                     (inode->size / BSIZE) + 1;
    if (num_blocks > (NDIRECT + NINDIRECT -1))
        errorOutput("bad address in inode");
    if (num_blocks > NDIRECT)
        num_blocks++;
    return num_blocks;
}

void is_bad_inode(void *img_ptr, struct superblock *sb) {
    struct dinode *inode = (struct dinode *) (img_ptr + 2 * BSIZE);
    int i = 0;
    for (i = 0; i < sb->ninodes; i++) {
        if (inode->type != T_NONE && inode->type != T_FILE &&
            inode->type != T_DIR && inode->type != T_DEV)
            errorOutput("bad inode");
        inode++;
    }
}

void is_bad_address_in_inode(void *img_ptr, struct superblock *sb, void *bitmap) {
    struct dinode *inode = (struct dinode *) (img_ptr + 2 * BSIZE);
    int i = 0;
    int *blocks = (int *) calloc(sb->size, sizeof(int));
    int *in_use_address = (int *) calloc(sb->size, sizeof(int));
    for (i = 0; i <= sb->ninodes; i++) {
        int num_blocks = get_num_blocks(inode);
        int j, k = 0;
        // just checking direct pointers
        for (j = 0; j < num_blocks; j++) {
            unsigned int block_num = 0;
            if (j > NDIRECT) {
                void *inode_indirect_block_addr = img_ptr + inode->addrs[NDIRECT] * BSIZE;
                int block_index = k++;
                unsigned int *temp = ((unsigned int *) (inode_indirect_block_addr +
                                                        (block_index * sizeof(unsigned int))));
                block_num = *temp;
            }
            else
                block_num = inode->addrs[j];
            if (block_num < 0 || block_num >= sb->size)
                errorOutput("bad address in inode");
            if (inode->nlink>0) {
                if (in_use_address[block_num] == 1)
                    errorOutput("address used more than once");
                in_use_address[block_num] = 1;
            }
            blocks[block_num] = 1;
            int khancha = block_num / 8;
            int offset = block_num % 8;
            char *c = (char *) (bitmap + khancha);
            char mask = 1 << (offset);
            char p = mask & (*c);
            if (p == 0)
                errorOutput("address used by inode but marked free in bitmap");
        }
        inode++;
    }
    //checking bitmap
    int inode_num_blocks = (sb->ninodes / IPB) + 1;
    int bitmap_block_num = 2 + inode_num_blocks;
    for (i = bitmap_block_num+1; i < sb->size; i++) {
        int khancha = i / 8;
        int offset = i % 8;
        char *c = (char *) (bitmap + khancha);
        char mask = 1 << (offset);
        char p = mask & (*c);
        if (p != 0 && blocks[i] == 0)
            errorOutput("bitmap marks block in use but it is not in use");
    }
}

void is_root_dir_exist(void *img_ptr, struct superblock *sb, void *bitmap) {
    struct dinode *inode = (struct dinode *) (img_ptr + 2 * BSIZE);
    inode++;
    //check root content
    if (inode->type != T_DIR)
        errorOutput("root directory does not exist");
    int j = 0, k = 0;
    int current_found = 0, parent_found = 0;
    int num_blocks = get_num_blocks(inode);
    for (j = 0; j < num_blocks; j++) {
        unsigned int block_num = 0;
        if (j > NDIRECT) {
            void *inode_indirect_block_addr = img_ptr + inode->addrs[NDIRECT] * BSIZE;
            int block_index = k++;
            unsigned int *temp = ((unsigned int *) (inode_indirect_block_addr + (block_index * sizeof(unsigned int))));
            block_num = *temp;
        }
        else
            block_num = inode->addrs[j];
        struct dirent *dir = (struct dirent *) (img_ptr + block_num * BSIZE);
        int l = 0;
        int num_dir = BSIZE / DIRENTRYSIZ;
        for (l = 0; l < num_dir; l++) {
            if (dir->inum == 1 && strcmp(dir->name, ".") == 0)
                current_found = 1;
            if (dir->inum == 1 && strcmp(dir->name, "..") == 0)
                parent_found = 1;
            dir++;
        }
    }
    if (!current_found || !parent_found)
        errorOutput("root directory does not exist");
}

void check_entry_in_dir(void *img_ptr, struct superblock *sb, int src_dir, int target_dir) {
    struct dinode *inode = (struct dinode *) (img_ptr + 2 * BSIZE + src_dir * sizeof(struct dinode));
    int num_blocks = get_num_blocks(inode);
    if (inode->type != T_DIR)
        errorOutput("INTERNAL ERROR!");
    int j = 0, k = 0, found = 0;
    for (j = 0; j < num_blocks; j++) {
        unsigned int block_num = 0;
        if (j > NDIRECT) {
            void *inode_indirect_block_addr = img_ptr + inode->addrs[NDIRECT] * BSIZE;
            int block_index = k++;
            unsigned int *temp = ((unsigned int *) (inode_indirect_block_addr +
                                                    (block_index * sizeof(unsigned int))));
            block_num = *temp;
        }
        else
            block_num = inode->addrs[j];
        struct dirent *dir = (struct dirent *) (img_ptr + block_num * BSIZE);
        int l = 0;
        int num_dir = BSIZE / DIRENTRYSIZ;
        for (l = 0; l < num_dir; l++, dir++) {
            int inode_inside = dir->inum;
            if (inode_inside == target_dir)
                found = 1;
        }
    }
    if (found == 0)
        errorOutput("parent directory mismatch");
}

void is_current_parent_dir_exist(void *img_ptr, struct superblock *sb, void *bitmap) {
    struct dinode *inode = (struct dinode *) (img_ptr + 2 * BSIZE);
    int i = 0;
    for (i = 0; i < sb->ninodes; i++, inode++) {
        int inode_num = i;
        if (inode_num == 1 && inode->type != T_DIR)
            errorOutput("root directory does not exist");
        if (inode->type != T_DIR)
            continue;
        int j = 0, k = 0;
        int current_found = 0, parent_found = 0;
        int num_blocks = get_num_blocks(inode);
        for (j = 0; j < num_blocks; j++) {
            unsigned int block_num = 0;
            if (j > NDIRECT) {
                void *inode_indirect_block_addr = img_ptr + inode->addrs[NDIRECT] * BSIZE;
                int block_index = k++;
                unsigned int *temp = ((unsigned int *) (inode_indirect_block_addr +
                                                        (block_index * sizeof(unsigned int))));
                block_num = *temp;
            }
            else
                block_num = inode->addrs[j];
            struct dirent *dir = (struct dirent *) (img_ptr + block_num * BSIZE);
            int l = 0;
            int num_dir = BSIZE / DIRENTRYSIZ;
            for (l = 0; l < num_dir; l++) {
                if (inode_num == 1) {
                    if (dir->inum == 1 && strcmp(dir->name, ".") == 0)
                        current_found = 1;
                    if (dir->inum == 1 && strcmp(dir->name, "..") == 0) {
                        check_entry_in_dir(img_ptr, sb, dir->inum, inode_num);
                        parent_found = 1;
                    }
                }
                else {
                    //todo  inode marked use but not found in a directory. - scan 1 dir scan 2 all files - check it
                    if (dir->inum == inode_num && strcmp(dir->name, ".") == 0)
                        current_found = 1;
                    if (strcmp(dir->name, "..") == 0) {
                        check_entry_in_dir(img_ptr, sb, dir->inum, inode_num);
                        parent_found = 1;
                    }
                }
                dir++;
            }
        }
        if (!current_found || !parent_found) {
            if (inode_num == 1)
                errorOutput("root directory does not exist");
            else
                errorOutput("directory not properly formatted.");
        }
    }
}

int main(int argc, char *argv[]) {
    int fd = open("fs.img", O_RDONLY);
    assert(fd > -1);
    int rc;
    struct stat sbuf;
    rc = fstat(fd, &sbuf);
    assert(rc == 0);
    void *img_ptr = mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    assert(img_ptr != MAP_FAILED);
    struct superblock *sb;
    sb = (struct superblock *) (img_ptr + BSIZE);
    int inode_num_blocks = (sb->ninodes / IPB) + 1;
    int bitmap_block_num = 2 + inode_num_blocks;
    void *bitmap = img_ptr + bitmap_block_num * BSIZE;

    is_bad_inode(img_ptr, sb);
    is_bad_address_in_inode(img_ptr, sb, bitmap);
//    is_root_dir_exist(img_ptr, sb, bitmap);
    is_current_parent_dir_exist(img_ptr, sb, bitmap);

    printf("YO bro %d, %d, %d \n", sb->size, sb->ninodes, sb->nblocks);

    struct dinode *inode = (struct dinode *) (img_ptr + 2 * BSIZE);
    int i = 0;
    for (i = 0; i < sb->ninodes; i++) {
        inode++;
        if (inode->type != T_NONE) {
            printf("INODE info - %d %d, %d \n", i, inode->type, inode->size);
            int num_blocks = get_num_blocks(inode);
            printf("Num- blocks %d \n", num_blocks);
            if (inode->type != T_DIR)
                continue;
            // this will run only for dir structure
            int j = 0;
            for (j = 0; j < num_blocks && j < NDIRECT + 1; j++) {
                int num_dir = BSIZE / DIRENTRYSIZ;
                struct dirent *dir = (struct dirent *) (img_ptr + inode->addrs[j] * BSIZE);
                int k = 0;
                for (k = 0; k < num_dir; k++) {
                    printf("Directory entry %d %s\n", dir->inum, dir->name);
                    dir++;
                }
                printf("Address of addrs :: %u \n", inode->addrs[j]);
            }
        }
    }
    return 0;
}
