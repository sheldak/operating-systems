#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
    if(argc < 2)
        perror("too few arguments");

    // first argument - characters to read by producer and to write by consumer
    char *bufferSize = argv[1];

    // creating named pipe
    mkfifo("pipe", 0666);

    // creating 5 producers
    char *producersFiles[5] = {"producer1.txt", "producer2.txt", "producer3.txt", "producer4.txt", "producer5.txt"};
    for(int i=0; i<5; i++) {
        pid_t currPid = fork();

        if (currPid == 0) {
            char *arguments[5] = {"producer", "pipe", producersFiles[i], bufferSize, NULL};
            execvp("./producer", arguments);
        }
    }

    // creating consumer
    pid_t currPid = fork();
    if(currPid == 0) {
        char *arguments[5] = {"consumer", "pipe", "consumer.txt", bufferSize, NULL};
        execvp("./consumer", arguments);
    }

    // waiting for all processes to terminate
    for(int i=0; i<6; i++)
        wait(NULL);

    return 0;
}
