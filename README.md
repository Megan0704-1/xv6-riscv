## CSE330 Project-3 Kernel Threads and Kernel-space Shared-memory IPC

In this directory, there are two scripts available for student testing convenience.

## [test_module.sh](https://github.com/visa-lab/CSE330-OS/blob/project-3/test_module.sh)

This script can be used to test the kernel module. It will do the following when provided the directory to your source code and arguments to pass as the values for your module parameters:
 - Note, the reason we have it take a directory to your code rather than a zip file is to ease the testing process during development by not requiring you to create a zip file just to test your code. This script is to be used specifically during development.

![image](https://github.com/user-attachments/assets/1f4bade3-61aa-4ec1-a1df-8c8a96710477)


### Usage and expected output:

Usage: Replace `/path/to/code/` with the directory which has your `producer_consumer.c` and `Makefile`, replace `prod` with the number of producers you would like there to be, replace `cons` with the number of consumers you would like there to be, and replace `size` with the value you want used for the size.
```bash
./test_module.sh /path/to/code/ prod cons size
```

Expected Output (tested with 5 producers, 5 consumers, and a size of 5): 
```
Testing your kernel module with 5 producers, 5 consumers, and a size of 5:
[log]: Look for Makefile
[log]: - file /home/vboxuser/Project3/Makefile found
[log]: Look for source file (producer_consumer.c)
[log]: - file /home/vboxuser/Project3/producer_consumer.c found
[log]: Compile the kernel module
[log]: - Compiled successfully
[log]: Load the kernel module
[log]: - Loaded successfully
[log]: - Found all expected threads
[log]: Checking output
[log]: - Output is correct
[log]: Unload the kernel module
[log]: - Kernel module unloaded sucessfully
[log]: Checking to make sure kthreads are terminated
[log]: - All threads have been stopped
[producer_consumer]: Passed with 20 out of 20
[Total Score]: 20 out of 20
```

## [test_zip_contents.sh](https://github.com/visa-lab/CSE330-OS/blob/project-3/test_zip_contents.sh)

This script is to be used to ensure the final submission adheres to the expected format specified in the project codument. It will do the following:

1. Unzip your submission into a directory `unzip_<unix_timestamp>/`
2. The script will check for all of the expected files within the `source_code` directory
3. The script will remove the directory it created `unzip_<unix_timestamp>`

Once the script is done running, it will inform you of the correctness of the submission by showing you anything it could not find.

Usage:
```
./test_zip_contents.sh /path/to/zip/file
```

Expected Output:
```
[log]: Look for directory (source_code)
[log]: - directory /home/vboxuser/git/CSE330-OS/unzip_1727299910/source_code found
[log]: Look for Makefile
[log]: - file /home/vboxuser/git/CSE330-OS/unzip_1727299910/source_code/Makefile found
[log]: Look for source file (producer_consumer.c)
[log]: - file /home/vboxuser/git/CSE330-OS/unzip_1727299910/source_code/producer_consumer.c found
[test_zip_contents]: Passed
```

## [utils.sh](https://github.com/visa-lab/CSE330-OS/blob/project-3/utils.sh)

This script is not meant to be run directly, and only contains code that is used across both scripts mentioned above.
- Please do not make any changes in provided test case code to pass the test cases.
- You can use print statements in case you want to debug and understand the logic of the test code.
- Please get in touch with the TAs if you face any issues using the test scripts.
