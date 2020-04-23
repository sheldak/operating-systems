#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <time.h>

#include "utilities.h"

mqd_t queueDesc = -1;
char *queueName;

mqd_t serverQueueDesc = -1;

int serverQueueID = -1;

int ID = -1;

int connected = 0;
int interlocutorID = -1;
int interlocutorQueueID = -1;
int interlocutorPID = -1;

char *getRandomName() {
    char *namePrefix = "/queue_";
    char *nameSuffix = calloc(CLIENT_NAME_SIZE+1, sizeof(char));

    for(int i=0; i<CLIENT_NAME_SIZE; i++) {
        char nextLetter = (char) ((int) 'a' + (rand()%26));
        nameSuffix[i] = nextLetter;
    }
    nameSuffix[CLIENT_NAME_SIZE] = '\0';

    char *name = (char*) malloc((strlen(namePrefix)+strlen(nameSuffix)+1));

    strcpy(name, namePrefix);
    strcat(name, nameSuffix);

    return name;
}

void termination() {
    // closing message queue
    if(mq_close(queueDesc) < 0) perror("Cannot close message queue");

    // closing server message queue
    if(mq_close(serverQueueDesc) < 0) perror("Cannot close server message queue");
    else printf("\nServer's message queue closed successfully\n");

    // removing message queue
    if(mq_unlink(queueName) < 0) perror("Cannot remove message queue");
    else printf("Message queue removed successfully\n");

    // terminating process
    exit(0);
}

void openServerQueue() {
    serverQueueDesc = mq_open(SERVER_QUEUE_NAME, O_WRONLY);
    if(serverQueueDesc < 0) perror("Cannot open server's message queue");
}

