#include "utilities.c"

int semaphoreID = -1;
int memoryID = -1;

sharedVariables *memoryAddress;

int receivers = -1;
int packers = -1;
int senders = -1;

pid_t *receiversPIDs;
pid_t *packersPIDs;
pid_t *sendersPIDs;


sharedVariables getInitialSharedVariables() {
    sharedVariables shared;

    for(int i=0; i<ARRAY_SIZE; i++)
        shared.array[i] = 0;

    shared.firstToPrepare = 0;
    shared.ordersToPrepare = 0;
    shared.firstToSend = ARRAY_SIZE/2;
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
}

void terminate() {
    stopWorkers();

    if(semctl(semaphoreID, 0, IPC_RMID) < 0) perror("Cannot delete semaphore");
    else printf("Semaphores deleted successfully\n");

    memoryAddress = shmat(memoryID, NULL, 0);

    if(shmdt(memoryAddress) < 0) perror("Cannot detach memory");
    else printf("Shared memory detached successfully from main process\n");

    if(shmctl(memoryID, IPC_RMID, NULL) < 0) perror("Cannot delete shared memory segment");
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

    // getting key for semaphore
    key_t semaphoreKey = ftok( getenv("HOME"), SEMAPHORE_ID);
    if(semaphoreKey == -1) perror("Cannot get key for semaphore for main process by ftok function");

    // getting semaphore ID
    semaphoreID = semget(semaphoreKey, 1, 0666 | IPC_CREAT |  IPC_EXCL);

    // setting semaphore value
    if(semctl(semaphoreID, 0, SETVAL, 1) < 0) perror("Cannot set semaphore");

    // getting key for shared memory
    key_t memoryKey = ftok( getenv("HOME"), MEMORY_ID);
    if(memoryKey == -1) perror("Cannot get key for shared memory by ftok function");

    // creating shared structure
    sharedVariables initialShared = getInitialSharedVariables();

    // getting shared memory ID
    memoryID = shmget(memoryKey, sizeof(sharedVariables), 0666 | IPC_CREAT |  IPC_EXCL);

    // getting address of shared memory
    memoryAddress = shmat(memoryID, NULL, 0);
    if(memoryAddress == (sharedVariables *) (-1)) perror("Cannot get address");

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

    sleep(1);

//    int val = semctl(semaphoresID, 0, GETVAL);
//
//    if(val < 0) perror("Cannot read semaphore");

//    printf("%d\n", val);



//    // getting key for shared memory
//    memoryKey = ftok( getenv("HOME"), MEMORY_ID);
//    if(memoryKey == -1) perror("Cannot get key by ftok function");
//
//    int memoryID = shmget(memoryKey, 10 * sizeof(int), 0666 | IPC_CREAT |  IPC_EXCL);
////    printf("%d\n", semaphoresID);
//
//    int *memoryAddress = shmat(memoryID, NULL, 0);
//    if(memoryAddress == (int *) (-1)) perror("Cannot get address");

//    sharedVariables *sharedVars;
//    sharedVars = memoryAddress;

//    *shared = 3;
//    *shared++;
//    *shared = 5;

//    printf("%d", *memoryAddress++);
//    printf("%d", *memoryAddress--);

//    if(semctl(semaphoresID, 11, IPC_RMID) < 0) perror("Cannot delete semaphore");
//
//    if(shmdt(memoryAddress) < 0) perror("Cannot detach memory");
//
//    if(shmctl(memoryID, IPC_RMID, NULL) < 0) perror("Cannot delete shared memory segment");

    return 0;
}
