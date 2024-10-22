## Difference between UID and PID in linux kernel programming
### What is PID, process ID?
The PID is a unique identifier the kernel assigned to each process when it is created.
It is used for kernel to manage processes and refer to them in syscalls and user space programs.
> Operations
> - Sending signals to processes.
> - Waiting for a process to complete (`waitpid`)
> - Getting process info
**PID is just an identifier**

### What is UID, user ID?
UID identifies the user who owns the process and is used to determine what system resources (credentials) the process can access based on the user's permissions.
- UID is tied to the user, and multiple processes can have the same UID.
e.g., Root has UID 0

### Example
If you logged in the server as Megan with UID=1000, and run a process like chrome, the process will have the UID of 1000 and be assigned with a unique PID (say 1234).
```
ps -u Megan
```
PID 1234 is the identifier for the instance of chrome, and UID 1000 is the owner of the process.

## What is process credentials?
Process 

### Synchronization
- Producer
    - Acquire lock before accessing the buffer.
    - Use `wait_event_interruptible` on `producer` queue if buffer is full.
- Consumer
    - Acquire lock before popping from the buffer.
    - Use `wait_event_interruptible` on `consumer` queue if buffer is empty.

### Logic
- Producer
    - Scans the process list (task_struct) for zombie processes owned by given uid and inserts them to the shared buffer.
- Consumer
    - Get the zombie out from the shared buffer and call kill_zombie function to remove it.
- Kill_zombie function
    - Checks if the zombie process's parent is init process, if not, it re-parents the task to the init process, removes it from the current parent's child list and signals the init process to handle the task.
