//
// Created by alihitawala on 4/29/16.
//

#ifndef FILECHECKER_DEF_H
#define FILECHECKER_DEF_H
// On-disk file system format.
// Both the kernel and user programs use this header file.

// Block 0 is unused.
// Block 1 is super block.
// Inodes start at block 2.

#define ROOTINO 1  // root i-number
#define BSIZE 512  // block size

// File system super block
struct superblock {
    unsigned int size;         // Size of file system image (blocks)
    unsigned int nblocks;      // Number of data blocks
    unsigned int ninodes;      // Number of inodes.
};

#define NDIRECT 12
#define NINDIRECT (BSIZE / sizeof(unsigned int))
#define MAXFILE (NDIRECT + NINDIRECT)

// On-disk inode structure
struct dinode {
    short type;           // File type
    short major;          // Major device number (T_DEV only)
    short minor;          // Minor device number (T_DEV only)
    short nlink;          // Number of links to inode in file system
    unsigned int size;            // Size of file (bytes)
    unsigned int addrs[NDIRECT+1];   // Data block addresses
};

// Inodes per block.
#define IPB           (BSIZE / sizeof(struct dinode))

// Block containing inode i
#define IBLOCK(i)     ((i) / IPB + 2)

// Bitmap bits per block
#define BPB           (BSIZE*8)

// Block containing bit for block b
#define BBLOCK(b, ninodes) (b/BPB + (ninodes)/IPB + 3)

// Directory is a file containing a sequence of dirent structures.
#define DIRSIZ 14
#define DIRENTRYSIZ 16

struct dirent {
    ushort inum;
    char name[DIRSIZ];
};

#define T_NONE  0   // Directory
#define T_DIR  1   // Directory
#define T_FILE 2   // File
#define T_DEV  3   // Special device
#endif //FILECHECKER_DEF_H
