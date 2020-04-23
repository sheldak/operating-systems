#ifndef utilities_h
#define utilities_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define SIZE 128
#define MAX_CLIENTS  15
#define PROJECT_ID 'A'

#define RECEIVE_MTYPE -7
#define MESSAGE_SIZE (sizeof(message) - sizeof(long))


typedef enum mtype {
    STOP = 1,
    DISCONNECT = 2,
    LIST = 3,
    CONNECT = 4,
    INIT = 5,
    MSG = 6
} mtype;


struct message {
    long type;
    key_t queueKey;
    pid_t clientPID;
    int clientID;
    int toConnectID;
    int allClients[MAX_CLIENTS];
    int unconnectedClients[MAX_CLIENTS];
    char message[SIZE];
} typedef message;



#endif