// System call numbers
#define SYS_fork    1
#define SYS_exit    2
#define SYS_wait    3
#define SYS_pipe    4
#define SYS_read    5
#define SYS_kill    6
#define SYS_exec    7
#define SYS_fstat   8
#define SYS_chdir   9
#define SYS_dup    10
#define SYS_getpid 11
#define SYS_sbrk   12
#define SYS_sleep  13
#define SYS_uptime 14
#define SYS_open   15
#define SYS_write  16
#define SYS_mknod  17
#define SYS_unlink 18
#define SYS_link   19
#define SYS_mkdir  20
#define SYS_close  21

#define SYS_send 22 // [New] send(pid, msg)
#define SYS_recv 23 // [New] recv(pid, msg, flags)

#define SYS_getppid 24 // [New] getppid()

#define SYS_register_service 25 // [New] register_service(name, pid)
#define SYS_lookup_service 26 // [New] lookup_service(name)

#define SYS_disk_read 27  // [New] disk read
#define SYS_disk_write 28 // [New] disk write
                         
#define SYS_debug 29
#define SYS_debug_msg 30

#define SYS_console_read 31
#define SYS_console_write 32
