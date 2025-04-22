// user/fs_server.c
// user space FS server

#include "user/user.h"
#include "user/file_impl.h"
#include "user/fs_impl.h"
#include "user/stat.h"
#include "user/fcntl.h"

#define CHECKIP(x) \
  if(x == 0) return -1

// global vars
static int fs_ready = 0;
struct file fd_table[NPROC][NOFILE]; // open file per process

// helper for IPC req
static int fs_open(int pid, const char *path, int omode);
static int fs_read(int pid, int fd, char *dst, int n);
static int fs_write(int pid, int fd, const char *src, int n);
static int fs_close(int pid, int fd);
static int fs_fstat(int pid, int fd, struct stat *st);
static int fs_dup(int pid, int fd);
static int fs_mknod(int pid, const char *path, short major, short minor);
// static int fs_unlink(int pid, const char *path);

int main(void) {
  register_service(FS_SERVER_NAME, getpid());
  fs_init();
  fs_ready = 1;

  ServiceRequest fs_req;
  ServiceReply fs_repl; 

  memset(&fs_req, 0, sizeof(fs_req));

  while(1) {
    fs_req.header.msgtype = 1;
    fs_repl.header.msgtype = 0;

    int recv_status = recv(IPC_ANY_SENDER, &fs_req, IPC_WAITING);
    sleep(1);
    if(recv_status < 0) {
      return -1;
    }

    memset(&fs_repl, 0, sizeof(fs_repl));
    fs_repl.repl.status = 0;

    int sender_pid = fs_req.req.client_pid;

    // dispatch request
    switch(fs_req.req.type) {
      case FS_OPEN: {
        char *path = fs_req.req.handle.fs_open.path;
        int omode = fs_req.req.handle.fs_open.omode;
        int fd = fs_open(sender_pid, path, omode);
        // sanity
        if(fd < 0) {
          fs_repl.repl.status = -1;
        } else {
          fs_repl.repl.handle.fs_open.fd = fd;
        }
        break;
      }
      case FS_READ: {
        int from_fd = fs_req.req.handle.fs_read.fh;
        char *to_data = fs_repl.repl.handle.fs_read.data;
        int len = fs_req.req.handle.fs_read.len;
        int byte = fs_read(sender_pid, from_fd, to_data, len);
        // sanity
        if(byte < 0) {
          fs_repl.repl.status = -1;
        } else {
          fs_repl.repl.handle.fs_read.bytes = byte;
        }
        break;
      }
      case FS_WRITE: {
        int to_fd = fs_req.req.handle.fs_write.fh;
        char *from_data = fs_req.req.handle.fs_write.data;
        int len = fs_req.req.handle.fs_write.len;
        int byte = fs_write(sender_pid, to_fd, from_data, len);
        // sanity
        if(byte < 0) {
          fs_repl.repl.status = -1;
        } else {
          fs_repl.repl.handle.fs_write.bytes = byte;
        }
        break;
      }
      case FS_CLOSE:
        fs_repl.repl.status = fs_close(sender_pid, fs_req.req.handle.fs_close.fh);
        break;
      case FS_FSTAT:
        fs_repl.repl.status = fs_fstat(sender_pid, fs_req.req.handle.fs_fstat.fh, &fs_repl.repl.handle.fs_fstat.st);
        break;
      case FS_DUP: {
        int fd = fs_dup(sender_pid, fs_req.req.handle.fs_dup.fh);
        if(fd < 0) {
          fs_repl.repl.status = -1;
        } else {
          fs_repl.repl.handle.fs_dup.fd = fd;
        }
        break;
       }
      case FS_MKNOD: {
        const char *path = fs_req.req.handle.fs_mknod.path;
        short major = fs_req.req.handle.fs_mknod.major;
        short minor = fs_req.req.handle.fs_mknod.minor;
        fs_repl.repl.status = fs_mknod(sender_pid, path, major, minor);
        break;
       }
      default:
        fs_repl.repl.status = -1;
    }
    // send repl to req back to client
    fs_repl.header.msglen = sizeof(fs_repl.repl);
    send(sender_pid, &fs_repl);
  }

  return 0;
}

