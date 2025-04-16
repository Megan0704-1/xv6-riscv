// [New] Protocol for communicating with user space server 
// [New] clients should specify the struct before routing through kernel

#ifndef SERVER_PROTOCOL_H
#define SERVER_PROTOCOL_H

#include "kernel/param.h"
#include "kernel/fs.h"

#ifndef MAXCOUNT
#define MAXCOUNT 8 /* Maximum multi-block transfer */
#endif

#ifndef BSIZE
#define BSIZE 512
#endif

enum RequestType {FS_OPEN, FS_READ, FS_WRITE, FS_CLOSE, FS_FSTATS, FS_DEL, DEV_READ, DEV_WRITE};

struct ServiceRequest {
  enum RequestType type;
  int client_pid;

  // param to send to server for each request
  union {
    // file system
    struct { char path[MAXPATH]; int omode; } fs_open;
    struct { int fshandle; } fs_close;
    struct { int fshandle; int len; } fs_read;
    struct { int fshandle; int len; char data[BSIZE]; } fs_write;
    struct { char path[MAXPATH]; } fs_del;

    // device driver
    struct { uint block_no; int count; } dev_read;
    struct { uint block_no; int count; char data[BSIZE * MAXCOUNT]; } dev_write;
  } param;
};

struct ServiceReply {
  int status; /* total bytes trasfer if success, else -1 */

  // return objects from each request
  union {
    // file system
    struct { int fs_handle; } fs_open;
    struct { int bytes; char data[BSIZE]; } fs_read;
    struct { int bytes; } fs_write;
    struct { int total_blocks; int free_blocks; } fs_fstats;

    // device driver
    struct { int bytes; char data[BSIZE * MAXCOUNT]; } dev_read; 
    struct { int bytes; } dev_write;
  } reply;
};

#endif // SERVER_PROTOCOL_H
