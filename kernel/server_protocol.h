// [New] Protocol for communicating with user space server 
// [New] clients should specify the struct before routing through kernel

#ifndef SERVER_PROTOCOL_H
#define SERVER_PROTOCOL_H

#include "param.h"

#ifndef USERPARAM
#define USERPARAM 1

// fs params
#define MAX_READ_SIZE 256
#define MAX_WRITE_SIZE 256

#define MAXCOUNT 8 /* Maximum multi-block transfer */

#ifndef BSIZE
#define BSIZE 1024
#endif

#endif

typedef enum {FS_OPEN, FS_READ, FS_WRITE, FS_CLOSE, FS_FSTAT, FS_DUP, FS_MKNOD, FS_CHDIR, DEV_READ, DEV_WRITE} RequestType;

typedef struct {
  int msgid; // status for reply, pid for request
  int msgtype; // request type
  int msglen; // msg len
} IPCHeader;

typedef struct {
  RequestType type;
  int client_pid;

  // param to send to server for each request
  union {
    // file system
    struct { char path[MAXPATH]; int omode; } fs_open;
    struct { int fh; } fs_close;
    struct { int fh; int len; } fs_read;
    struct { int fh; int len; char data[MAX_WRITE_SIZE]; } fs_write;
    struct { int fh; } fs_fstat;
    struct { int fh; } fs_dup;
    struct { char path[MAXPATH]; int major; int minor; } fs_mknod;
    struct { char path[MAXPATH]; } fs_chdir;

    // device driver
    struct { int block_no; int count; } dev_read;
    struct { int block_no; int count; char data[BSIZE * MAXCOUNT]; } dev_write;
  } handle;
} IPCRequest;

typedef struct {
  int status; /* total bytes trasfer if success, else -1 */
  int server_pid;

  // return objects from each request
  union {
    // file system
    struct { int fd; } fs_open;
    struct { } fs_close;
    struct { int bytes; char data[MAX_READ_SIZE]; } fs_read;
    struct { int bytes; } fs_write;
    struct { struct stat *st; } fs_fstat;
    struct { int fd; } fs_dup;
    struct { } fs_mknod;
    struct { } fs_chdir;

    // device driver
    struct { int bytes; } dev_write;
  } handle;
} IPCReply;

typedef struct {
  IPCHeader header;
  IPCRequest req;
} ServiceRequest;

typedef struct {
  IPCHeader header;
  IPCReply repl;
} ServiceReply;

#endif // SERVER_PROTOCOL_H
