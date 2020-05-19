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

void receive() {
    // getting number to write to the shared array
    int randomNumber = rand()%MAX_NUMBER + 1;

    // decrementing semaphore
    decrementSemaphore(semaphoresID);

    // checking if there is room for new order in the array
    if(shared->ordersToPrepare + shared->ordersToSend < ARRAY_SIZE) {
        int emptyIndex = 0;
        while(shared->array[emptyIndex].number != 0)
            emptyIndex++;

        // writing number (order) to the array
        shared->array[emptyIndex].number = randomNumber;
        shared->ordersToPrepare++;

        // printing current status of the orders
        printf("%d %s\n", getpid(), getCurrentTime());
        printf("Added number: %d. Orders to prepare: %d. Orders to send: %d.\n\n",
               randomNumber, shared->ordersToPrepare, shared->ordersToSend);
    }

    // incrementing semaphore
    incrementSemaphore(semaphoresID);
}

int main() {
    if (setvbuf(stdout, NULL, _IONBF, 0) != 0) {
        printf("Error: buffering mode could not be changed!\n");
        exit(1);
    }

    // to get different numbers in different receivers
    srand(time(NULL) + getpid());

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

    // receiving orders
    while(1) {
        receive();
    }

    return 0;
}