#include "utilities.c"

int semaphoresID = -1;
int memoryID = -1;

void *memoryAddress;
sharedVariables *shared;


void terminate() {
    if(shmdt(memoryAddress) < 0) perror("Cannot detach memory");

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

void pack() {
    // decrementing semaphore
    decrementSemaphore(semaphoresID);

    // checking if there is any order to pack
    if(shared->ordersToPrepare > 0) {
        // getting order to pack
        int number = shared->array[shared->firstToPrepare];

        // removing order from array
        shared->array[shared->firstToPrepare] = 0;
        shared->firstToPrepare = (shared->firstToPrepare + 1) % ARRAY_SIZE;
        shared->ordersToPrepare--;

        // packing order
        number *= 2;

        // adding order to array to be sent
        shared->array[(shared->firstToSend + shared->ordersToSend) % ARRAY_SIZE] = number;
        shared->ordersToSend++;

        // printing current status of the orders
        printf("%d %s\n", getpid(), getCurrentTime());
        printf("Prepared order: %d. Orders to prepare: %d. Orders to send: %d.\n\n",
               number, shared->ordersToPrepare, shared->ordersToSend);
    }

    // incrementing semaphore
    incrementSemaphore(semaphoresID);
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

    // getting semaphore ID
    semaphoresID = openSemaphore();

    // getting shared memory address
    memoryAddress = openSharedMemory();

    // converting address to shared structure
    shared = (sharedVariables*) memoryAddress;

    // packing orders
    while(1) {
        pack();
    }

    return 0;
}