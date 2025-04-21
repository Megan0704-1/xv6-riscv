#ifndef FS_IMPL_H
#define FS_IMPL_H
// On-disk file system format.
// Both the kernel and user programs use this header file.

// [New]
#include "kernel/param.h"
#include "user/file_impl.h"
#define FS_SERVER_NAME "fs" // fs server name for registration

#define ROOTINO  1   // root i-number
#define BSIZE 1024  // block size


// Disk layout:
// [ boot block | super block | log | inode blocks |
//                                          free bit map | data blocks]
//
// mkfs computes the super block and builds an initial file system. The
// super block describes the disk layout:
struct superblock {
  uint magic;        // Must be FSMAGIC
  uint size;         // Size of file system image (blocks)
  uint nblocks;      // Number of data blocks
  uint ninodes;      // Number of inodes.
  uint nlog;         // Number of log blocks
  uint logstart;     // Block number of first log block
  uint inodestart;   // Block number of first inode block
  uint bmapstart;    // Block number of first free map block
};

#define FSMAGIC 0x10203040

#define NDIRECT 12
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)

// On-disk inode structure
struct dinode {
  short type;           // File type
  short major;          // Major device number (T_DEVICE only)
  short minor;          // Minor device number (T_DEVICE only)
  short nlink;          // Number of links to inode in file system
  uint size;            // Size of file (bytes)
  uint addrs[NDIRECT+1];   // Data block addresses
};

// Inodes per block.
#define IPB           (BSIZE / sizeof(struct dinode))

// Block containing inode i
#define IBLOCK(i, sb)     ((i) / IPB + sb.inodestart)

// Bitmap bits per block
#define BPB           (BSIZE*8)

// Block of free map containing bit for block b
#define BBLOCK(b, sb) ((b)/BPB + sb.bmapstart)

// Directory is a file containing a sequence of dirent structures.
#define DIRSIZ 14

struct dirent {
  ushort inum;
  char name[DIRSIZ];
};

#define NBMAP(sb) ( (sb.size + BPB - 1) / BPB )
#define MAX_BMAP_BYTES (BSIZE / 8 * FSSIZE + 1)

// reference
extern struct file fd_table[NPROC][NOFILE]; // open file per process
                                                
// declarations
void fs_init(void);
struct inode* iget(uint dev, uint inum);
void iupdate(struct inode *ip);
int balloc(uint dev);
void bfree(uint dev, uint bno);
int readi(struct inode *ip, char *dst, uint off, uint n);
int writei(struct inode *ip, const char *src, uint off, uint n);
struct inode *namei(char *path, struct inode *cwd);
struct inode *namei_for_proc(int pid, const char *path); 
struct inode *create(int pid, const char *path, short type, short major, short minor);
int dirlookup(struct inode *dp, const char *name, uint *poff);
int dirlink(struct inode *dp, const char *name, uint inum);
int isdirempty(struct inode *dp);
struct inode *ialloc(uint, short);
void stati(struct inode*, struct stat*);

#endif // FS_IMPL_H
