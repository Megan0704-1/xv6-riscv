// [New] service_registry.h
#ifndef SERVICE_REGISTRY_H
#define SERVICE_REGISTRY_H

#define MAX_SERVICES 16
#define SERVICE_NAME_LEN 16

struct service_entry {
  char name[SERVICE_NAME_LEN];
  int pid; 
};

// service_registry stores  max services of service_entries
extern struct service_entry service_registry[MAX_SERVICES];

// init
void service_registry_init(void);

// register a service with given name and PID
// return 0: success; 1: fail
int register_service(const char *name, int pid);

// look up service by name
// return service PID
int lookup_service(const char *name);

#endif // SERVICE_REGISTRY_H
