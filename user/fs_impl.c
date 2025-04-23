#define USERSPACE_FS

#include "user/user.h"
#include "user/fs_impl.h"
#include "user/file_impl.h"
#include "user/buf.h"
#include "user/stat.h"

#define min(a, b) ((a) < (b) ? (a) : (b))

#define begin_op()    ((void)0)
#define end_op()    ((void)0)
#define log_write(b)    ((void)0)

// global vars
struct superblock sb; // one sb per device
static struct inode icache[NINODE];  // 50
static uchar free_bitmap[MAX_BMAP_BYTES];


// Init fs
void
fs_init(void) {
  fileinit();
  char buf[BSIZE]; 
  if(disk_read(1, buf) < 0) {
    exit(1);
  }

  memmove(&sb, buf, sizeof(sb));

  if(sb.magic != FSMAGIC){
    exit(1);
  }

  uint n = NBMAP(sb);
  for(uint i=0; i<n; ++i) {
    if(disk_read(sb.bmapstart + i, buf) < 0) {
      exit(1);
    }
    memmove(free_bitmap + i*BSIZE, buf, BSIZE);
  }

  // clear inode cache
  for(int i=0; i<NINODE; ++i) {
    icache[i].ref = 0;
    icache[i].valid = 0;
  }

  // set root dir as cur dir for all procs
  struct inode *root_ip = iget(ROOTDEV, ROOTINO);
  if(root_ip == 0) {
    exit(1);
  }

  for(int p=0; p<NPROC; ++p) {
    // note. fd 0,1,2 is reserved by console
    for(int fd=0; fd<3; ++fd) {
      fd_table[p][fd].ref += 1;
      fd_table[p][fd].type = FD_DEVICE;
      fd_table[p][fd].major = CONSOLE;
      fd_table[p][fd].readable = 1;
      fd_table[p][fd].writable = 1;
      fd_table[p][fd].ip=0;
    }

    // each proc holds a ref to root_ip
    fd_table[p][0].ip = root_ip;
    root_ip->ref ++;
  }

}

// iget: get inode with num inum from disk (look up in icache)
struct inode*
iget(uint dev, uint inum)
{
  struct inode *ip, *empty=0;
  
  // Check if already cached
  for(ip = icache; ip < &icache[NINODE]; ip++){
    if(ip->ref > 0 && ip->dev == dev && ip->inum == inum){
      ip->ref++;
      return ip;
    }
    if(empty == 0 && ip->ref == 0) empty = ip;
  }

  if(empty == 0) {
    debug_msg("should not reach here.");
    return 0;
  }

  empty->dev = dev;
  empty->inum = inum;
  empty->ref = 1;
  empty->valid = 0;

  uint blkno = IBLOCK(inum, sb);
  char buf[BSIZE];
  if(disk_read(blkno, buf) < 0) {
    debug_msg("iget: disk_read failed");
    empty->ref = 0;
    return 0;
  }

  // copy device inode to file inode
  struct dinode *dip = (struct dinode*)buf + (inum % IPB);
  empty->type = dip->type;
  empty->major = dip->major;
  empty->minor = dip->minor;
  empty->nlink = dip->nlink;
  empty->size = dip->size;
  memmove(empty->addrs, dip->addrs, sizeof(dip->addrs));
  empty->valid = 1;

  return empty;

}

// write modified inode back to device 
void
iupdate(struct inode *ip)
{
  struct dinode d;
  d.type = ip->type;
  d.major = ip->major;
  d.minor = ip->minor;
  d.nlink = ip->nlink;
  d.size = ip->size;
  memmove(d.addrs, ip->addrs, sizeof(ip->addrs));

  // blk no
  uint blkno = IBLOCK(ip->inum, sb);
  uchar buf[BSIZE];
  if(disk_read(blkno, buf) < 0) {
    debug_msg("iupdate: disk_read failed\n");
    exit(1);
  }

  // write to disk
  struct dinode *dip = (struct dinode *)buf + (ip->inum % IPB);
  *dip = d;

  if(disk_write(blkno, buf) < 0) {
    debug_msg("iupdate: disk_write failed\n");
    exit(1);
  }
}

