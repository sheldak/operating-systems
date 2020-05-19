#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define SIZE 256

int main(int argc, char **argv) {
    if(argc < 3)
        perror("too few arguments");

    // making child process for catcher
    pid_t catcherPID = fork();

    if(catcherPID == 0) {
        char * const newArgv[] = {"catcher", argv[2], NULL};
        execvp("./catcher", newArgv);
    }

    // making child process for sender
    pid_t senderPID = fork();

    if(senderPID == 0) {
        char *catcherPIDString = calloc(SIZE, sizeof(char));
        sprintf(catcherPIDString, "%d", catcherPID);

        char * const newArgv[] = {"sender", catcherPIDString , argv[1], argv[2], NULL};
        execvp("./sender", newArgv);
    }

    // waiting for child processes to the end of sending signals
    waitpid(catcherPID, NULL, 0);
    waitpid(senderPID, NULL, 0);

    return 0;
}
