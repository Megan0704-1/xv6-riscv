# Moving FS and Drivers to user space
Note for FS.
- With FS and drivers in US, the system call impl in the kernel must be reworked as thin IPC forwarders.
- Find a way to shuttle messages between client process and server processes.
- Find a way to pacakge the request to current syscalls (sys_open, sys_read, ...), and send it to server.

# IPC Workflow
Consider ```open("/READNE", O_RDONLY);```
1. The user process traps to the kernel, calling sys_open.
2. Kernel sys_open gathers arguments (path, access controls)
3. Kernel creates a IPC Request for type FS_OPEN containing the arguments, AND the caller pid.
4. Kernel sends this to the FS server (a globally accessible PID), sleeps the current process.
5. The FS server in user space unblocks by ipc_recv, parse arguemtns and perform open:
    - Look up the inode for "/README"
    - allocates an internal open files structure for it
    - Reads metadata
    - Assign handle number (5) to this open file.
6. The FS server then prepares the IPC Reply with status=0, and the handle. Using ipc_send to send back to the user.
7. The kernel wakes the client process, fills its register with the return value.
8. **Kernel** needs to return a file descriptor to the user.
    - fdalloc, we can get a free fd number (3) in the process
    - process file table will be like "fd" = (3, {server=FS, handle=5})

# Miscellaneous
- change kalloc for sysipc::sys_send to fine-grained control

# Note.
Bmaps
1. bmap function:
"Block map": maps a file block to the device-relative block (on disk)
find in fs.c

2. bitmap data:
The on-disk "bit-map": records which disk blocks are free (bit=0) or allocated (bit=1)
one bit per block.
find in superblock (describe disk layout) fields like bmapstart

### Example.
bitmap‑block #0 (sb.bmapstart)          tracks data blocks 0 … 8191
bitmap‑block #1 (sb.bmapstart + 1)      tracks data blocks 8192 … 16383

When the kernel wants to know “is data block 42 137 free?”, it:
1.	Computes
```bitmap_block = BBLOCK(42137, sb)``` → which of the eight bitmap blocks.
2.	Reads that bitmap block (if not cached).
3.	Looks at bit 42137 % 8192 within that 1 KiB buffer.
4.	Sets or clears the bit and, if it changed, writes that same bitmap block back.
