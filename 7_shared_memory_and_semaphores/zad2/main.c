#include "utilities.c"

sem_t *semaphoreAddress;
sharedVariables *memoryAddress;

int receivers = -1;
int packers = -1;
int senders = -1;

pid_t *receiversPIDs;
pid_t *packersPIDs;
pid_t *sendersPIDs;


sharedVariables getInitialSharedVariables() {
    sharedVariables shared;

    for(int i=0; i<ARRAY_SIZE; i++) {
        shared.array[i].number = 0;
        shared.array[i].packed = 0;
    }

    shared.ordersToPrepare = 0;
    shared.ordersToSend = 0;

    return shared;
}

void stopWorkers() {
    for(int i=0; i<receivers; i++)
        kill(receiversPIDs[i], SIGINT);

    for(int i=0; i<packers; i++)
        kill(packersPIDs[i], SIGINT);

    for(int i=0; i<senders; i++)
        kill(sendersPIDs[i], SIGINT);

    for(int i=0; i<receivers; i++)
        waitpid(receiversPIDs[i], NULL, 0);

    for(int i=0; i<packers; i++)
        waitpid(packersPIDs[i], NULL, 0);

    for(int i=0; i<senders; i++)
        waitpid(sendersPIDs[i], NULL, 0);
}

void terminate() {
    stopWorkers();

    // closing semaphore
    if(sem_close(semaphoreAddress) < 0) perror("Cannot close semaphore in main process");

    // deleting semaphore
    if(sem_unlink(SEMAPHORE_NAME) < 0) perror("Cannot delete semaphore");
    else printf("\nSemaphore deleted successfully\n");

    // detaching shared memory
    if(munmap(memoryAddress, sizeof(sharedVariables)) < 0) perror("Cannot detach memory in main process");

    // deleting shared memory
    if(shm_unlink(MEMORY_NAME) < 0) perror("Cannot delete shared memory");
    else printf("Shared memory deleted successfully\n");

    exit(0);
}

void handleSEGVSignal(int signum) {
    printf("Segmentation fault\n");
    // terminating process after segmentation fault
    exit(0);
}

void handleINTSignal(int signum) {
    // terminating process and all workers
    exit(0);
}

int main(int argc, char **argv) {
    if (setvbuf(stdout, NULL, _IONBF, 0) != 0) {
        printf("Error: buffering mode could not be changed!\n");
        exit(1);
    }

    if(argc < 4) perror("Too few arguments");

    // deleting semaphores and shared memory before process termination
    if(atexit(terminate) != 0) perror("atexit function error");

    // to stop program properly in case of segmentation fault
    if (signal(SIGSEGV, handleSEGVSignal) == SIG_ERR) perror("Signal function error when setting SIGSEGV handler");

    // to stop program properly in case of SIGINT signal
    if (signal(SIGINT, handleINTSignal) == SIG_ERR) perror("signal function error when setting SIGINT handler");

    // first argument - number of receivers
    char *rest;
    receivers = (int) strtol(argv[1], &rest, 10);

    // second argument - number of packers
    packers = (int) strtol(argv[2], &rest, 10);

    // third argument - number of senders
    senders = (int) strtol(argv[3], &rest, 10);

    // creating semaphore
    semaphoreAddress = sem_open(SEMAPHORE_NAME, O_CREAT | O_EXCL, 0666, 1);
    if(semaphoreAddress == SEM_FAILED) perror("Cannot create semaphore");

    // creating segment of shared memory
    int memoryDescriptor = shm_open(MEMORY_NAME, O_RDWR | O_CREAT | O_EXCL,0666);
    if(memoryDescriptor < 0) perror("Cannot create shared memory");

    // specifying size of the shared memory
    if(ftruncate(memoryDescriptor, sizeof(sharedVariables)) < 0) perror("Cannot truncate memory");

    // attaching shared memory
    memoryAddress = mmap(NULL, sizeof(sharedVariables), PROT_WRITE, MAP_SHARED, memoryDescriptor, 0);
    if(memoryAddress == (sharedVariables *) (-1)) perror("Cannot get address of shared memory by the main process");

    // creating shared structure
    sharedVariables initialShared = getInitialSharedVariables();

    // make shared memory an initialized sharedVariables structure
    *memoryAddress = initialShared;

    // preparing arrays with worker's PIDs
    receiversPIDs = calloc(receivers, sizeof(pid_t));
    packersPIDs = calloc(packers, sizeof(pid_t));
    sendersPIDs = calloc(senders, sizeof(pid_t));

    // making receivers in other processes
    for(int i=0; i<receivers; i++) {
        pid_t pid = fork();
        if(pid == 0) {
            if(execlp("./receiver", "receiver", NULL) < 0) perror("Cannot create receiver");
        }

        receiversPIDs[i] = pid;
    }

    // making packers in other processes
    for(int i=0; i<packers; i++) {
        pid_t pid = fork();
        if(pid == 0) {
            if(execlp("./packer", "packer", NULL) < 0) perror("Cannot create packer");
        }

        packersPIDs[i] = pid;
    }

    // making senders in other processes
    for(int i=0; i<senders; i++) {
        pid_t pid = fork();
        if(pid == 0) {
            if(execlp("./sender", "sender", NULL) < 0) perror("Cannot create sender");
        }

        sendersPIDs[i] = pid;
    }

   while(1) {}

    return 0;
}
