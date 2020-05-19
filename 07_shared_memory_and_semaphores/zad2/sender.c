#include "utilities.c"

sem_t *semaphoreAddress;
void *memoryAddress;

sharedVariables *shared;


void terminate() {
    // closing semaphore
    if(sem_close(semaphoreAddress) < 0) perror("Cannot close semaphore in sender process");

    // detaching shared memory
    if(munmap(memoryAddress, sizeof(sharedVariables)) < 0) perror("Cannot detach memory in sender process");

    exit(0);
}

void handleSEGVSignal(int signum) {
    printf("Segmentation fault\n");

    // terminating process after segmentation fault
    exit(0);
}

void handleINTSignal(int signum) {
    // terminating process
    exit(0);
}

void send() {
    // decrementing semaphore
    decrementSemaphore(semaphoreAddress);

    // checking if there is any order to send
    if(shared->ordersToSend > 0) {
        // getting order to send
        int toSend = 0;
        while(shared->array[toSend].packed == 0)
            toSend++;

        // getting order
        int number = 3 * shared->array[toSend].number;

        // removing order from array
        shared->array[toSend].number = 0;
        shared->array[toSend].packed = 0;
        shared->ordersToSend--;

        // printing current status of the orders
        printf("%d %s\n", getpid(), getCurrentTime());
        printf("Sent order: %d. Orders to prepare: %d. Orders to send: %d.\n\n",
               number, shared->ordersToPrepare, shared->ordersToSend);
    }

    // incrementing semaphore
    incrementSemaphore(semaphoreAddress);
    usleep(1000);
}

int main() {
    if (setvbuf(stdout, NULL, _IONBF, 0) != 0) {
        printf("Error: buffering mode could not be changed!\n");
        exit(1);
    }

    // detaching memory before process termination
    if(atexit(terminate) != 0) perror("atexit function error");

    // to stop process properly in case of segmentation fault
    if (signal(SIGSEGV, handleSEGVSignal) == SIG_ERR) perror("Signal function error when setting SIGSEGV handler");

    // to stop process properly in case of SIGINT signal
    if (signal(SIGINT, handleINTSignal) == SIG_ERR) perror("signal function error when setting SIGINT handler");

    // getting semaphore address
    semaphoreAddress = openSemaphore();

    // getting shared memory address
    memoryAddress = openSharedMemory();

    // converting address to shared structure
    shared = (sharedVariables*) memoryAddress;

    // sending orders
    while(1) {
        send();
    }

    return 0;
}