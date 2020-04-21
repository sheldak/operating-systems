#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>

#include "utilities.h"

key_t queueKey = -1;
int queueID = -1;

int serverQueueID = -1;

void termination() {
    if(msgctl(queueID, IPC_RMID, NULL) == 0)
        printf("Message queue removed successfully\n");
    else
        perror("Error when removing message queue");

    exit(0);
}

void handleINT(int signum) {
    termination();
}

void setServerQueueID() {
    key_t serverKey = ftok(getenv("HOME"), PROJECT_ID);
    if(serverKey == -1)
        perror("Cannot get server key by ftok function");

    serverQueueID = msgget(serverKey, 0666);
    if(serverQueueID == -1)
        perror("Cannot get server queue ID by msgget function");
}

void sendInit() {
    message *msg = malloc(sizeof(message));
    msg->type = INIT;
    msg->queueKey = queueKey;

    if (msgsnd(serverQueueID, msg, sizeof(&msg), 0) < 0)
        perror("Cannot send message INIT");
}

int main() {
    // deleting message queue before process termination
    if(atexit(termination) != 0)
        perror("atexit function error");

    // deleting message queue in case of SIGINT signal
    if (signal(SIGINT, handleINT) == SIG_ERR)
        perror("signal function error");

    queueKey = ftok( getenv("HOME"), getpid());
    if(queueKey == -1)
        perror("Cannot get key by ftok function");

    queueID = msgget(queueKey, 0666 | IPC_CREAT | IPC_EXCL);
    if(queueID == -1)
        perror("Cannot get queue ID by msgget function");

    setServerQueueID();

    sendInit();

    size_t length = 0;
    char *commission = malloc(SIZE * sizeof(char));

    while(1) {
        struct message *msgBuffer = malloc(sizeof(struct message*));

        printf("Type commission:\n");
//        getline(&commission, &length, stdin);

        msgrcv(queueID, msgBuffer, sizeof(&msgBuffer), RECEIVE_MTYPE, 0);
        if(msgBuffer->type == INIT) {
            printf("got INIT\n", msgBuffer->type);
            break;
        }

    }


    return 0;
}
