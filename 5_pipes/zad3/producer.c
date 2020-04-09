#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

#define SIZE 128


int main(int argc, char **argv) {
    if(argc < 4)
        perror("too few arguments");
    srand(time(0));

    // to check the number of PID digits
    pid_t pid = getpid();
    size_t bufferEnlargement = 2;
    while(pid > 0) {
        bufferEnlargement++;
        pid /= 10;
    }

    // prefix of messages - PID of the producer
    char *prefix = calloc(bufferEnlargement, sizeof(char));
    sprintf(prefix, "#%d#", getpid());

    // first argument - path to the named pipe
    char *pipePath = argv[1];
    int pipeDesc = open(pipePath, O_WRONLY);

    // second argument - path to products to send by pipe
    char *productsPath = argv[2];
    FILE *file = fopen(productsPath, "r");

    // third argument - characters to read by producer
    char *rest = calloc(SIZE, sizeof(char));
    size_t bufferSize = (size_t) strtol(argv[3], &rest, 10);

    // writing to the pipe
    for(int i=0; i<5; i++) {
        // waiting 1-2 seconds
        struct timespec *timer = malloc(sizeof(struct timespec*));
        timer->tv_sec = 1;
        timer->tv_nsec = rand()%1000000000;
        nanosleep(timer, NULL);

        // getting line from producer's file
        char *line = calloc(bufferSize, sizeof(char));
        size_t s;
        getline(&line, &s, file);

        // creating and sending message: #PID#(bufferSize characters)Z
        char *message = calloc(bufferSize+bufferEnlargement, sizeof(char));
        strcpy(message, prefix);
        strcat(message, line);
        write(pipeDesc, message, bufferSize+bufferEnlargement);

        free(line);
        free(message);
        free(timer);
    }

    fclose(file);
    close(pipeDesc);

    return 0;
}
