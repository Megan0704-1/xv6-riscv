//
// Support functions for system calls that involve file descriptors.
//

#define USERSPACE_FS

#include "user/user.h"
#include "user/fs_impl.h"
#include "user/file_impl.h"

#define begin_op()    ((void)0)
#define end_op()    ((void)0)
#define log_write(b)    ((void)0)

static struct file ftable[NFILE];

// Initialize the file table (must be called once at server startup)
void
fileinit(void)
{
  for(int i = 0; i < NFILE; i++){
    ftable[i].ref = 0;
  }
}

// Allocate a file structure.
struct file*
filealloc(void)
{
  for(int i = 0; i < NFILE; i++){
    if(ftable[i].ref == 0){
      ftable[i].ref      = 1;
      ftable[i].off      = 0;
      ftable[i].ip       = 0;
      ftable[i].readable = 0;
      ftable[i].writable = 0;
      return &ftable[i];
    }
  }
  return 0;
}

// Increment ref count for file f.
struct file*
filedup(struct file *f)
{
  if(f->ref < 1){
    printf("filedup: invalid ref %d\n", f->ref);
    exit(1);
  }
  f->ref++;
  return f;
}

// Close file f.  (Decrement ref count, clean up when it hits 0.)
void
fileclose(struct file *f)
{
  if(f->ref < 1){
    printf("fileclose: invalid ref %d\n", f->ref);
    exit(1);
  }
  if(--f->ref > 0)
    return;

  // last reference: write back inode and drop its ref
  if(f->ip){
    iupdate(f->ip);      // flush metadata to disk
    f->ip->ref--;
  }
  f->ip       = 0;
  f->off      = 0;
  f->readable = 0;
  f->writable = 0;
}

// Get metadata about file f.
int
filestat(struct file *f, struct stat *st)
{
  if(f->type == FD_INODE || f->type == FD_DEVICE){
    // fill in the on‐disk stat struct
    stati(f->ip, st);
    return 0;
  }
  return -1;
}

// Read up to n bytes from file f into user‐space address dst.
int
fileread(struct file *f, uint64 dst, int n)
{
  if(!f->readable || n < 0) return -1;

  int r = readi(f->ip, (char*)dst, f->off, n);
  if(r > 0)
    f->off += r;
  return r;
}

// Write up to n bytes from user‐space address src into file f.
int
filewrite(struct file *f, uint64 src, int n)
{
  if(!f->writable || n < 0) return -1;

  int r = writei(f->ip, (const char*)src, f->off, n);
  if(r > 0)
    f->off += r;
  return r;
}
