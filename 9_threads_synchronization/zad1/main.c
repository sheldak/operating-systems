#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <pthread.h>

#define SIZE 128


int chairs;

struct threadSpecs {
    int threadsNum;
    int threadID;
} typedef threadSpecs;

void terminate() {
    exit(0);
}

void handleSEGVSignal() {
    printf("Segmentation fault\n");

    // terminating process after segmentation fault
    exit(0);
}

void handleINTSignal() {
    // terminating process and all workers
    exit(0);
}

void *barber() {
    printf("I am barber!\n");
    return (void*) 0;
}

void *client() {
    printf("I am client!\n");
    return (void*) 0;
}


//void *countShadesSign(void *arg) {
//    threadSpecs *specs = (threadSpecs*) arg;
//
//    // getting range of shades which this thread is counting
//    int minShade = (256 / specs->threadsNum) * specs->threadID;
//    int maxShade = (256 / specs->threadsNum) * (specs->threadID + 1) - 1;
//
//    // for time measurement
//    struct timespec startTime;
//    struct timespec endTime;
//    clock_gettime(CLOCK_REALTIME, &startTime);
//
//    // counting shades
//    for(int i=0; i<h; i++) {
//        for(int j=0; j<w; j++) {
//            int shade = matrix[i][j];
//            if(shade >= minShade && shade <= maxShade)
//                histogram[shade]++;
//        }
//    }
//
//    // getting time of counting
//    clock_gettime(CLOCK_REALTIME, &endTime);
//    long countingTime = getTimeDifference(startTime, endTime);
//
//    return (void*) countingTime;
//}



int main(int argc, char **argv) {
    if (setvbuf(stdout, NULL, _IONBF, 0) != 0) {
        printf("Error: buffering mode could not be changed!\n");
        exit(1);
    }

    if (argc < 3) perror("Too few arguments");

    if (atexit(terminate) != 0) perror("atexit function error");

    // to stop program properly in case of segmentation fault
    if (signal(SIGSEGV, handleSEGVSignal) == SIG_ERR) perror("Signal function error when setting SIGSEGV handler");

    // to stop program properly in case of SIGINT signal
    if (signal(SIGINT, handleINTSignal) == SIG_ERR) perror("signal function error when setting SIGINT handler");

    // first argument - number of chairs in waiting room
    char *rest;
    chairs = (int) strtol(argv[1], &rest, 10);

    // second argument - number of clients
    int clients = (int) strtol(argv[2], &rest, 10);

    // barber's thread ID
    pthread_t barberThreadID;

    // array with clients' threads' IDs
    pthread_t *clientsThreadsIDs = calloc(clients, sizeof(pthread_t));

    // making barber's thread
    if (pthread_create(&barberThreadID, NULL, barber, NULL) != 0) perror("Cannot create barber's thread");

    // making clients' threads
    for (int i = 0; i < clients; i++) {
//        threadSpecs *specs = malloc(sizeof(threadSpecs));
//        specs->threadsNum = threads;
//        specs->threadID = i;

        if (pthread_create(&clientsThreadsIDs[i], NULL, client, NULL) != 0)
            perror("Cannot create client's thread");
    }

    // waiting for threads termination
    for (int i = 0; i < clients; i++) {
        if (pthread_join(clientsThreadsIDs[i], NULL) != 0) perror("pthread_join error");
    }

    // freeing memory
    free(clientsThreadsIDs);

    return 0;
}
