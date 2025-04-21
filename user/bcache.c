#include "user/user.h"
#include "user/bcache.h"

#ifndef NCACHE
#define NCACHE 32
#endif

static buf cache[NCACHE];

// read from blkno provided
// return buf*
buf *
bread(uint blkno) 
{
  for(int i=0; i<NCACHE; ++i) {
    if((cache[i].blkno == blkno) && (cache[i].data[0])) {
      return &cache[i];
    }
  }

  int v=0;
  disk_read(blkno, cache[v].data);
  cache[v].blkno = blkno;
  return &cache[v];
}

// provide buf struct
// writes to disk
void
bwrite(buf *buffer)
{
  disk_write(buffer->blkno, buffer->data);
}

void
brelse(buf* b)
{
  /*do nothing*/
}
