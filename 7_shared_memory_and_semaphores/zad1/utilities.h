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

#define SEMAPHORE_ID 'A'
#define MEMORY_ID 'A'

#define ARRAY_SIZE 20
#define MAX_NUMBER 100

struct sharedVariables {
    int array[ARRAY_SIZE];
    int firstToPrepare;
    int ordersToPrepare;
    int firstToSend;
    int ordersToSend;
} typedef sharedVariables;

char *getCurrentTime();
int openSemaphore();
void *openSharedMemory();
void decrementSemaphore(int semaphoreID);
void incrementSemaphore(int semaphoreID);

#endif