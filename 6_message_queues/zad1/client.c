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

int ID = -1;

void termination() {
    // deleting message queue
    if(msgctl(queueID, IPC_RMID, NULL) == 0)
        printf("Message queue removed successfully\n");
    else
        perror("Error when removing message queue");

    // terminating process
    exit(0);
}

void setServerQueueID() {
    // getting server message queue key
    key_t serverKey = ftok(getenv("HOME"), PROJECT_ID);
    if(serverKey == -1) perror("Cannot get server key by ftok function");

    // setting server message queue ID
    serverQueueID = msgget(serverKey, 0666);
    if(serverQueueID == -1) perror("Cannot get server queue ID by msgget function");
}

void sendSTOP() {
    // making STOP message for the server
    message *msg = malloc(sizeof(message));
    msg->type = STOP;
    msg->clientID = ID;

    // sending STOP message to the server
    if(msgsnd(serverQueueID, msg, sizeof(&msg), 0) < 0) perror("Cannot send STOP");

    // freeing memory
    free(msg);

    printf("\nClient terminated!\n");

    // terminating process (it will call termination function because of atexit)
    exit(0);
}

void handleLIST(message *msg) {
    printf("You can connect with: ");
    for(int i=0; i<MAX_CLIENTS; i++) {
        if(msg->unconnectedClients[i] == 1)
            printf("%d ", i);
    }
    printf("\n\n");
}

void handleINIT(message *msg) {
    // setting client's ID
    ID = msg->clientID;

    printf("Client registered!\n");
}

int receiveMessage(int useNOWAIT) {
    // returns 1 if any message was handled
    int toReturn = 1;

    // creating message buffer
    message msgBuffer;

    // receiving message from server
    if(useNOWAIT == 1) {
        if(msgrcv(queueID, &msgBuffer, sizeof(msgBuffer), RECEIVE_MTYPE, IPC_NOWAIT) < 0)
            toReturn = 0;
    }
    else {
        if(msgrcv(queueID, &msgBuffer, sizeof(msgBuffer), RECEIVE_MTYPE, 0) < 0)
            perror("Cannot receive any message");
    }

    // handling message
    if(msgBuffer.type == STOP)
        sendSTOP();
    else if(msgBuffer.type == LIST)
        handleLIST(&msgBuffer);
    else if(msgBuffer.type == INIT)
        handleINIT(&msgBuffer);

    return toReturn;
}

void sendLIST() {
    // making LIST message for the server
    message *msg = malloc(sizeof(message));
    msg->type = LIST;
    msg->clientID = ID;

    // sending LIST message to the server
    if(msgsnd(serverQueueID, msg, sizeof(&msg), 0) < 0) perror("Cannot send STOP");

    // receiving array with unconnected clients
    receiveMessage(0);

    // freeing memory
    free(msg);
}

void registerClient() {
    // creating INIT message
    message *msg = malloc(sizeof(message));
    msg->type = INIT;
    msg->queueKey = queueKey;

    // sending INIT message to the server
    if(msgsnd(serverQueueID, msg, sizeof(&msg), 0) < 0) perror("Cannot send INIT");

    // handling message
    receiveMessage(0);

    // freeing memory
    free(msg);
}

void handleINTSignal(int signum) {
    // sending STOP message to the server and terminating process
    sendSTOP();
}

void handleCommission(char *commission, int length) {
    char *token = strtok(commission, " \n");
    if(strcmp(token, "list") == 0)
        sendLIST();
}

int main() {
    if (setvbuf(stdout, NULL, _IONBF, 0) != 0) {
        printf("Error: buffering mode could not be changed!\n");
        exit(1);
    }

    // deleting message queue before process termination
    if(atexit(termination) != 0)
        perror("atexit function error");

    // deleting message queue in case of SIGINT signal
    if (signal(SIGINT, handleINTSignal) == SIG_ERR) perror("signal function error");

    // getting key for message queue
    queueKey = ftok( getenv("HOME"), getpid());
    if(queueKey == -1) perror("Cannot get key by ftok function");

    // getting message queue ID
    queueID = msgget(queueKey, 0666 | IPC_CREAT | IPC_EXCL);
    if(queueID == -1) perror("Cannot get queue ID by msgget function");

    // getting server's message queue ID and saving it to the global variable
    setServerQueueID();

    // sending INIT message to the server
    registerClient();

    size_t length = 0;

    while(1) {
        while(receiveMessage(1) == 1) {}

        char *commission = malloc(SIZE * sizeof(char));

        printf("Type commission:\n");
        getline(&commission, &length, stdin);

        handleCommission(commission, length);

        free(commission);
    }


    return 0;
}
