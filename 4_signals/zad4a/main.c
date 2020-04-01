#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define SIZE 256

void handler(int signum, siginfo_t *info, void *ucontext) {
    if(signum == SIGUSR1) {
        if(info->si_code <= 0)
            printf("Signal SIGUSR1 generated by kill, raise, abort or alarm\n");
        else
            printf("Signal SIGUSR1 generated by the system\n");
    }
    else if(signum == SIGSEGV) {
        printf("Segmentation fault occurred at the address %p\n", info->si_addr);
        exit(1);
    }
    else if(signum == SIGCHLD)
        printf("Child process exit status: %d\n", info->si_status);
}

int main(int argc, char **argv) {
    if(argc < 3)
        perror("too few arguments");


    pid_t catcherPID = fork();

    if(catcherPID == 0) {
        char * const newArgv[] = {"catcher", argv[2], NULL};
        execvp("./catcher", newArgv);
    }

    pid_t senderPID = fork();

    if(senderPID == 0) {
        char *catcherPIDString = calloc(SIZE, sizeof(char));
        sprintf(catcherPIDString, "%d", catcherPID);

        char * const newArgv[] = {"sender", catcherPIDString , argv[1], argv[2], NULL};
        execvp("./sender", newArgv);
    }

    waitpid(catcherPID, NULL, 0);
    waitpid(senderPID, NULL, 0);


    return 0;
}
