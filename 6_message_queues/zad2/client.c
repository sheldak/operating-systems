#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

#include "utilities.h"

mqd_t queueDesc = -1;
char *queueName;

mqd_t serverQueueDesc = -1;

int ID = -1;

int connected = 0;
int interlocutorID = -1;
char *interlocutorQueueName;
int interlocutorQueueDesc = -1;

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

    // closing interlocutor's message queue
    if(mq_close(interlocutorQueueDesc) < 0) perror("Cannot close interlocutor's message queue");

    connected = 0;
    interlocutorID = -1;
    interlocutorQueueDesc = -1;
}

void handleLIST(char *message) {
    for(int i=2; i<strlen(message); i++)
        printf("%c", message[i]);

    printf("\n\n");
}

void handleCONNECT(char *message) {
    int type;
    sscanf(message, "%d %s %d", &type, interlocutorQueueName, &interlocutorID);

    // making connection
    if(strcmp(interlocutorQueueName, "error") != 0) {
        printf("Connected with %d. Type \"disconnect\" to end the conversation\n\n", interlocutorID);
        connected = 1;

        // getting interlocutor's message queue descriptor
        interlocutorQueueDesc = mq_open(interlocutorQueueName, O_WRONLY);
        if(interlocutorQueueDesc < 0) perror("Cannot open interlocutor's message queue");
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

void handleMSG(char *message) {
    // printing message
    printf("THEM: ");
    for(int i=2; i<strlen(message); i++)
        printf("%c", message[i]);

    printf("\n\n");
}

void receiveMessage() {
    // creating message buffers
    char *msgBuffer = malloc(MESSAGE_SIZE * sizeof(char));
    unsigned int type;

    ssize_t messageSize = mq_receive(queueDesc, msgBuffer, MESSAGE_SIZE, &type);
    if(messageSize < 0)
        perror("Cannot receive any message");

    // handling message
    if(type == STOP)
        sendSTOP();
    else if(type == DISCONNECT)
        handleDISCONNECT();
    else if(type == LIST)
        handleLIST(msgBuffer);
    else if(type == CONNECT)
        handleCONNECT(msgBuffer);
    else if(type == INIT)
        handleINIT(msgBuffer);
    else if(type == MSG)
        handleMSG(msgBuffer);

    // freeing memory
    free(msgBuffer);

    // structure for notifying
    struct sigevent notification;
    notification.sigev_notify = SIGEV_SIGNAL;
    notification.sigev_signo = SIGUSR1;

    mq_notify(queueDesc, &notification);
}

void sendDISCONNECT() {
    // making DISCONNECT message for interlocutor and server
    char message[MESSAGE_SIZE];
    sprintf(message, "%d %d %d", DISCONNECT, ID, interlocutorID);

    // sending DISCONNECT message to the interlocutor
    if(mq_send(interlocutorQueueDesc, message, MESSAGE_SIZE, DISCONNECT) < 0) perror("Cannot send DISCONNECT");

    // handling DISCONNECT message
    handleDISCONNECT();

    // sending DISCONNECT message to the server to make future connections possible
    if(mq_send(serverQueueDesc, message, MESSAGE_SIZE, DISCONNECT) < 0) perror("Cannot send DISCONNECT");
}

void sendLIST() {
    // creating LIST message
    char message[MESSAGE_SIZE];
    sprintf(message, "%d %d", LIST, ID);

    // sending LIST message to the server
    if(mq_send(serverQueueDesc, message, MESSAGE_SIZE, LIST) < 0) perror("Cannot send LIST");

    // receiving arrays
    receiveMessage();
}

void sendCONNECT(int toConnectID) {
    // creating CONNECT message
    char message[MESSAGE_SIZE];
    sprintf(message, "%d %d %d", CONNECT, ID, toConnectID);

    // sending CONNECT message to the server
    if(mq_send(serverQueueDesc, message, MESSAGE_SIZE, CONNECT) < 0) perror("Cannot send CONNECT");

    // receiving message with the other client message queue ID and PID
    receiveMessage();
}

void registerClient() {
    // creating INIT message
    char message[MESSAGE_SIZE];
    sprintf(message, "%d %s", INIT, queueName);

    // sending INIT message to the server
    if(mq_send(serverQueueDesc, message, MESSAGE_SIZE, INIT) < 0) perror("Cannot send INIT");

    // handling message
    receiveMessage();
}

void sendMSG(char *text) {
    // creating MSG message
    char message[MESSAGE_SIZE];
    sprintf(message, "%d %s", MSG, text);

    // sending MSG message to the server
    if(mq_send(interlocutorQueueDesc, message, MESSAGE_SIZE, MSG) < 0) perror("Cannot send MSG");
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
    receiveMessage();
}

void handleCommission(char *commission) {
    char *token = strtok(commission, " \n");
    if(strcmp(token, "stop") == 0)
        sendSTOP();
    else if(strcmp(token, "list") == 0)
        sendLIST();
    else if(strcmp(token, "connect") == 0) {
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
        sendMSG(text);
    }
}

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

    // initializing interlocutor queue
    interlocutorQueueName = malloc(MESSAGE_SIZE * sizeof(char));

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
        char *text = malloc(SIZE * sizeof(char));
        size_t length = 0;

        if(connected == 0) {
            printf("Type commission:\n");
            getline(&text, &length, stdin);
            if(connected == 1)
                textToInterlocutor(text);
            else
                handleCommission(text);
        }
        else {
            getline(&text, &length, stdin);
            if(connected == 1)
                textToInterlocutor(text);
            else
                handleCommission(text);
        }

        free(text);
    }


    return 0;
}
