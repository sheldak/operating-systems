#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "utilities.h"

int queueID = -1;

void termination() {
    if(msgctl(queueID, IPC_RMID, NULL) == 0)
        printf("Message queue removed successfully\n");
    else
        perror("Error when removing message queue");

    exit(0);
}

void handlerINT(int signum) {
    termination();
}

int main() {
//    if (setvbuf(stdout, NULL, _IONBF, 0) != 0) {
//        printf("Error: buffering mode could not be changed!\n");
//        exit(1);
//    }

    // deleting message queue before process termination
    if(atexit(termination) != 0)
        perror("atexit function error");

    // deleting message queue in case of SIGINT signal
    if (signal(SIGINT, handlerINT) == SIG_ERR)
        perror("signal function error");

    char *path = getenv("HOME");
    if(!path)
        perror("Cannot get path by getenv function");

    key_t queueKey = ftok(path, PROJECT_ID);
    if(queueKey == -1)
        perror("Cannot get key by ftok function");

    queueID = msgget(queueKey, 0666 | IPC_CREAT | IPC_EXCL);
    if(queueID == -1)
        perror("Cannot get queue ID by msgget function");

    size_t length = 0;
    char *buffer = malloc(SIZE * sizeof(char));

    while(1) {
        message *msgBuffer = malloc(sizeof(message));

        msgrcv(queueID, msgBuffer, sizeof(&msgBuffer), RECEIVE_MTYPE, 0);

        if(msgBuffer->type == INIT) {
            msgsnd(msgget(msgBuffer->queueKey, 0666), msgBuffer, sizeof(&msgBuffer), 0);
            printf("Sent INIT\n");
        }
        break;

    }

    return 0;
}