#ifndef utilities_h
#define utilities_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/mman.h>

#define ARRAY_SIZE 20
#define MAX_NUMBER 100

#define SEMAPHORE_NAME "/semaphore"
#define MEMORY_NAME "/shared_memory"

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
sem_t *openSemaphore();
void *openSharedMemory();
void decrementSemaphore(sem_t *semaphoreAddress);
void incrementSemaphore(sem_t *semaphoreAddress);

#endif