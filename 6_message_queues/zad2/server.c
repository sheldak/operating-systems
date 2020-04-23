#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "utilities.h"

// cd dev/c/operating_systems/6_message_queues/zad2

mqd_t queueDesc = -1;
char *clientsQueueNames[MAX_CLIENTS];
int clientsQueuesDesc[MAX_CLIENTS];
int unconnectedClients[MAX_CLIENTS];

void handleSTOP(char *message) {
    int type;
    int clientID;
    sscanf(message, "%d %d", &type, &clientID);

    if(clientID == -1)
        printf("Request for adding new client denied. List is full\n");
    else {
        if(mq_close(clientsQueuesDesc[clientID]) < 0) perror("Cannot close client's message queue");

        // removing client from array with clients
        clientsQueuesDesc[clientID] = -1;

        // setting client as not unconnected (it will not exist so should not be able to connect with other clients)
        unconnectedClients[clientID] = 0;

        printf("Client with ID %d removed\n", clientID);
    }
}

//void handleDISCONNECT(message *msg) {
//    unconnectedClients[msg->clientID] = 1;
//    unconnectedClients[msg->toConnectID] = 1;
//
//    printf("%d and %d disconnected\n", msg->clientID, msg->toConnectID);
//}
//
//void handleLIST(message *msg) {
//    // message which will be send to the new client
//    message *response = malloc(sizeof(message));
//    response->type = LIST;
//
//    // copying arrays
//    for(int i=0; i<MAX_CLIENTS; i++) {
//        if(clientsQueues[i] == -1)
//            response->allClients[i] = 0;
//        else
//            response->allClients[i] = 1;
//        response->unconnectedClients[i] = unconnectedClients[i];
//    }
//
//    // to not send requesting client as available to itself
//    response->unconnectedClients[msg->clientID] = 0;
//
//    if(msgsnd(clientsQueues[msg->clientID], response, MESSAGE_SIZE, 0) < 0) perror("Cannot send LIST");
//}
//
//void handleCONNECT(message *msg) {
//    // messages which will be send to clients
//    message *response = malloc(sizeof(message));
//    response->type = CONNECT;
//
//    // checking if client to connect exists and is a different client
//    if(unconnectedClients[msg->toConnectID] == 1 && msg->clientID != msg->toConnectID) {
//        // sending queue key and PID to the client which sent CONNECT commission
//        response->queueKey = clientsQueueKeys[msg->toConnectID];
//        response->clientPID = clientsPIDs[msg->toConnectID];
//        response->clientID = msg->toConnectID;
//
//        if(msgsnd(clientsQueues[msg->clientID], response, MESSAGE_SIZE, 0) < 0)
//            perror("Cannot send CONNECT to first client");
//
//        // sending queue key and PID to the client with whom previous client want to connect
//        response->queueKey = clientsQueueKeys[msg->clientID];
//        response->clientPID = clientsPIDs[msg->clientID];
//        response->clientID = msg->clientID;
//
//        if(msgsnd(clientsQueues[msg->toConnectID], response, MESSAGE_SIZE, 0) < 0)
//            perror("Cannot send CONNECT to second client");
//
//        // sending signal to second client to make it read message
//        kill(clientsPIDs[msg->toConnectID], SIGUSR1);
//
//        // marking clients as unreachable
//        unconnectedClients[msg->clientID] = 0;
//        unconnectedClients[msg->toConnectID] = 0;
//
//        printf("%d and %d connected\n", msg->clientID, msg->toConnectID);
//    }
//    else {
//        // sending PID = -1 as a flag to the requesting client because it has sent invalid client ID
//        response->clientPID = -1;
//
//        if(msgsnd(clientsQueues[msg->clientID], response, MESSAGE_SIZE, 0) < 0)
//            perror("Cannot send CONNECT to client when there is no client with passed ID");
//    }
//}

