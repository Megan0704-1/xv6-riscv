#include "user/user.h"
#include "kernel/fs.h"

int
main(int argc, char *argv[])
{
  char buf[BSIZE];

  if(disk_read(1, buf) < 0){
    printf("disk_test: disk_read failed\n");
    exit(1);
  }

  struct superblock *sb = (struct superblock*)buf;
  printf("super‑block:\n");
  printf("  size     %d blocks\n", sb->size);
  printf("  nblocks  %d\n", sb->nblocks);
  printf("  ninodes  %d\n", sb->ninodes);
  printf("  nlog     %d (log size)\n", sb->nlog);
  exit(0);
}
