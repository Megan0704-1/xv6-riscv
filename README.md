# CSE330 Project-5 Memory Manager

In this directory, there are two scripts available for students testing convenience.

## test_module.py

Before using this script, you must compile a C source, `testp5.c` file into a binary, `testp5`. This binary
takes a scalar value to denote how much memory to allocate. The `testp5` program will allocate a large chunk
of memory on the heap and then access each page to bring it present into memory. Since the memory allocation
is large, some of the pages accessed will be moved to disk. This binary is invoked by `test_module.py` so you
do not need to run it yourself but you are welcome to do so.
- Note, you ***MUST*** configure your virtual machine to use 4GB of memory and use 4 for the value of the scalar. This
is to ensure that the test script will be able to generate a sufficient amount of swapped pages from which various
virtual addresses will be provided to your kernel module.

You can compile `testp5` using the provided `Makefile` by simply running the following command:
```bash
make
```

This script can be used to test the kernel module. It will do the following when provided the path to your
compiled kernel module (i.e., a .ko file kernel object), a scalar value to denote how much memory to allocate,
and the number of present, swapped, and invalid pages to test:
1. We will start the `testp5` program
2. We will wait 5 seconds to give the OS some time to move pages to swap
3. We will test a number of random virtual addresses which lie within present pages in memory
4. We will test a number of random virtual addresses which lie within swapped pages in memory
5. We will test a number of random invalid virtual addresses which do not lie within any page in memory

Your kernel module will be loaded and unloaded for each address tested. If for any reason the kernel module either
fails to load or unload, the script will stop testing and you will be left only with the total points you have
accumulated so far.

Since this script reads `/var/log/kern.log` and multiple files from procfs to validate your output, it ***MUST*** be run with `sudo`.

### Usage and expected output:

Usage: Replace `/path/to/memory_manager.ko` with the path to your compiled kernel module:
```bash
Usage: sudo ./test_module.py /path/to/memory_manager.ko <scalar> <present> <swapped> <invalid>
 - scalar  : You MUST set your VM to use 4GB memory to ensure the test scripts can generate swap pages. Use 4 for this argument.
 - present : The number of present addresses to test.
 - swapped : The number of swapped addresses to test.
 - invalid : The number of invalid addresses to test.
This script MUST be run as root and you MUST have compiled your kernel module before running.
```

Expected output: (Testing 100 pages present in memory)
```
[log]: Waiting for 5 seconds to allow time for pages to be moved to swap
[log]: Checking 100 random present pages
[log]: - 100/100 correct
[log]: Checking 100 random swapped pages
[log]: - 0/0 correct
[log]: Checking 100 random invalid pages
[log]: - 0/0 correct
[memory_manager]: Passed (100.0/100)
```

Feel free to run with any number of present, swapped, or invalid tests. This script is intended to give you the flexibility to
test each one in isolation (i.e. if you want to check 10 addresses from pages in memory, 0 addresses from pages in swap, and
0 invalid addresses to test one thing at a time on a smaller scale, you are able to do so).

## test_zip_contents.sh

This script is to be used to ensure the final submission adheres to the expected format specified in the project codument. It will do the following:

1. Unzip your submission into a directory `unzip_<unix_timestamp>/`
2. The script will check for all of the expected files within the `source_code` directory
3. The script will remove the directory it created `unzip_<unix_timestamp>`

Once the script is done running, it will inform you of the correctness of the submission by showing you anything it could not find.

Usage:
```bash
./test_zip_contents.sh /path/to/zip/file
```

Expected output:
```
[log]: Look for directory (source_code)
[log]: ─ file /home/vboxuser/git/GTA-CSE330-Fall2024/Project5/test/unzip_1730664481/source_code found
[log]: Look for Makefile
[log]: ─ file /home/vboxuser/git/GTA-CSE330-Fall2024/Project5/test/unzip_1730664481/source_code/Makefile found
[log]: Look for source file (memory_manager.c)
[log]: ─ file /home/vboxuser/git/GTA-CSE330-Fall2024/Project5/test/unzip_1730664481/source_code/memory_manager.c found
[test_zip_contents]: Passed
```
