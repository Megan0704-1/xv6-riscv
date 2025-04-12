#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

int main(int argc, char **argv) {
  // we asume pong is fork right after ping, so pong's pid = pings's pid + 1
  int pong_pid = getpid() + 1;

  struct message reply;

  char* msg = "Hello from Ping.\n";
  write(1, msg, strlen(msg));

  char* msg0 = "Sending hello to Pong\n";
  write(1, msg0, strlen(msg0));

  if(send(pong_pid, &msg) < 0) {
    printf("Ping: send failed\n");
    exit(1);
  }

  char* msg1 = "Ping message sent, now waiting for reply...\n";
  write(1, msg1, strlen(msg1));

  if(recv(pong_pid, &reply) < 0) {
    printf("Ping: receieve failed\n");
    exit(1);
  }

  char* msg2 = "Ping: recv reply from Pong!\n";
  write(1, msg2, strlen(msg2));

  char* ex = "Ping exiting\n";
  write(1, ex, strlen(ex));
  exit(0);
}
