#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

int main(int argc, char** argv) {
  int ping_pid = getpid() - 1;
  struct message response;

  char* msg = "Pong waiting...\n";
  write(1, msg, strlen(msg));

  if(recv(ping_pid, &msg) < 0) {
    printf("Pong: recv failed\n");
    exit(1);
  }

  char* msg1 = "Pong got message from ping!\n";
  write(1, msg1, strlen(msg1));

  strcpy(response.data, "Hello from Pong!");

  char* msg2 = "Pong: sending reply to Ping\n";
  write(1, msg2, strlen(msg2));

  if(send(ping_pid, &response) < 0){
    printf("Pong: send failed\n");
    exit(1);
  }

  char* ex = "Pong: reply send. Exiting\n";
  write(1, ex, strlen(ex));
  exit(0);
}
