#include "user/user.h"
#include "user/fs_impl.h"
#include "user/file_impl.h"
#include "user/stat.h"

static ServiceRequest req __attribute__((aligned(8)));
static ServiceReply repl __attribute__((aligned(8)));

static int fs_pid_cached = -1;
static int get_fs_pid(void) {
  if(fs_pid_cached < 0) {
    fs_pid_cached = lookup_service(FS_SERVER_NAME);
  }
  return fs_pid_cached;
}

static int rpc_roundtrip(ServiceRequest *req, ServiceReply *repl) {
  int pid = get_fs_pid();
  if(pid < 0) return -1;
  req->client_pid = getpid();

  if(send(pid, req) < 0) return -1;
  if(recv(IPC_ANY_SENDER, repl, IPC_WAITING) < 0) return -1;

  if(repl->status < 0) return -1;
  return 0;
}

// file syscall wrappers
int open(const char *path, int flags) {
  // ServiceRequest req;
  // ServiceReply repl;

  memset(&req, 0, sizeof(req));
  req.type = FS_OPEN;
  strcpy(req.handle.fs_open.path, path);
  req.handle.fs_open.omode = flags;
  if((rpc_roundtrip(&req, &repl) < 0)) return -1;
  return repl.handle.fs_open.fd;
}

int read(int fd, void *buf, int n) {
  // ServiceRequest req;
  // ServiceReply repl;

  int total = 0;
  char *buffer = (char*)buf;
  while(total < n) {
    int chunk = (n-total) > MAX_READ_SIZE ? MAX_READ_SIZE : (n-total);
    memset(&req, 0, sizeof(req));
    req.type = FS_READ;
    req.handle.fs_read.fh = fd;
    req.handle.fs_read.len = chunk;
    if(rpc_roundtrip(&req, &repl) < 0) {
      return -1;
    }

    int read_bytes = repl.handle.fs_read.bytes;
    if(read_bytes > 0) {
      memmove(buffer, repl.handle.fs_read.data, read_bytes);
      buffer += read_bytes;
      total += read_bytes;
    }
    if(read_bytes < chunk) break; // fewer bytes than requested
  }
  return total;
}

int write(int fd, const void *buf, int n) {
  // ServiceRequest req;
  // ServiceReply repl;

  int total = 0;
  while(total < n) {
    int chunk = (n-total) > MAX_WRITE_SIZE ? MAX_WRITE_SIZE : (n-total);
    memset(&req, 0, sizeof(req));

    req.type = FS_WRITE;
    req.handle.fs_write.fh = fd;
    req.handle.fs_write.len = chunk;

    memmove(req.handle.fs_write.data, buf + total, chunk);
    if(rpc_roundtrip(&req, &repl) < 0) return -1;
    total += repl.handle.fs_write.bytes;
    if(repl.handle.fs_write.bytes < chunk) break;
  }
  return total;
}

int close(int fd) {
  // ServiceRequest req;
  // ServiceReply repl;

  memset(&req, 0, sizeof(req));

  req.type = FS_CLOSE;
  req.handle.fs_close.fh = fd;

  if((rpc_roundtrip(&req, &repl) < 0)) return -1;
  return 0;
}

int fstat(int fd, struct stat *st) {
  // ServiceRequest req;
  // ServiceReply repl;

  memset(&req, 0, sizeof(req));

  req.type = FS_FSTAT;
  req.handle.fs_fstat.fh = fd;

  if((rpc_roundtrip(&req, &repl) < 0)) return -1;

  memmove(st, &repl.handle.fs_fstat.st, sizeof(struct stat));
  return 0;
}

int dup(int fd) {
  // ServiceRequest req = {0};
  // ServiceReply repl = {0};

  memset(&req, 0, sizeof(req));

  req.type = FS_DUP;
  req.handle.fs_dup.fh = fd;

  if((rpc_roundtrip(&req, &repl) < 0)) return -1;

  return repl.handle.fs_dup.fd;
}