static int fs_open(int pid, const char *path, int omode) {
  int fd;
  // find first unuserd handle from service requester
  for(fd = 3; fd < NOFILE; ++fd) {
    if(fd_table[pid][fd].ref == 0) break;
  }
  if(fd > NOFILE) return -1;

  struct inode *ip = 0;
  if(omode & O_CREATE /*0x200*/) {
    ip = create(pid, path, T_FILE, 0, 0);
    CHECKIP(ip);
  } else {
    ip = namei_for_proc(pid, path);
    CHECKIP(ip); // file not found

    // try to write a dir
    if((ip->type == T_DIR) && ((omode & O_RDONLY) || (omode & O_RDWR))) {
      ip->ref --;
      return -1;
    }

    // let device open succeed
    if(ip->type == T_DEVICE) {
      fd_table[pid][fd].type = FD_DEVICE;
      fd_table[pid][fd].major = ip->major;
    } else {
      fd_table[pid][fd].type = FD_INODE;
    }
  }

  // ip is now in memory
  fd_table[pid][fd].ref++;
  fd_table[pid][fd].ip = ip;
  fd_table[pid][fd].off = 0;
  fd_table[pid][fd].readable = !(omode & O_WRONLY);
  fd_table[pid][fd].writable = (omode & O_WRONLY) || (omode & O_RDWR);

  return fd;
}

// read from an open file to buffer
static int fs_read(int pid, int fd, char *dst, int n) {
  if(fd < 0 || fd >= NOFILE || fd_table[pid][fd].ref == 0) return -1;

  struct file *f = &fd_table[pid][fd];
  if(!f->readable) return -1;

  if(f->type == FD_DEVICE) {
    return fileread(f, (uint64)dst, n);
  }

  struct inode *ip = f->ip;
  int r = readi(ip, dst, f->off, n);

  if(r > 0) {
    f->off += r;
  }

  return r;
}

// write data to open file from buffer
static int fs_write(int pid, int fd, const char *src, int n) {
  if(fd < 0 || fd >= NOFILE || fd_table[pid][fd].ref == 0) {
    return -1;
  }

  struct file *f = &fd_table[pid][fd];
  if(!f->writable) return -1;

  if(f->type == FD_DEVICE) {
    return filewrite(f, (uint64)src, n);
  }
  
  struct inode *ip = f->ip;
  int r = writei(ip, src, f->off, n);
  
  if(r > 0) {
    f->off += r;
    iupdate(ip); // update inode on disk (account for file size change)
  }

  return r;
}

// close a file (free if not used)
static int fs_close(int pid, int fd) {
  if(fd < 0 || fd >= NOFILE || fd_table[pid][fd].ref == 0) return -1;

  struct file *f = &fd_table[pid][fd];
  struct inode *ip = f->ip;

  f->ref = 0;
  f->ip = 0;
  f->off = 0;
  f->readable = 0;
  f->writable = 0;

  ip->ref --;
  // check if refs and links remain -> 0: it's a free inode
  if(ip->ref <=0 && ip->nlink == 0) {
    // free direct blks
    for(int i=0; i<NDIRECT; ++i) {
      if(ip->addrs[i] != 0) {
        bfree(ip->dev, ip->addrs[i]);
        ip->addrs[i] = 0;
      }
    }

    // free indirect blk
    if(ip->addrs[NDIRECT] != 0) {
      char buf[BSIZE];
      disk_read(ip->addrs[NDIRECT], buf); // read from indirect to buf
      uint *table = (uint*)buf;
      for(int j=0; j<NINDIRECT; ++j) {
        if(table[j] != 0) {
          bfree(ip->dev, table[j]);
        }
      }
      bfree(ip->dev, ip->addrs[NDIRECT]);
    }

    ip->type = 0;
    iupdate(ip);
  }
  return 0;
}

static int fs_fstat(int pid, int fd, struct stat *st) {
  if(fd < 0 || fd > NOFILE || fd_table[pid][fd].ref == 0) return -1;
  struct inode *ip = fd_table[pid][fd].ip;

  st->dev = ip->dev;
  st->ino = ip->inum;
  st->type = ip->type;
  st->nlink = ip->nlink;
  st->size = ip->size;
  return 0;
}

static int fs_dup(int pid, int fh) {
  struct file *f = &fd_table[pid][fh];
  if(fh < 0 || fh >= NOFILE || f->ref == 0) {
    return -1;
  } 
  int new_fd = 0;
  for(new_fd=3; new_fd<NOFILE; ++new_fd) {
    if(fd_table[pid][new_fd].ref == 0) {
      break;
    }
  }
  if(new_fd > NOFILE) return -1;

  fd_table[pid][new_fd] = *f;
  fd_table[pid][new_fd].ref++;
  return new_fd;
}

// create a file node at path
static int fs_mknod(int pid, const char *path, short major, short minor) {
  struct inode *ip;
  ip = create(pid, path, T_DEVICE, major, minor);
  CHECKIP(ip);

  iupdate(ip);
  ip->ref --;
  return 0;
}