// alloc a free disk block and mark it as used
int
balloc(uint dev)
{
  // bitmap block is byte-address: 1024 byte per block
  for(uint bmapBlk=0; bmapBlk<NBMAP(sb); ++bmapBlk) {
    uint blkno = sb.bmapstart + bmapBlk;
    uint blkoffBase = bmapBlk * BSIZE;
    uchar *blkptr = free_bitmap + blkoffBase;

    // scan every byte in the bitmap blk
    for(uint byteidx=0; byteidx < BSIZE; ++byteidx) {
      int byte = blkptr[byteidx];
      if(byte == 0xFF) {
        // all bits in this byte are used
        continue;
      }

      for(int bit=0; bit<8; ++bit) {
        if((byte & (1<<bit)) == 0) {
          // mark bit as allocated
          blkptr[byteidx] |= (1<<bit);

          // write the slice back to disk
          if(disk_write(blkno, blkptr) < 0) {
            debug_msg("balloc: disk_write blk ptr failed\n");
            return -1;
          }

          // absolate data blk num
          uint dataBlk = bmapBlk * BSIZE * 8 + byteidx * 8 + bit;
          return dataBlk;
        }
      }
    }
  }
  return -1; // no free blk
}

// Free a disk block.
void
bfree(uint dev, uint b)
{
  if(b >= sb.size) return;

  uint bmap_idx = b / 8;
  uint bit = b % 8;
  uchar mask = ~(1 << bit);
  free_bitmap[bmap_idx] &= mask;

  // write back to disk
  int dbmap_idx = BBLOCK(b, sb);
  if(disk_write(dbmap_idx, free_bitmap + (dbmap_idx-sb.bmapstart) * BSIZE) < 0) {
    exit(1);
  }
}

// Read data from inode.
int
readi(struct inode *ip, char *dst, uint off, uint n)
{
  if(off > ip->size || off + n < off) return -1; // check overflow
  if(off + n > ip->size) n = ip->size - off; // not read pass EOF

  uint total_byte = 0;
  while(total_byte < n) {
    uint blkno = off / BSIZE;
    uint blkoff = off % BSIZE;
    uint toread = (n - total_byte < BSIZE - blkoff) ? (n - total_byte) : (BSIZE - blkoff);

    uint addr;
    if(blkno < NDIRECT) {
      addr = ip->addrs[blkno];
    } else {
      // indirect blk
      if (ip->addrs[NDIRECT] == 0) {
        addr = 0;
      } else {
        char indir[BSIZE];
        if(disk_read(ip->addrs[NDIRECT], indir) < 0) return -1;
        uint *table = (uint*)indir;
        addr = table[blkno - NDIRECT];
      }
    }

    if(addr == 0) {
      memset(dst, 0, toread);
    } else {
      char buf[BSIZE];
      if(disk_read(addr, buf) < 0) return -1;
      memmove(dst + total_byte, buf+blkoff, toread);
    }
    total_byte += toread;
    off += toread;
  }

  return total_byte;
}

