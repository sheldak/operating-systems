#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <pthread.h>


int chairs;
int *waitingRoom;
int firstClient;
int barberIsSleeping = 0;

pthread_mutex_t waitingRoomMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t waitingRoomEmptiness = PTHREAD_COND_INITIALIZER;

pthread_t *clientsThreadsIDs;

struct clientID {
    int ID;
} typedef clientID;

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

void waitRandomTime(int shaving) {
    int time;
    if(shaving == 1)
        time = rand()%4 + 1;
    else
        time = rand()%2 + 1;

    sleep(time);
}

void *barber() {
    // setting cancel type to make it possible for main thread to cancel barber
    if (pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL) != 0)
        perror("Barber cannot set cancel type");

    while(1) {
        // ID of client to shave
        int toShave;

        // locking mutex
        pthread_mutex_lock(&waitingRoomMutex);

        // checking if there is any client to shave; in case there is not, barber is sleeping
        while (waitingRoom[firstClient] == -1) {
            printf("Barber: I go to sleep\n\n");
            barberIsSleeping = 1;
            pthread_cond_wait(&waitingRoomEmptiness, &waitingRoomMutex);
        }

        // setting the information that first client will be shaved
        toShave = waitingRoom[firstClient];
        waitingRoom[firstClient] = -1;
        firstClient = (firstClient + 1) % chairs;

        if(barberIsSleeping == 1)
            barberIsSleeping = 0;
        else {
            // checking number of clients which are waiting
            int waitingClients = 0;
            while(waitingRoom[(firstClient + waitingClients) % chairs] != -1)
                waitingClients++;

            printf("Barber: %d clients is waiting; I am shaving client with ID: %d\n\n", waitingClients, toShave);
        }

        // unlocking mutex
        pthread_mutex_unlock(&waitingRoomMutex);

        // shaving
        waitRandomTime(1);

        // cancelling client thread
        pthread_cancel(clientsThreadsIDs[toShave]);
    }
}

void *client(void *arg) {
    // getting client's ID
    int ID = ((clientID*) arg)->ID;

    // flag to know if the client has managed to be in waiting room
    int inWaitingRoom = 0;

    // attempt to go to the waiting room
    while(inWaitingRoom == 0) {
        // locking mutex
        pthread_mutex_lock(&waitingRoomMutex);

        // checking if there is any empty seat
        for(int i=0; i<chairs && inWaitingRoom == 0; i++) {
            if(waitingRoom[(firstClient + i) % chairs] == -1) {
                // writing client's ID to the seat
                waitingRoom[(firstClient + i) % chairs] = ID;
                inWaitingRoom = 1;

                // waking up barber if he is sleeping
                if(barberIsSleeping == 1) {
                    pthread_cond_broadcast(&waitingRoomEmptiness);
                    printf("I am waking up barber; ID: %d\n\n", ID);
                }
                // waiting if someone is being shaved
                else {
                    int freeSeats = chairs - i - 1;
                    printf("Waiting room; free seats: %d; ID: %d\n\n", freeSeats, ID);
                }
            }
        }

        // unlocking mutex
        pthread_mutex_unlock(&waitingRoomMutex);

        // waiting for next attempt if there is no place to sit
        if(inWaitingRoom == 0) {
            printf("Occupied; ID: %d\n\n", ID);
            waitRandomTime(0);
        }
    }

    // waiting for being shaved (barber will cancel client's thread)
    pause();

    return (void*) 0;
}

int main(int argc, char **argv) {
    srand(time(NULL));

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

    // initialing waiting room array
    waitingRoom = calloc(chairs, sizeof(int));
    for(int i=0; i<chairs; i++)
        waitingRoom[i] = -1;
    firstClient = 0;

    // barber thread ID
    pthread_t barberThreadID;

    // array with clients threads' IDs
    clientsThreadsIDs = calloc(clients, sizeof(pthread_t));

    // making barber thread
    if (pthread_create(&barberThreadID, NULL, barber, NULL) != 0) perror("Cannot create barber's thread");

    // making clients threads
    for (int i = 0; i < clients; i++) {
        waitRandomTime(0);

        clientID *ID = malloc(sizeof(clientID));
        ID->ID = i;

        if (pthread_create(&clientsThreadsIDs[i], NULL, client, ID) != 0)
            perror("Cannot create client's thread");
    }

    // waiting for threads termination
    for (int i = 0; i < clients; i++) {
        if (pthread_join(clientsThreadsIDs[i], NULL) != 0) perror("pthread_join error");
    }

    // canceling barber thread
    pthread_cancel(barberThreadID);

    // destroying mutex and condition variable
    pthread_mutex_destroy(&waitingRoomMutex);
    pthread_cond_destroy(&waitingRoomEmptiness);

    // freeing memory
    free(waitingRoom);
    free(clientsThreadsIDs);

    return 0;
}
