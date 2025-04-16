#include "user/user.h"

#define CHECK(cond, msg) do { \
  if(!(cond)) { \
    printf("TEST FAILED: %s (line %d)\n", msg, __LINE__); \
    exit(1); \
  } \
} while(0)

struct testmsg {
  int msgid;
  int msgtype;
  int msglen;
  char data[512];
};

static void
test_basic_sender_recv(void) {
  int pid = fork();
  if(pid < 0) {
    printf("TEST FAILED: fork in test_basic_sender_recv\n");
    exit(1);
  }
  if(pid == 0) {
    // child as recver
    struct testmsg rmsg;
    int ret = recv(IPC_ANY_SENDER, &rmsg, 0); // flags 0 for block recv
    CHECK(ret==0, "basic_sender_recv: child recv fails\n");
    CHECK(rmsg.msgid==100, "basic_sender_recv: child get unexpected msg\n");
    CHECK(rmsg.msgtype==42, "basic_sender_recv: child get unexpected msg\n");
    CHECK(rmsg.msglen==11, "basic_sender_recv: child get unexpected msg\n");
    CHECK(strcmp(rmsg.data, "HelloWorld") == 0, "basic_sender_recv: data corruption\n");
    printf("basic_sender_recv child test passed\n");
    exit(0);
  } else {
    // parent send message
    struct testmsg smsg;
    smsg.msgid = 100;
    smsg.msgtype=42;

    char *txt = "HelloWorld";
    smsg.msglen = strlen(txt) + 1; // +1 for \0
    memmove(smsg.data, txt, smsg.msglen);

    int ret = send(pid, &smsg);
    CHECK(ret == 0, "basic_sender_recv: parent send fails\n");
    wait((int*)0);
    printf("basic_sender_recv parent test passed\n");
  }
}

static void
test_specific_sender_recv(void) {
  int childpid = fork();
  if(childpid < 0) {
    printf("TEST FAILED: fork in test_specific_sender_recv\n");
    exit(1);
  }
  if(childpid == 0) {
    // child as sender, send to specific id: parent
    struct testmsg smsg;
    smsg.msgid = 100;
    smsg.msgtype = 53;
    char *txt = "Send from child";
    smsg.msglen = strlen(txt) + 1;
    memmove(smsg.data, txt, smsg.msglen);

    int ret = send(getppid(), &smsg);
    CHECK(ret==0, "specific_sender_recv: child send fails\n");
    printf("specific_sender_recv child test passed\n");
    exit(0);
  } else {
    // parent as recver
    wait((int*)0);
    struct testmsg rmsg;
    
    int ret = recv(childpid, &rmsg, IPC_NONBLOCK);
    CHECK(ret==0, "specific_sender_recv: parent recv fails\n");
    CHECK(rmsg.msgid==100, "specific_sender_recv: parent get unexpected msg\n");
    CHECK(rmsg.msgtype==53, "specific_sender_recv: parent get unexpected msg\n");
    CHECK(rmsg.msglen==16, "specific_sender_recv: parent get unexpected msg\n");
    CHECK(strcmp(rmsg.data, "Send from child") == 0, "specific_sender_recv: data corruption\n");
    printf("specific_sender_recv parent test passed\n");
  }
}

static void
test_ipc_nonblocking(void) {
  struct testmsg rmsg;
  int ret = recv(IPC_ANY_SENDER, &rmsg, IPC_NONBLOCK);
  CHECK(ret<0, "nonblocking: expected failure on empty queue\n");
  printf("nonblocking: empty queue test passed\n");

  int childpid = fork();
  if(childpid < 0) {
    printf("TEST FAILED: fork in test_specific_sender_recv\n");
    exit(1);
  } 

  if(childpid == 0) {
    struct testmsg smsg;
    smsg.msgid = 300;
    smsg.msgtype = 55;
    char *txt = "Non Blocking Test";
    smsg.msglen = strlen(txt) + 1;
    memmove(smsg.data, txt, smsg.msglen);

    int ret2 = send(getppid(), &smsg);
    CHECK(ret2==0, "nonblocking: child send fails\n");
    printf("nonblocking: child test passed\n");
    exit(0);
  } else {
    wait((int*)0);
    sleep(5);
    ret = recv(childpid, &rmsg, IPC_NONBLOCK);
    CHECK(ret == 0, "nonblocking: parent recv after send failed");
    CHECK(rmsg.msgid == 300, "nonblocking: parent wrong msgid");
    CHECK(strcmp(rmsg.data, "Non Blocking Test") == 0, "nonblocking: parent payload mismatch");
    printf("nonblocking: parent test passed\n");
  }
}

int main(int argc, char** argv) {
  printf("=== Start IPC Que test ===\n");

  test_basic_sender_recv();
  test_specific_sender_recv();
  test_ipc_nonblocking();

  printf("=== All Test Passed ===\n");
  exit(0);
}
