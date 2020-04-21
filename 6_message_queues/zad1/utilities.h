#ifndef utilities_h
#define utilities_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define SIZE 128
#define MAX_CLIENTS  15
#define PROJECT_ID 'A'

#define MAX_MESSAGE_SIZE 4096
#define RECEIVE_MTYPE -6


typedef enum mtype {
    INIT = 1,
    LIST = 2,
    CONNECT = 3,
    DISCONNECT = 4,
    STOP = 5
} mtype;


struct message {
    long type;
    key_t queueKey;
} typedef message;



#endif