## CSE330 Project-2 Kernel Module and System Call

In this directory, there are two scripts available for student testing convenience.

### [test_module.sh](https://github.com/visa-lab/CSE330-OS/blob/project-2/test_module.sh)

This script can be used to test the kernel module. It will do the following after unzipping the provided zip file:

![image](https://github.com/user-attachments/assets/19985d13-289a-4071-837e-7aee92abb216)

#### Usage and expected output:

Usage:
```bash
./test_module.sh /path/to/zip/file.zip
```

Expected Output:
```
Unzipping to "/path/to/zip/file.zip"
[log]: Look for kernel_module directory
[log]: Look for Makefile
[log]: Look for source file (my_name.c)
[log]: Compile the kernel module
[log]: Load the kernel module
[sudo] password for vboxuser: 
[log]: Check dmesg output
[log]: Unload the kernel module
[my_name]: Passed with 50 out of 50
[Total Score]: 50 out of 50
```

### [test_syscall.sh](https://github.com/visa-lab/CSE330-OS/blob/project-2/test_syscall.sh)

This script can be used to test the syscall. It will do the following after unzipping the provided zip file:

![image](https://github.com/user-attachments/assets/e58011fa-249f-492d-a2bf-59d538ad1884)

#### Usage and expected output:

Usage:
```bash
./test_syscall.sh /path/to/zip/file.zip
```

Expected Output:
```
Unzipping to "/tmp/demo/demo.zip"
[log]: Look for kernel_syscall directory
[log]: Look for Makefile
[log]: Look for source file (my_syscall.c)
[log]: Look for userspace directory
[log]: Look for source file (syscall_in_userspace_test.c)
[log]: Look for screenshots directory
[log]: Look for syscall_output screenshot
[my_syscall]: Passed with 50 out of 50
[Total Score]: 50 out of 50 (this is NOT your final grade)
NOTE: This script does not check the content of your screenshot.
      Check grading rubric for details, the grader will inspect
      the text within the screenshots.
```

Note, the autograder script will need to deduct points from the points given in this script if the screenshot content is incorrect.

### [utils.sh](https://github.com/CSE330-OS/GTA-CSE330-Fall2024/blob/main/Project2/testing/utils.sh)

This script is not meant to be run directly, and only contains code that is used in both scripts mentioned above.

### Note

- Please do not make changes in provided test code to pass the test cases.
- You can use print statements in case you want to debug and understand the logic of the test code (the diagrams may also be useful).
- Please get in touch with the TAs if you face issues in using the test scripts.
