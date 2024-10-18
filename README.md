CSE330 Project-4 Zombie Killer

In this directory, there are two scripts available for students testing convenience.

## [test_module.sh](https://github.com/visa-lab/CSE330-OS/blob/project-4/test_module.sh)

This script can be used to test the kernel module. It will do the following when provided the directory to your
source code, arguments to pass as the values to your module parameters, and the numbers of regular and zombie
processes to start:
 - Note, the reason we have it take a directory to your code rather than a zip file is to ease the testing process
during development by not requiring you to create a zip file just to test your code. This script is to be used
specifically during development.

It does the following, in this sequence:
- We will check for all files we expect to see
- We will compile your kernel module (and abort if the compilation fails - ***0 points***)
- We will launch the provided number of regular processes to run in the background
- We will load your kernel module (and abort if the compilation fails - ***0 points***)
- Zombie processes start in batches throughout runtime every few seconds
  - The total number of zombie processes will match the number provided
- We will check to make sure the number of kernel threads you have started is correct (***2 points possible***)
- We will give your kernel module some time to clean up all the zombies
- We will perform analysis on the processes which have been cleaned, and which still remain (***20 points possible***)
- We will unload your kernel module
- We will check to make sure all your kernel threads have stopped (***3 points possible***)

Note: All processes we start will run under a user which we will create within your VM named "TestP4". Don't worry,
the script will clean up after itself after it is done running.

### Usage and expected output:

Usage: Replace `/path/to/code/` with the directory which has your `producer_consumer.c` and `Makefile`:
```bash
Usage: ./test_module.sh /path/to/your/submission/ <prod> <cons> <size> <regular> <zombies>
 <prod>    - the number of producer threads for the kernel module
 <cons>    - the number of consumer threads for the kernel module
 <size>    - the size of the buffer for the kernel module
 <regular> - the number of regular processes to spawn for the process generator
 <zombies> - the number of zombie processes to spawn for the process generator
```

Expected output (from test case 3):
```
[log]: Creating user TestP4...
[log]: Look for Makefile
[log]: ─ file /home/vboxuser/git/test/GTA-CSE330-Fall2024/Project4/Makefile found
[log]: Look for source file (producer_consumer.c)
[log]: ─ file /home/vboxuser/git/test/GTA-CSE330-Fall2024/Project4/producer_consumer.c found
[log]: Compile the kernel module
[log]: ─ Compiled successfully
[log]: Starting 10 normal processes
[log]: Load the kernel module
[log]: ─ Loaded successfully
[log]: Starting zombie processes ...
[log]: ─ Total zombies spawned so far: 10/10
[log]: Checking the counts of the running kernel threads
[log]: ─ Found all expected threads
[log]: We will now wait some time to give your kernel module time to cleanup
[log]: └─ We will wait 10 seconds
[log]: Checking the pids of all remaining processes against your output
[log]: ─ All zombies were cleaned
[log]: ─ None of the regular processes were killed
[log]: ┬─ All zombies were cleaned
[log]: └─ None of the regular processes were killed
[log]: Unload the kernel module
[log]: ─ Kernel module unloaded sucessfully
[log]: Checking to make sure kthreads are terminated
[log]: ─ All threads have been stopped
[zombie_killer]: Passed
[final score]: 25.00/25
[log]: Deleting user TestP4...
```

## [test_zip_contents.sh](https://github.com/visa-lab/CSE330-OS/blob/project-4/test_zip_contents.sh)

This script is to be used to ensure the final submission adheres to the expected format specified in the project codument. It will do the following:

1. Unzip your submission into a directory `unzip_<unix_timestamp>/`
2. The script will check for all of the expected files within the `source_code` directory
3. The script will remove the directory it created `unzip_<unix_timestamp>`

Once the script is done running, it will inform you of the correctness of the submission by showing you anything it could not find.

Usage:
```
./test_zip_contents.sh /path/to/zip/file
```

Expected output:
```
[log]: Look for directory (source_code)
[log]: ─ file /home/vboxuser/git/test/GTA-CSE330-Fall2024/Project4/testing/unzip_1729234187/source_code found
[log]: Look for Makefile
[log]: ─ file /home/vboxuser/git/test/GTA-CSE330-Fall2024/Project4/testing/unzip_1729234187/source_code/Makefile found
[log]: Look for source file (producer_consumer.c)
[log]: ─ file /home/vboxuser/git/test/GTA-CSE330-Fall2024/Project4/testing/unzip_1729234187/source_code/producer_consumer.c found
[test_zip_contents]: Passed
```

## [utils.sh](https://github.com/visa-lab/CSE330-OS/blob/project-4/utils.sh)

This script is not meant to be run directly, and only contains code that is used across both scripts mentioned above.
- Please do not make any changes in provided test case code to pass the test cases.
- You can use print statements in case you want to debug and understand the logic of the test code.
- Please get in touch with the TAs if you face any issues using the test scripts.
