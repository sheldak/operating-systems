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

sem_t *openSemaphore() {
    sem_t *semaphoreAddress = sem_open(SEMAPHORE_NAME, 0);
    if(semaphoreAddress == SEM_FAILED) perror("Cannot open semaphore");

    return semaphoreAddress;
}

void *openSharedMemory() {
    // opening segment of shared memory
    int memoryDescriptor = shm_open(MEMORY_NAME, O_RDWR, 0);
    if(memoryDescriptor < 0) perror("Cannot open shared memory");

    // attaching shared memory
    sharedVariables *memoryAddress = mmap(
            NULL, sizeof(sharedVariables), PROT_READ | PROT_WRITE, MAP_SHARED, memoryDescriptor, 0);
    if(memoryAddress == (sharedVariables *) (-1)) perror("Cannot get address of shared memory by the worker process");

    return memoryAddress;
}

void decrementSemaphore(sem_t *semaphoreAddress) {
    if(sem_wait(semaphoreAddress) < 0) perror("Cannot decrement semaphore");
}

void incrementSemaphore(sem_t *semaphoreAddress) {
    if(sem_post(semaphoreAddress) < 0) perror("Cannot increment semaphore");
}
