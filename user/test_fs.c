#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "user/fcntl.h"

int main(void) {
  int fd;
  char buf[100];
  struct stat st;
  int n;

  printf("FS test: create and write file\n");
  fd = open("hello.txt", O_CREATE | O_RDWR);
  if(fd < 0) {
    printf("error: open for create failed\n");
  }

  debug_msg("start fs_test:");
  const char *msg = "hello, Megan Kuo!\n";
  int len = strlen(msg);
  n = write(fd, msg, len);
  if(n != len) {
    printf("error: write failed, wrote %d bytes (expected %d)\n", n, len);
    exit(1);
  }
  printf("FS_test: write success\n");
  close(fd);

  printf("FS test: read file and verify content\n");
  fd = open("hello.txt", O_RDONLY);
  if(fd < 0) {
    printf("error: open for read failed\n");
    exit(1);
  }
  memset(buf, 0, sizeof(buf));
  n = read(fd, buf, sizeof(buf));
  if(n != len || strcmp(buf, msg) != 0) {
    printf("error: read content mismatch\n");
    printf("got '%s', expected '%s'\n", buf, msg);
    exit(1);
  }
  printf("Read back content: %s\n", buf);

  if(fstat(fd, &st) < 0) {
    printf("error: fstat failed\n");
  } else {
    printf("File size is %ld bytes, nlink %d, type %d\n", st.size, st.nlink, st.type);
  }
  close(fd);

  printf("FS test completed successfully\n");
  exit(0);
  return 0;
}

