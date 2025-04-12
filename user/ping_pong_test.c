// init: The initial user-level program

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/spinlock.h"
#include "kernel/sleeplock.h"
#include "kernel/fs.h"
#include "kernel/file.h"
#include "user/user.h"
#include "kernel/fcntl.h"

char *argv[] = { 0 };

int main(void) {

  char* start_msg = "Init: starting ping-pong test\n";
  int n = write(1, start_msg, strlen(start_msg));
  if(n < 0) {
    fprintf(2, "ping pong test failed for writing to shell.");
    exit(1);
  }

  int pid_ping = fork();
  if(pid_ping < 0) {
    fprintf(2, "init: fork failed\n");
    exit(1);
  }
  if(pid_ping == 0) {
    // at ping
    exec("ping", argv);
    fprintf(2, "init: exec ping failed\n");
    exit(1);
  }

  // parent
  int pid_pong = fork();
  if(pid_pong < 0) {
    fprintf(2, "init: second fork failed\n");
    exit(1);
  }
  if(pid_pong == 0) {
    // at pong
    exec("pong", argv);
    fprintf(2, "init: exec ping failed\n");
    exit(1);
  }

  wait(0);
  wait(0);
  printf("init: ping-pong test completed. Halting.\n");

  // Halt or loop indefinitely since there's no shell in this test
  for(;;) {
    sleep(100);   // idle
  }
}
