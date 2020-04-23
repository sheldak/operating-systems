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

#endif