#ifndef utilities_h
#define utilities_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>

#define SIZE 128
#define MAX_CLIENTS  15
#define PROJECT_ID 'A'

#define RECEIVE_MTYPE -7
#define MESSAGE_SIZE 128

#define SERVER_QUEUE_NAME "/queue_server"
#define CLIENT_NAME_SIZE 16


typedef enum mtype {
    STOP = 6,
    DISCONNECT = 5,
    LIST = 4,
    CONNECT = 3,
    INIT = 2,
    MSG = 1
} mtype;


struct message {
    long type;
    key_t queueKey;
    pid_t clientPID;
    int clientID;
    int toConnectID;
    int allClients[MAX_CLIENTS];
    int unconnectedClients[MAX_CLIENTS];
    char message[MESSAGE_SIZE];
} typedef message;



#endif