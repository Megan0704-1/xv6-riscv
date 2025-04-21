#ifndef BCACHE_H
#define BCACHE_H

#include "user/user.h"

#ifndef BSIZE
#define BSIZE 1024
#endif

typedef struct buf {
  int valid;
  int dirty;
  uint dev;
  uint blkno;
  int refcnt;
  struct buf *prev;
  struct buf *next;
  char data[BSIZE];
} buf;

void binit(void);
buf *bread(uint);
void bwrite(buf*);
void brelse(buf*);

#endif
