// init: The initial user-level program

#include "kernel/types.h"
#include "user/stat.h"
#include "kernel/spinlock.h"
#include "kernel/sleeplock.h"
#include "user/fs_impl.h"
#include "user/file_impl.h"
#include "user/user.h"
#include "user/fcntl.h"

char *argv[] = { "sh", 0 };

int
main(void)
{
  int pid, wpid;

  pid = fork();
  if(pid == 0) {
    exec("fs_server", (char*[]){ "fs_server", 0 });
    exit(1);
  } 

  while(lookup_service(FS_SERVER_NAME) < 0) {
    sleep(1);
  }

  int fd = open("console", O_RDWR);
  if(fd < 0){
    mknod("console", CONSOLE, 0);
    fd = open("console", O_RDWR);
  }
  dup(fd);  // stdout
  dup(fd);  // stderr

  for(;;){
    printf("init: starting sh\n");
    pid = fork();
    if(pid < 0){
      printf("init: fork failed\n");
      exit(1);
    }
    if(pid == 0){
      exec("sh", argv);
      printf("init: exec sh failed\n");
      exit(1);
    }

    for(;;){
      // this call to wait() returns if the shell exits,
      // or if a parentless process exits.
      wpid = wait((int *) 0);
      if(wpid == pid){
        // the shell exited; restart it.
        break;
      } else if(wpid < 0){
        printf("init: wait returned an error\n");
        exit(1);
      } else {
        // it was a parentless process; do nothing.
      }
    }
  }
}