void handleINIT(char *message) {
    // getting message contents
    int type;
    char *clientQueueName = malloc(MESSAGE_SIZE * sizeof(char));
    sscanf(message, "%d %s", &type, clientQueueName);

    // looking for place for the new client in array with all clients and if it is, adding client to unconnected clients
    int newClientID = -1;
    for(int i=0; i<MAX_CLIENTS && newClientID == -1; i++) {
        if(clientsQueuesDesc[i] == -1) {
            newClientID = i;
            clientsQueueNames[i] = clientQueueName;
            clientsQueuesDesc[i] = mq_open(clientQueueName, O_WRONLY);
            if(clientsQueuesDesc[i] < 0) perror("Cannot open client's message queue");
            unconnectedClients[i] = 1;
        }
    }

    // response to client
    char response[MESSAGE_SIZE];

    // if there is no space for more clients, send STOP message to the new one to terminate it
    if(newClientID == -1) {
        sprintf(response, "%d", STOP);

        // opening client's queue just for sending STOP message
        mqd_t clientQueueDesc = mq_open(clientQueueName, O_WRONLY);
        if(clientQueueDesc < 0) perror("Cannot open client's message queue for STOP");

        // sending STOP message to the client
        if(mq_send(clientQueueDesc, response, MESSAGE_SIZE, STOP) < 0) perror("Cannot send STOP");

        // closing client's queue
        if(mq_close(clientQueueDesc) < 0) perror("Cannot close client's message queue after STOP");
    }
    // if there is enough space for the new client, send INIT message with its ID to it
    else {
        sprintf(response, "%d %d", INIT, newClientID);

        // sending INIT message to the client
        if(mq_send(clientsQueuesDesc[newClientID], response, MESSAGE_SIZE, INIT) < 0) perror("Cannot send INIT");

        printf("Added new client with ID %d\n", newClientID);
    }
}

void handleMessage(char *message, mtype type) {
    if(type == STOP)
        handleSTOP(message);
//    else if(msg->type == DISCONNECT)
//        handleDISCONNECT(msg);
//    else if(msg->type == LIST)
//        handleLIST(msg);
//    else if(msg->type == CONNECT)
//        handleCONNECT(msg);
    else if(type == INIT)
        handleINIT(message);
}

void receiveMessage() {
    // creating message buffer
    char *msgBuffer = malloc(MESSAGE_SIZE * sizeof(char));
    unsigned int type;

    // receiving message from client
    ssize_t messageSize = mq_receive(queueDesc, msgBuffer, MESSAGE_SIZE, &type);
    if(messageSize < 0)
        perror("Cannot receive any message");

    // handling message
    if(messageSize > 0)
        handleMessage(msgBuffer, type);

    free(msgBuffer);
}

void termination() {
    // terminating all clients
    for(int i=0; i<MAX_CLIENTS; i++) {
        if(clientsQueuesDesc[i] != - 1) {
            // sending STOP to client
            char message[MESSAGE_SIZE];
            sprintf(message, "%d", STOP);

            if(mq_send(clientsQueuesDesc[i], message, MESSAGE_SIZE, STOP) < 0) perror("Cannot send STOP");
        }
    }
    // getting stop messages of all clients to ensure their terminations
    for(int i=0; i<MAX_CLIENTS; i++) {
        if(clientsQueuesDesc[i] != - 1)
            receiveMessage();
    }

    // closing message queue
    if(mq_close(queueDesc) < 0) perror("Cannot close message queue");
    else printf("\nMessage queue removed successfully\n");

    // removing message queue
    if(mq_unlink(SERVER_QUEUE_NAME) < 0) perror("Cannot remove message queue");

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

    // initialing arrays with information about clients
    for(int i=0; i<MAX_CLIENTS; i++) {
        clientsQueueNames[i] = calloc(MESSAGE_SIZE, sizeof(char));
        clientsQueuesDesc[i] = -1;
        unconnectedClients[i] = 0;
    }

    // creating message queue
    struct mq_attr attributes;
    attributes.mq_flags = 0;
    attributes.mq_maxmsg = 5;
    attributes.mq_msgsize = MESSAGE_SIZE;
    attributes.mq_curmsgs = 0;

    queueDesc = mq_open(SERVER_QUEUE_NAME, O_RDONLY | O_CREAT | O_EXCL, 0666, &attributes);
    if(queueDesc < 0) perror("Cannot create message queue");

//
//    // getting ID of message queue
//    queueID = msgget(queueKey, 0666 | IPC_CREAT | IPC_EXCL);
//    if(queueID == -1) perror("Cannot get queue ID by msgget function");
//
//    // handling messages from clients
    while(1) {
        receiveMessage();
    }

    return 0;
}