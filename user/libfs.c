#include "user/user.h"
#include "user/fs_impl.h"
#include "user/file_impl.h"
#include "user/stat.h"

static ServiceRequest srq __attribute__((aligned(8)));
static ServiceReply srp __attribute__((aligned(8)));

static int fs_pid_cached = -1;
static int get_fs_pid(void) {
  if(fs_pid_cached < 0) {
    fs_pid_cached = lookup_service(FS_SERVER_NAME);
  }
  return fs_pid_cached;
}

static int rpc_roundtrip(ServiceRequest *srq, ServiceReply *srp) {
  int fspid = get_fs_pid();
  int mypid = getpid();
  if(mypid < 0) return -1;

  srq->req.client_pid = mypid;
  srq->header.msgid = sizeof(*srq);
  srq->header.msgtype = 1; // req
  srq->header.msglen = sizeof(srq->req);

  memset(srp, 0, sizeof(*srp));
  srp->header.msgtype = 0; // repl

  if(send(fspid, srq) < 0) return -1;
  if(recv(fspid, srp, IPC_WAITING) < 0) return -1;

  if(srp->repl.status < 0) return -1;
  return 0;
}

// file syscall wrappers
int open(const char *path, int flags) {
  memset(&srq, 0, sizeof(srq));
  srq.req.type = FS_OPEN;
  strcpy(srq.req.handle.fs_open.path, path);
  srq.req.handle.fs_open.omode = flags;
  if((rpc_roundtrip(&srq, &srp) < 0)) return -1;
  return srp.repl.handle.fs_open.fd;
}

int read(int fd, void *buf, int n) {
  int total = 0;
  char *buffer = (char*)buf;
  while(total < n) {
    int chunk = (n-total) > MAX_READ_SIZE ? MAX_READ_SIZE : (n-total);
    memset(&srq, 0, sizeof(srq));
    srq.req.type = FS_READ;
    srq.req.handle.fs_read.fh = fd;
    srq.req.handle.fs_read.len = chunk;
    if(rpc_roundtrip(&srq, &srp) < 0) {
      return -1;
    }

    int read_bytes = srp.repl.handle.fs_read.bytes;
    if(read_bytes > 0) {
      memmove(buffer, srp.repl.handle.fs_read.data, read_bytes);
      buffer += read_bytes;
      total += read_bytes;
    }
    if(read_bytes < chunk) break; // fewer bytes than requested
  }
  return total;
}

int write(int fd, const void *buf, int n) {
  int total = 0;
  while(total < n) {
    int chunk = (n-total) > MAX_WRITE_SIZE ? MAX_WRITE_SIZE : (n-total);
    memset(&srq, 0, sizeof(srq));

    srq.req.type = FS_WRITE;
    srq.req.handle.fs_write.fh = fd;
    srq.req.handle.fs_write.len = chunk;

    memmove(srq.req.handle.fs_write.data, (char*)buf + total, chunk);
    if(rpc_roundtrip(&srq, &srp) < 0) return -1;
    total += srp.repl.handle.fs_write.bytes;
    if(srp.repl.handle.fs_write.bytes < chunk) break;
  }
  return total;
}

int close(int fd) {
  memset(&srq, 0, sizeof(srq));

  srq.req.type = FS_CLOSE;
  srq.req.handle.fs_close.fh = fd;

  if((rpc_roundtrip(&srq, &srp) < 0)) return -1;
  return 0;
}

int fstat(int fd, struct stat *st) {
  memset(&srq, 0, sizeof(srq));

  srq.req.type = FS_FSTAT;
  srq.req.handle.fs_fstat.fh = fd;

  if((rpc_roundtrip(&srq, &srp) < 0)) return -1;

  memmove(st, &srp.repl.handle.fs_fstat.st, sizeof(struct stat));
  return 0;
}

int dup(int fd) {
  memset(&srq, 0, sizeof(srq));

  srq.req.type = FS_DUP;
  srq.req.handle.fs_dup.fh = fd;

  if((rpc_roundtrip(&srq, &srp) < 0)) return -1;
  return srp.repl.handle.fs_dup.fd;
}

int mknod(const char *path, short major, short minor) {
  memset(&srq, 0, sizeof(srq));

  srq.req.type = FS_MKNOD;

  strcpy(srq.req.handle.fs_mknod.path, path);
  srq.req.handle.fs_mknod.major = major;
  srq.req.handle.fs_mknod.minor = minor;

  if(rpc_roundtrip(&srq, &srp) < 0) {
    return -1;
  }

  return srp.repl.status;
}

