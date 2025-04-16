#define NPROC        64  // maximum number of processes
#define NCPU          8  // maximum number of CPUs
#define NOFILE       16  // open files per process
#define NFILE       100  // open files per system
#define NINODE       50  // maximum number of active i-nodes
#define NDEV         10  // maximum major device number
#define ROOTDEV       1  // device number of file system root disk
#define MAXARG       32  // max exec arguments
#define MAXOPBLOCKS  10  // max # of blocks any FS op writes
#define LOGSIZE      (MAXOPBLOCKS*3)  // max data blocks in on-disk log
#define NBUF         (MAXOPBLOCKS*3)  // size of disk block cache
#define FSSIZE       2000  // size of file system in blocks
#define MAXPATH      128   // maximum file path name
#define USERSTACK    1     // user stack pages

#define IPC_QUEUE_LIMIT 32                // [New] Max num of elements in the queue
#define IPC_MAX_PAYLOAD 4096              // [New] 4KB
#define IPC_ANY_SENDER -1                 // [New] any sender repr. by -1
                                          
#define IPC_WAITING 0x1                   // [New] process is sleep-waiting for a msg
#define IPC_NONBLOCK 0x2                  // [New] non-blocking recv
                                         
#define IPC_HEADER_SIZE (sizeof(int) * 3) // [New] Remove fixed size MSGSIZE.
                                          // The IPC header consists of 3 ints:
                                          // 1. msgid, 2. type, 3. len of bytes

