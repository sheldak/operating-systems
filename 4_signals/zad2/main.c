#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <ftw.h>
#include <sys/wait.h>

#define SIZE 256

void handlerUSR1(int signum) {
    pid_t pid = getpid();
    printf("Process %d caught SIGUSR1 signal\n", pid);
}

int main(int argc, char **argv) {
    if(argc < 2)
        perror("too few arguments");

    char *arguments[5] = {"ignore", "handler", "mask", "pending", "exec"};

    if(argc == 3 && strcmp(argv[2], arguments[4]) == 0) {
        printf("\"exec\" function called\n");

        if(strcmp(argv[1], arguments[3]) == 0) {
            pid_t pid = getpid();
            sigset_t signalsPending;
            sigpending(&signalsPending);

            if(sigismember(&signalsPending, SIGUSR1) == 1)
                printf("Signal SIGUSR1 is pending in process %d\n", pid);
            else if(sigismember(&signalsPending, SIGUSR1) == 0)
                printf("Signal SIGUSR1 is not pending in process %d\n", pid);
        }
        else
            raise(SIGUSR1);

        printf("\n");
    }
    else {
        printf("Parent PID: %d\n", getpid());
        if(strcmp(argv[1], arguments[0]) == 0)  // ignoring signal
            signal(SIGUSR1, SIG_IGN);
        else if(strcmp(argv[1], arguments[1]) == 0)  // handling signal
            signal(SIGUSR1, handlerUSR1);
        else if(strcmp(argv[1], arguments[2]) == 0 || strcmp(argv[1], arguments[3]) == 0) {  // masking signal
            sigset_t mask;
            sigemptyset(&mask);
            sigaddset(&mask, SIGUSR1);

            if (sigprocmask(SIG_SETMASK, &mask, NULL) < 0)
                perror("Cannot set mask");
        }
        else
            perror("invalid argument");

        raise(SIGUSR1);

        // checking if signal is pending (just for "pending" argument)
        if(strcmp(argv[1], arguments[3]) == 0) {
            pid_t pid = getpid();
            sigset_t signalsPending;
            sigpending(&signalsPending);

            if(sigismember(&signalsPending, SIGUSR1) == 1)
                printf("Signal SIGUSR1 is pending in process %d\n", pid);
            else if(sigismember(&signalsPending, SIGUSR1) == 0)
                printf("Signal SIGUSR1 is not pending in process %d\n", pid);
        }

        pid_t pid = fork();

        if(pid == 0) {
            pid = getpid();
            printf("Child PID: %d\n", pid);

            if(strcmp(argv[1], arguments[3]) == 0) {
                sigset_t signalsPending;
                sigpending(&signalsPending);

                if(sigismember(&signalsPending, SIGUSR1) == 1)
                    printf("Signal SIGUSR1 is pending in child process %d\n", pid);
                else if(sigismember(&signalsPending, SIGUSR1) == 0)
                    printf("Signal SIGUSR1 is not pending in child process %d\n", pid);
            }
            else
                raise(SIGUSR1);
        }
        else {
            if(strcmp(argv[1], arguments[1]) != 0) {
                char * const newArgv[] = {"main", argv[1], "exec"};
                execvp("./main", newArgv);
            }
        }
    }

    printf("\n");
    return 0;
}
