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
int clientsQueueKeys[MAX_CLIENTS];
int clientsQueues[MAX_CLIENTS];
int unconnectedClients[MAX_CLIENTS];
int clientsPIDs[MAX_CLIENTS];

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

void handleDISCONNECT(message *msg) {
    unconnectedClients[msg->clientID] = 1;
    unconnectedClients[msg->toConnectID] = 1;

    printf("%d and %d disconnected\n", msg->clientID, msg->toConnectID);
}

void handleLIST(message *msg) {
    // message which will be send to the new client
    message *response = malloc(sizeof(message));
    response->type = LIST;

    // copying array
    for(int i=0; i<MAX_CLIENTS; i++)
        response->unconnectedClients[i] = unconnectedClients[i];

    // to not send requesting client as available to itself
    response->unconnectedClients[msg->clientID] = 0;

    if(msgsnd(clientsQueues[msg->clientID], response, MESSAGE_SIZE, 0) < 0) perror("Cannot send LIST");
}

void handleCONNECT(message *msg) {
    // messages which will be send to clients
    message *response = malloc(sizeof(message));
    response->type = CONNECT;

    // checking if client to connect exists and is a different client
    if(unconnectedClients[msg->toConnectID] == 1 && msg->clientID != msg->toConnectID) {
        // sending queue key and PID to the client which sent CONNECT commission
        response->queueKey = clientsQueueKeys[msg->toConnectID];
        response->clientPID = clientsPIDs[msg->toConnectID];
        response->clientID = msg->toConnectID;

        if(msgsnd(clientsQueues[msg->clientID], response, MESSAGE_SIZE, 0) < 0)
            perror("Cannot send CONNECT to first client");

        // sending queue key and PID to the client with whom previous client want to connect
        response->queueKey = clientsQueueKeys[msg->clientID];
        response->clientPID = clientsPIDs[msg->clientID];
        response->clientID = msg->clientID;

        if(msgsnd(clientsQueues[msg->toConnectID], response, MESSAGE_SIZE, 0) < 0)
            perror("Cannot send CONNECT to second client");

        // sending signal to second client to make it read message
        kill(clientsPIDs[msg->toConnectID], SIGUSR1);

        // marking clients as unreachable
        unconnectedClients[msg->clientID] = 0;
        unconnectedClients[msg->toConnectID] = 0;

        printf("%d and %d connected\n", msg->clientID, msg->toConnectID);
    }
    else {
        // sending PID = -1 as a flag to the requesting client because it has sent invalid client ID
        response->clientPID = -1;

        if(msgsnd(clientsQueues[msg->clientID], response, MESSAGE_SIZE, 0) < 0)
            perror("Cannot send CONNECT to client when there is no client with passed ID");
    }
}

void handleINIT(message *msg) {
    // looking for place for the new client in array with all clients and if it is, adding client to unconnected clients
    int newClientID = -1;
    for(int i=0; i<MAX_CLIENTS && newClientID == -1; i++) {
        if(clientsQueues[i] == -1) {
            newClientID = i;
            clientsQueueKeys[i] = msg->queueKey;
            clientsQueues[i] = msgget(msg->queueKey, 0666);
            unconnectedClients[i] = 1;
            clientsPIDs[i] = msg->clientPID;
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
    else if(msg->type == DISCONNECT)
        handleDISCONNECT(msg);
    else if(msg->type == LIST)
        handleLIST(msg);
    else if(msg->type == CONNECT)
        handleCONNECT(msg);
    else if(msg->type == INIT)
        handleINIT(msg);
}

void receiveMessage(int mtype) {
    // creating message buffer
    message *msgBuffer = malloc(sizeof(message));

    // receiving message from client
    if(msgrcv(queueID, msgBuffer, MESSAGE_SIZE, mtype, 0) < 0)
        perror("Cannot receive any message");

    // handling message
    handleMessage(msgBuffer);
}

void termination() {
    // terminating all clients
    for(int i=0; i<MAX_CLIENTS; i++) {
        if(clientsQueues[i] != - 1)
            kill(clientsPIDs[i], SIGINT);
    }
    // getting stop messages of all clients to ensure their terminations
    for(int i=0; i<MAX_CLIENTS; i++) {
        if(clientsQueues[i] != - 1)
            receiveMessage(STOP);
    }

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

int main() {
    if (setvbuf(stdout, NULL, _IONBF, 0) != 0) {
        printf("Error: buffering mode could not be changed!\n");
        exit(1);
    }

    // deleting message queue before process termination
    if(atexit(termination) != 0) perror("atexit function error");

    // deleting message queue in case of SIGINT signal
    if (signal(SIGINT, handleINTSignal) == SIG_ERR) perror("signal function error");

    // initialing array with clients queues IDs, array of unconnected clients and array with client's PIDs
    for(int i=0; i<MAX_CLIENTS; i++) {
        clientsQueues[i] = -1;
        unconnectedClients[i] = 0;
        clientsPIDs[i] = -1;
    }

    // getting key for message queue
    key_t queueKey = ftok(getenv("HOME"), PROJECT_ID);
    if(queueKey == -1) perror("Cannot get key by ftok function");

    // getting ID of message queue
    queueID = msgget(queueKey, 0666 | IPC_CREAT | IPC_EXCL);
    if(queueID == -1) perror("Cannot get queue ID by msgget function");

    // handling messages from clients
    while(1) {
        receiveMessage(RECEIVE_MTYPE);
    }

    return 0;
}