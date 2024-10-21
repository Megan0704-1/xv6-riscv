#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    pid_t pid = fork();
    if(pid > 0) {
        // parent
        sleep(90);
    } else if(pid == 0) {
        // child
        exit(0);
    } else {
        perror("fork");
        exit(1);
    }
    return 0;
}
