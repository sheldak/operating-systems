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
int clientsQueues[MAX_CLIENTS];
int unconnectedClients[MAX_CLIENTS];

void termination() {
    // deleting message queue
    if(msgctl(queueID, IPC_RMID, NULL) == 0)
        printf("\nMessage queue removed successfully\n");
    else
        perror("\nError when removing message queue");

    // terminating process
    exit(0);
}

void handleINTSignal(int signum) {
    // terminating process (it will call termination function because of atexit)
    exit(0);
}

void handleSTOP(message *msg) {
    // removing client from array with clients
    clientsQueues[msg->clientID] = -1;

    // setting client as not unconnected (it will not exist so should not be able to connect with other clients)
    unconnectedClients[msg->clientID] = 0;

    if(msg->clientID == -1)
        printf("Request for adding new client denied. List is full\n");
    else
        printf("Client with ID %d removed\n", msg->clientID);
}

void handleLIST(message *msg) {
    // message which will be send to the new client
    message *response = malloc(sizeof(message));
    response->type = LIST;

    // copying array
    for(int i=0; i<MAX_CLIENTS; i++)
        response->unconnectedClients[i] = unconnectedClients[i];

    if(msgsnd(clientsQueues[msg->clientID], response, MESSAGE_SIZE, 0) < 0) perror("Cannot send LIST");
}

void handleINIT(message *msg) {
    // looking for place for the new client in array with all clients and if it is, adding client to unconnected clients
    int newClientID = -1;
    for(int i=0; i<MAX_CLIENTS && newClientID == -1; i++) {
        if(clientsQueues[i] == -1) {
            newClientID = i;
            clientsQueues[i] = msgget(msg->queueKey, 0666);
            unconnectedClients[i] = 1;
        }
    }

    // message which will be send to the new client
    message *response = malloc(sizeof(message));

    // if there is no space for more clients, send STOP message to the new one to terminate it
    if(newClientID == -1) {
        int clientQueueID = msgget(msg->queueKey, 0666);

        response->type = STOP;

        if(msgsnd(clientQueueID, response, MESSAGE_SIZE, 0) < 0) perror("Cannot send STOP");
    }
    // if there is enough space for the new client, send INIT message with its ID to it
    else {
        response->type = INIT;
        response->clientID = newClientID;

        if(msgsnd(clientsQueues[newClientID], response, MESSAGE_SIZE, 0) < 0) perror("Cannot send INIT");
        printf("Added new client with ID %d\n", newClientID);
    }
}

void handleMessage(message *msg) {
    if(msg->type == STOP)
        handleSTOP(msg);
    else if(msg->type == LIST)
        handleLIST(msg);
    else if(msg->type == INIT)
        handleINIT(msg);
}

int main() {
    if (setvbuf(stdout, NULL, _IONBF, 0) != 0) {
        printf("Error: buffering mode could not be changed!\n");
        exit(1);
    }

    // deleting message queue before process termination
    if(atexit(termination) != 0) perror("atexit function error");

    // deleting message queue in case of SIGINT signal
    if (signal(SIGINT, handleINTSignal) == SIG_ERR) perror("signal function error");

    // initialing array with clients queues IDs and array of unconnected clients
    for(int i=0; i<MAX_CLIENTS; i++) {
        clientsQueues[i] = -1;
        unconnectedClients[i] = 0;
    }

    // getting key for message queue
    key_t queueKey = ftok(getenv("HOME"), PROJECT_ID);
    if(queueKey == -1) perror("Cannot get key by ftok function");

    // getting ID of message queue
    queueID = msgget(queueKey, 0666 | IPC_CREAT | IPC_EXCL);
    if(queueID == -1) perror("Cannot get queue ID by msgget function");

    // handling messages from clients
    while(1) {
        message *msgBuffer = malloc(sizeof(message));

        msgrcv(queueID, msgBuffer, MESSAGE_SIZE, RECEIVE_MTYPE, 0);

        handleMessage(msgBuffer);

        free(msgBuffer);
    }

    return 0;
}