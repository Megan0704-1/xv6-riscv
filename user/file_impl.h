#ifndef FILE_IMPL_H
#define FILE_IMPL_H

#define NDIRECT 12
#define CONSOLE 1

#include "user/user.h"

struct file {
  enum { FD_NONE, FD_PIPE, FD_INODE, FD_DEVICE } type;
  int ref; // reference count
  char readable;
  char writable;
  struct pipe *pipe; // FD_PIPE
  struct inode *ip;  // FD_INODE and FD_DEVICE
  uint off;          // FD_INODE
  short major;       // FD_DEVICE
};

#define major(dev)  ((dev) >> 16 & 0xFFFF)
#define minor(dev)  ((dev) & 0xFFFF)
#define	mkdev(m,n)  ((uint)((m)<<16| (n)))

// inode without locks
struct inode {
  uint dev;           // Device number
  uint inum;          // Inode number
  int ref;            // Reference count
  int valid;          // inode has been read from disk?

  short type;         // copy of disk inode
  short major;
  short minor;
  short nlink;
  uint size;
  uint addrs[NDIRECT+1]; // data blks, 12 direct and 1 indirect
};

// map major device number to device functions.

typedef int (*devread_t)(int, uint64, int);
typedef int (*devwrite_t)(int, uint64, int);

static const struct devsw {
  devread_t read;
  devwrite_t write;
} devsw[NDEV] = {
  [CONSOLE] = {
    .read = console_read,
    .write = console_write
  },
};

void fileinit(void);
int fileread(struct file *f, uint64, int);
int filewrite(struct file *f, uint64, int);

#endif // FILE_IMPL_H