void sendSTOP() {
    // creating STOP message
    char message[MESSAGE_SIZE];
    sprintf(message, "%d %d", STOP, ID);

    // sending STOP message to the server
    if(mq_send(serverQueueDesc, message, MESSAGE_SIZE, STOP) < 0) perror("Cannot send STOP");

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
    printf("All clients: ");
    for(int i=0; i<MAX_CLIENTS; i++) {
        if(msg->allClients[i] == 1)
            printf("%d ", i);
    }
    printf("\n");

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

void handleINIT(char *message) {
    int type;

    sscanf(message, "%d %d", &type, &ID);

    printf("Client registered! ID: %d\n", ID);
}

void handleMSG(message *msg) {
    // printing message
    printf("THEM: %s\n", msg->message);
}

int receiveMessage(int useNOWAIT) {
    // returns 1 if any message was handled
    int toReturn = 1;

    // creating message buffers
    char *msgBuffer = malloc(sizeof(message));
    unsigned int type;

    // receiving message from server
//    if(useNOWAIT == 1) {
//        if(msgrcv(queueID, msgBuffer, MESSAGE_SIZE, RECEIVE_MTYPE, IPC_NOWAIT) < 0)
//            toReturn = 0;
//    }
//    else {
    ssize_t messageSize = mq_receive(queueDesc, msgBuffer, MESSAGE_SIZE, &type);
    if(messageSize < 0)
        perror("Cannot receive any message");
//    }

    // handling message
    if(type == STOP)
        sendSTOP();
//    else if(msgBuffer->type == DISCONNECT)
//        handleDISCONNECT();
//    else if(msgBuffer->type == LIST)
//        handleLIST(msgBuffer);
//    else if(msgBuffer->type == CONNECT)
//        handleCONNECT(msgBuffer);
    else if(type == INIT)
        handleINIT(msgBuffer);
//    else if(msgBuffer->type == MSG)
//        handleMSG(msgBuffer);

    // freeing memory
    free(msgBuffer);

    // structure for notifying
    struct sigevent notification;
    notification.sigev_notify = SIGEV_SIGNAL;
    notification.sigev_signo = SIGUSR1;

    mq_notify(queueDesc, &notification);

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

    // freeing memory
    free(msg);
}

//void sendLIST() {
//    // making LIST message for the server
//    message *msg = malloc(sizeof(message));
//    msg->type = LIST;
//    msg->clientID = ID;
//
//    // sending LIST message to the server
//    if(msgsnd(serverQueueID, msg, MESSAGE_SIZE, 0) < 0) perror("Cannot send LIST");
//
//    // receiving array with unconnected clients
//    receiveMessage(0);
//
//    // freeing memory
//    free(msg);
//}

//void sendCONNECT(int toConnectID) {
//    // making CONNECT message for the server
//    message *msg = malloc(sizeof(message));
//    msg->type = CONNECT;
//    msg->clientID = ID;
//    msg->toConnectID = toConnectID;
//
//    // sending CONNECT message to the server
//    if(msgsnd(serverQueueID, msg, MESSAGE_SIZE, 0) < 0) perror("Cannot send CONNECT");
//
//    // receiving message with the other client message queue ID and PID
//    receiveMessage(0);
//
//    // freeing memory
//    free(msg);
//}

void registerClient() {
    // creating INIT message
    char message[MESSAGE_SIZE];
    sprintf(message, "%d %s", INIT, queueName);

    // sending INIT message to the server
    if(mq_send(serverQueueDesc, message, MESSAGE_SIZE, INIT) < 0) perror("Cannot send INIT");

    // handling message
    receiveMessage(0);
}

void handleINTSignal(int signum) {
    // sending STOP message to the server and terminating process
    sendSTOP();
}

void handleSEGVSignal(int signum) {
    printf("Segmentation fault\n");
    // terminating process after segmentation fault
    sendSTOP();
}

void handleUSR1Signal(int signum) {
    receiveMessage(0);
}


//void handleCommission(char *commission) {
//    char *token = strtok(commission, " \n");
//    if(strcmp(token, "stop") == 0)
//        sendSTOP();
//    else if(strcmp(token, "list") == 0)
//        sendLIST();
//    else if(strcmp(token, "connect") == 0) {
//        token = strtok(NULL, commission);
//
//        int toConnectID = (int) strtol(token, NULL, 10);
//        sendCONNECT(toConnectID);
//    }
//}
//
//void textToInterlocutor(char *text) {
//    if(strcmp(text, "disconnect\n") == 0)
//        sendDISCONNECT();
//    else {
//        // printing sent message
//        printf("ME: %s\n", text);
//
//        // creating MSG message
//        message *msg = malloc(sizeof(message));
//        msg->type = MSG;
//        strcpy(msg->message, text);
//
//        // sending MSG message to interlocutor
//        if(msgsnd(interlocutorQueueID, msg, MESSAGE_SIZE, 0) < 0) perror("Cannot send MSG");
//
//        // sending signal to interlocutor to make it read message from message queue
//        kill(interlocutorPID, SIGUSR1);
//    }
//}

int main() {
    srand(time(NULL));

    if (setvbuf(stdout, NULL, _IONBF, 0) != 0) {
        printf("Error: buffering mode could not be changed!\n");
        exit(1);
    }

    // deleting message queue before process termination
    if(atexit(termination) != 0)
        perror("atexit function error");

    // deleting message queue in case of SIGINT signal
    if (signal(SIGINT, handleINTSignal) == SIG_ERR) perror("signal function error when setting SIGINT handler");

    // to stop program properly in case of segmentation fault
    if (signal(SIGSEGV, handleSEGVSignal) == SIG_ERR) perror("signal function error when setting SIGSEGV handler");

    // to receive message when got SIGUSR1 signal
    if (signal(SIGUSR1, handleUSR1Signal) == SIG_ERR) perror("signal function error when setting SIGUSR1 handler");

    // creating message queue
    struct mq_attr attributes;
    attributes.mq_flags = 0;
    attributes.mq_maxmsg = 5;
    attributes.mq_msgsize = MESSAGE_SIZE;
    attributes.mq_curmsgs = 0;


    queueName = getRandomName();
    queueDesc = mq_open(queueName, O_RDONLY | O_CREAT | O_EXCL, 0666, &attributes);
    if(queueDesc < 0) perror("Cannot create message queue");


    // opening server queue
    openServerQueue();

    // sending INIT message to the server
    registerClient();

    while(1) {
//        char *text = malloc(SIZE * sizeof(char));
//        size_t length = 0;
//
//        if(connected == 0) {
//            printf("Type commission:\n");
//            getline(&text, &length, stdin);
//            if(connected == 1)
//                textToInterlocutor(text);
//            else
//                handleCommission(text);
//        }
//        else {
//            getline(&text, &length, stdin);
//            if(connected == 1)
//                textToInterlocutor(text);
//            else
//                handleCommission(text);
//        }
//
//        free(text);
    }


    return 0;
}