// write data to inode
int
writei(struct inode *ip, const char *src, uint off, uint n) {
  if(off > ip->size || off + n < off) {
    debug_msg("invalid offset\n");
    return -1;
  }
  if(off + n > MAXFILE*BSIZE) {
    debug_msg("overflow\n");
    return -1;
  }

  uint total_byte = 0;
  while(total_byte < n) {
    uint blkno = off / BSIZE;
    uint blkoff = off % BSIZE;
    uint towrite = (n-total_byte < BSIZE - blkoff) ? (n-total_byte) : (BSIZE - blkoff);

    uint addr;
    if(blkno < NDIRECT) {
      if((addr = ip->addrs[blkno]) == 0) {
        addr = balloc(ip->dev);
        if(addr < 0)  {
          debug_msg("address allocated by balloc is invalid\n");
          debug(addr);
          return -1;
        }
        ip->addrs[blkno] = addr;
      }
    } else {
      // indirect blk handling
      if(ip->addrs[NDIRECT] == 0) {
        uint iaddr = balloc(ip->dev);
        if(iaddr < 0) return -1;
        ip->addrs[NDIRECT] = iaddr;

        char zero[BSIZE];
        memset(zero, 0, BSIZE);
        if(disk_write(iaddr, zero) < 0) return -1;
      }

      // read indir blk to table
      char indir[BSIZE];
      if(disk_read(ip->addrs[NDIRECT], indir) < 0) return -1;
      uint *table = (uint*)indir;
      uint idx = blkno - NDIRECT;

      if((addr = table[idx]) == 0) {
        addr = balloc(ip->dev);
        if(addr < 0) return -1;
        table[idx] = addr;
        // write back indir blk
        if(disk_write(ip->addrs[NDIRECT], indir) < 0) return -1;
      }
    }

    char buf[BSIZE];
    if(blkoff != 0 || towrite != BSIZE) {
      if(addr == 0) {
        memset(buf, 0, BSIZE);
      } else {
        if(disk_read(addr, buf) < 0) {
          debug_msg("disk_read failed for writei\n");
          debug_msg("addr");
          debug(addr);
          debug_msg("buf");
          debug((uint64)buf);
          return -1;
        }
      }
    }

    // copy data to blk buffer and write to disk
    memmove(buf + blkoff, src + total_byte, towrite);
    if(disk_write(addr, buf) < 0) {
      debug_msg("disk_write failed for writei\n");
      debug_msg("addr");
      debug(addr);
      debug_msg("buf");
      debug((uint64)buf);
      return -1;
    }

    total_byte += towrite;
    off += towrite;
  }

  if(off > ip->size) {
    ip->size = off;
  }
  
  return total_byte;
}

// find dirent with giv en name in inode (dp)
int dirlookup(struct inode *dp, const char *name, uint *poff) {
    if(dp->type != T_DIR) return -1;

    struct dirent de;
    for(uint off = 0; off < dp->size; off += sizeof(de)) {
        if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de)) return -1;
        if(de.inum == 0) continue;
        if(strcmp(name, de.name) == 0) {
            // found
            if(poff) *poff = off;
            return de.inum;
        }
    }
    return -1;  // not found
}

// dirlink: write a new directory entry (name -> inum) into directory dp.
int dirlink(struct inode *dp, const char *name, uint inum) {
  if(dp->type != T_DIR) return -1;

  // Check that name is not present (to enforce unique names)
  uint off;
  struct dirent de;
  if(dirlookup(dp, name, 0) >= 0) {
    return -1; // exist
  }

  // Find an empty dirent
  for(off = 0; off < dp->size; off += sizeof(de)) {
    if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de)) {
      exit(1);
    }
    if(de.inum == 0) break;
  }

  // Prepare new dir entry
  de.inum = inum;
  memset(de.name, 0, DIRSIZ);
  strcpy(de.name, name);
  if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de)) {
    exit(1);
  }
  return 0;
}

// isdirempty: check if directory dp is empty 
int isdirempty(struct inode *dp) {
  struct dirent de;
  uint off = 0;

  // skip . entry
  if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de)) {
    exit(1);
  }
  off += sizeof(de);

  // skip .. entry
  if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de)) {
    exit(1);
  }

  off += sizeof(de);
  for(; off < dp->size; off += sizeof(de)) {
    if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de)) {
      exit(1);
    }
    if(de.inum != 0)
      return 0;  // found a file
  }
  return 1;
}

// resolve inode path (relative to absolute path)
struct inode *namei_for_proc(int pid, const char *path) {
  struct inode *ip;
  
  // abs path
  if(path[0] == '/') {
    ip = iget(ROOTDEV, ROOTINO);
  } else {
    ip = fd_table[pid][0].ip; // root inode
    ip->ref ++;
  }

  char name[DIRSIZ + 1];
  const char *p = path;

