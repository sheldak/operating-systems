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

int connected = 0;
int interlocutorID = -1;
int interlocutorQueueID = -1;
int interlocutorPID = -1;

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
    if(msgsnd(serverQueueID, msg, MESSAGE_SIZE, 0) < 0) perror("Cannot send STOP");

    // freeing memory
    free(msg);

    printf("\nClient terminated!\n");

    // terminating process (it will call termination function because of atexit)
    exit(0);
}

void handleDISCONNECT() {
    printf("Disconnected with %d\n", interlocutorID);

    connected = 0;
    interlocutorID = -1;
    interlocutorQueueID = -1;
    interlocutorPID = -1;
}

void handleLIST(message *msg) {
    printf("You can connect with: ");
    for(int i=0; i<MAX_CLIENTS; i++) {
        if(msg->unconnectedClients[i] == 1)
            printf("%d ", i);
    }
    printf("\n\n");
}

void handleCONNECT(message *msg) {
    // making connection
    if(msg->clientPID != -1) {
        printf("Connected with %d. Type \"disconnect\" to end the conversation\n\n", msg->clientID);
        connected = 1;
        interlocutorID = msg->clientID;

        // getting interlocutor's message queue ID
        interlocutorQueueID = msgget(msg->queueKey, 0666);
        if(interlocutorQueueID == -1) perror("Cannot get interlocutor queue ID by msgget function");

        // getting interlocutor's PID
        interlocutorPID = msg->clientPID;
    }
    // cannot make connection
    else
        printf("There is no available client with that ID\n");
}

void handleINIT(message *msg) {
    // setting client's ID
    ID = msg->clientID;

    printf("Client registered!\n");
}

void handleMSG(message *msg) {
    // printing message
    printf("THEM: %s\n", msg->message);
}

int receiveMessage(int useNOWAIT) {
    // returns 1 if any message was handled
    int toReturn = 1;

    // creating message buffer
    message *msgBuffer = malloc(sizeof(message));

    // receiving message from server
    if(useNOWAIT == 1) {
        if(msgrcv(queueID, msgBuffer, MESSAGE_SIZE, RECEIVE_MTYPE, IPC_NOWAIT) < 0)
            toReturn = 0;
    }
    else {
        if(msgrcv(queueID, msgBuffer, MESSAGE_SIZE, RECEIVE_MTYPE, 0) < 0)
            perror("Cannot receive any message");
    }

    // handling message
    if(msgBuffer->type == STOP)
        sendSTOP();
    if(msgBuffer->type == DISCONNECT)
        handleDISCONNECT();
    else if(msgBuffer->type == LIST)
        handleLIST(msgBuffer);
    else if(msgBuffer->type == CONNECT)
        handleCONNECT(msgBuffer);
    else if(msgBuffer->type == INIT)
        handleINIT(msgBuffer);
    else if(msgBuffer->type == MSG)
        handleMSG(msgBuffer);

    return toReturn;
}

void sendDISCONNECT() {
    // making DISCONNECT message for interlocutor and itself
    message *msg = malloc(sizeof(message));
    msg->type = DISCONNECT;
    msg->clientID = ID;
    msg->toConnectID = interlocutorID;

    // sending DISCONNECT message to the interlocutor
    if(msgsnd(interlocutorQueueID, msg, MESSAGE_SIZE, 0) < 0) perror("Cannot send DISCONNECT");

    // sending signal to interlocutor to make it read the message in message queue
    kill(interlocutorPID, SIGUSR1);

    // handling DISCONNECT message
    handleDISCONNECT();

    // sending DISCONNECT message to the server to make future connections possible
    if(msgsnd(serverQueueID, msg, MESSAGE_SIZE, 0) < 0) perror("Cannot send DISCONNECT");
}

void sendLIST() {
    // making LIST message for the server
    message *msg = malloc(sizeof(message));
    msg->type = LIST;
    msg->clientID = ID;

    // sending LIST message to the server
    if(msgsnd(serverQueueID, msg, MESSAGE_SIZE, 0) < 0) perror("Cannot send LIST");

    // receiving array with unconnected clients
    receiveMessage(0);

    // freeing memory
    free(msg);
}

void sendCONNECT(int toConnectID) {
    // making CONNECT message for the server
    message *msg = malloc(sizeof(message));
    msg->type = CONNECT;
    msg->clientID = ID;
    msg->toConnectID = toConnectID;

    // sending CONNECT message to the server
    if(msgsnd(serverQueueID, msg, MESSAGE_SIZE, 0) < 0) perror("Cannot send CONNECT");

    // receiving message with the other client message queue ID and PID
    receiveMessage(0);

    // freeing memory
    free(msg);
}

void registerClient() {
    // creating INIT message
    message *msg = malloc(sizeof(message));
    msg->type = INIT;
    msg->queueKey = queueKey;
    msg->clientPID = getpid();

    // sending INIT message to the server
    if(msgsnd(serverQueueID, msg, MESSAGE_SIZE, 0) < 0) perror("Cannot send INIT");

    // handling message
    receiveMessage(0);

    // freeing memory
    free(msg);
}

void handleINTSignal(int signum) {
    // sending STOP message to the server and terminating process
    sendSTOP();
}

void handleUSR1Signal(int signum) {
    // receiving all waiting messages after getting that signal
    while(receiveMessage(1) == 1) {}
}

void handleCommission(char *commission) {
    char *token = strtok(commission, " \n");
    if(strcmp(token, "list") == 0)
        sendLIST();
    if(strcmp(token, "connect") == 0) {
        token = strtok(NULL, commission);

        int toConnectID = (int) strtol(token, NULL, 10);
        sendCONNECT(toConnectID);
    }
}

void textToInterlocutor(char *text) {
    if(strcmp(text, "disconnect\n") == 0)
        sendDISCONNECT();
    else {
        // printing sent message
        printf("ME: %s\n", text);

        // creating MSG message
        message *msg = malloc(sizeof(message));
        msg->type = MSG;
        strcpy(msg->message, text);

        // sending MSG message to interlocutor
        if(msgsnd(interlocutorQueueID, msg, MESSAGE_SIZE, 0) < 0) perror("Cannot send MSG");

        // sending signal to interlocutor to make it read message from message queue
        kill(interlocutorPID, SIGUSR1);
    }
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
    if (signal(SIGINT, handleINTSignal) == SIG_ERR) perror("signal function error when setting SIGINT handler");

    // to make possible chat between clients
    if (signal(SIGUSR1, handleUSR1Signal) == SIG_ERR) perror("signal function error when setting SIGUSR1 handler");

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

    while(1) {
        char *text = malloc(SIZE * sizeof(char));
        size_t length = 0;

        if(connected == 0) {
            printf("Type commission:\n");
            getline(&text, &length, stdin);
            handleCommission(text);
        }
        else {
            getline(&text, &length, stdin);
            textToInterlocutor(text);
        }

        free(text);
    }


    return 0;
}
