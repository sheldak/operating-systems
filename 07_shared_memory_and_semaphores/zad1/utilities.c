#include "utilities.h"

char *getCurrentTime() {
    // getting time in seconds and microseconds
    struct timeval currTime;
    gettimeofday(&currTime, NULL);

    // changing microseconds to milliseconds
    int milliseconds = (int) currTime.tv_usec / 1000;

    // saving day and time (just hours, minutes and seconds) to char array
    char timeWithoutMilliseconds[100];
    strftime(timeWithoutMilliseconds, 100, "%Y-%m-%d %H:%M:%S", localtime(&currTime.tv_sec));

    // making one char array for time containing milliseconds
    char *time = calloc(100, sizeof(char));
    sprintf(time, "%s:%03d", timeWithoutMilliseconds, milliseconds);

    return time;
}

int openSemaphore() {
    // getting key for semaphore
    key_t semaphoreKey = ftok( getenv("HOME"), SEMAPHORE_ID);
    if(semaphoreKey == -1) perror("Cannot get key for semaphore for worker by ftok function");

    // getting semaphore ID
    int semaphoreID = semget(semaphoreKey, 0, 0);

    return semaphoreID;
}

void *openSharedMemory() {
    // getting key for shared memory
    key_t memoryKey = ftok( getenv("HOME"), MEMORY_ID);
    if(memoryKey == -1) perror("Cannot get key for shared memory by ftok function");

    // getting shared memory ID
    int memoryID = shmget(memoryKey, 0, 0);

    // getting address of shared memory
    void *memoryAddress = shmat(memoryID, NULL, 0);
    if(memoryAddress == (sharedVariables *) (-1)) perror("Cannot get address");

    return memoryAddress;
}

void decrementSemaphore(int semaphoreID) {
    struct sembuf operations[1];
    operations[0].sem_num = 0;
    operations[0].sem_op = -1;
    operations[0].sem_flg = SEM_UNDO;

    if(semop(semaphoreID, operations, 1) < 0) perror("Cannot decrement semaphore");
}

void incrementSemaphore(int semaphoreID) {
    struct sembuf operations[1];
    operations[0].sem_num = 0;
    operations[0].sem_op = 1;
    operations[0].sem_flg = SEM_UNDO;

    if(semop(semaphoreID, operations, 1) < 0) perror("Cannot increment semaphore");
}