  while(*p == '/') p++;
  for(;;) {
    // end of path
    if(*p == '\0') {
      return ip;
    }

    // extract path comp and put to name
    char *s = name;
    while(*p && *p != '/' && *p != '\0') {
      if(s < name + DIRSIZ) *s++ = *p;
      p++;
    }
    *s = '\0';

    while(*p == '/') p++;

    // lookup name in cur dir
    if(ip->type != T_DIR) {
      debug_msg("namei_for_proc: no file found under current path");
      ip->ref --;
      debug_msg("namei_for_proc\n");
      return 0;
    }

    uint off=0;
    int inum = dirlookup(ip, name, &off);
    if(inum < 0) {
      debug_msg("namei_for_proc: dir lookup not found for ip");
      ip->ref --;
      debug_msg("namei_for_proc\n");
      return 0;
    }

    struct inode *next = iget(ip->dev, inum);
    ip->ref--;

    if(next == 0) {
      debug_msg("namei_for_proc: search exhausted");
      debug_msg("namei_for_proc\n");
      return 0;
    }

    ip=next;
  }
}

struct inode*
nameiparent_for_proc(int pid, const char *path, char *name)
{
  // find last component after slash
  const char *s = path, *last = s;
  while(*s){
    if(*s == '/') last = s+1;
    s++;
  }
  strcpy(name, last);

  // pull off parent path
  char parent[128];
  int len = last - path;
  if(len == 0){
    // no slash = cwd is parent
    memcpy(parent, ".", 2);
  } else {
    memcpy(parent, path, len);
    parent[len] = 0;
  }

  struct inode *dp = namei_for_proc(pid, parent);
  return dp;
}

struct inode*
create(int pid, const char *path, short type, short major, short minor)
{
  char name[DIRSIZ+1];

  // find parent dir and final component
  struct inode *dp = nameiparent_for_proc(pid, path, name);
  if(dp == 0 || name[0] == 0 || dp->type != T_DIR) {
    debug_msg("checking dp");
    debug(dp->type);
    debug(dp->inum);
    debug(dp->ref);
    sleep(20);
    if(dp) dp->ref --;
    return 0;
  }

  // does the file already exist?
  uint off;
  int inum = dirlookup(dp, name, &off);
  struct inode *ip;
  if(inum >= 0){
    ip = iget(dp->dev, inum);
    dp->ref--;
    if(ip && ip->type == type) {
      debug_msg("find ip via dirlookup");
      return ip;
    }
    if(ip) ip->ref--;
    return 0;
  }

  // allocate a fresh inode on disk
  ip = ialloc(dp->dev, type);
  if(ip == 0){
    debug_msg("create: ialloc for ip failed\n");
    dp->ref--;
    return 0;
  }
  ip->type = type;
  ip->major = major;
  ip->minor = minor;
  ip->nlink = 1;
  ip->size  = 0;
  iupdate(ip);

  // if it’s a directory, write "." and ".."
  if(type == T_DIR){
    ip->nlink = 2;
    iupdate(ip);

    struct dirent de;
    de.inum = ip->inum;
    strcpy(de.name, ".");
    writei(ip, (char*)&de, 0, sizeof(de));

    de.inum = dp->inum;
    strcpy(de.name, "..");
    writei(ip, (char*)&de, sizeof(de), sizeof(de));
  }

  // add entry “name” -> inum into parent directory
  if(dirlink(dp, name, ip->inum) < 0){
    debug_msg("create: dirlink failed\n");
    // roll back
    ip->nlink = 0;
    iupdate(ip);
    ip->ref--;
    dp->ref--;
    return 0;
  }
  if(type == T_DIR){
    dp->nlink++;
    iupdate(dp);
  }

  dp->ref--;
  return ip;
}

struct inode*
ialloc(uint dev, short type)
{
  int inum;
  uchar buf[BSIZE];
  struct dinode *dip;

  for(inum = 1; inum < sb.ninodes; inum++){
    // read the raw block
    disk_read(IBLOCK(inum, sb), buf);
    dip = (struct dinode*)buf + (inum % IPB);

    if(dip->type == 0) {
      // zero out and allocate
      memset(dip, 0, sizeof(*dip));
      dip->type = type;

      // write it back
      disk_write(IBLOCK(inum, sb), buf);

      // now return the in‐memory inode
      return iget(dev, inum);
    }
  }

  return 0;
}

void
stati(struct inode *ip, struct stat *st)
{
  st->dev = ip->dev;
  st->ino = ip->inum;
  st->type = ip->type;
  st->nlink = ip->nlink;
  st->size = ip->size;
}
