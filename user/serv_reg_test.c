#include "user/user.h"

#define FAIL(msg) do { \
  printf("SERVICE_REGISTRY_TEST FAILED: %s\n", msg); \
  exit(1); \
} while(0)

#define CHECK(cond, msg) do { \
  if(!(cond)) FAIL(msg); \
} while(0)

int
main(void) 
{
  int ret, pid = getpid();

  printf("=== service_registry_test starting (pid=%d) ===\n", pid);

  ret = lookup_service("dummy");
  CHECK(ret < 0, "look up of unregistered service should return -1");

  ret = register_service("dummy1", pid);
  CHECK(ret == 0, "first register service (\"dummy1\") should succeed.");

  ret = lookup_service("dummy1");
  CHECK(ret == pid, "lookup service (\"dummy1\") should return registerd pid");

  ret = register_service("dummy1", pid);
  CHECK(ret < 0, "duplicate register service should fail");

  printf("=== service_registry_test PASSED ===\n");
  exit(0);
}

