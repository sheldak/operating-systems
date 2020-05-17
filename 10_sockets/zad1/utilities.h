#ifndef utilities_h
#define utilities_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <mqueue.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/epoll.h>


#define MAX_CLIENTS 12
#define MAX_NAME_SIZE 20
#define TEXT_SIZE 128

#define PING 0
#define NAME 1
#define WAIT 2
#define INIT 3
#define MOVE 4
#define STOP 5

#define X 1
#define O 2

struct message {
    int type;
    int ID;
    char name[MAX_NAME_SIZE];
    int sign;
    int board[3][3];
} typedef message;
















#define SEMAPHORE_ID 'A'
#define MEMORY_ID 'A'

#define ARRAY_SIZE 20
#define MAX_NUMBER 100

struct order {
    int number;
    int packed;
} typedef order;

struct sharedVariables {
    order array[ARRAY_SIZE];
    int ordersToPrepare;
    int ordersToSend;
} typedef sharedVariables;

char *getCurrentTime();
int openSemaphore();
void *openSharedMemory();
void decrementSemaphore(int semaphoreID);
void incrementSemaphore(int semaphoreID);

#endif