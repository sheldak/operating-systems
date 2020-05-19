#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define SIZE 128


int main(int argc, char **argv) {
    if(argc < 4)
        perror("too few arguments");

    // to check the number of PID digits
    pid_t pid = getpid();
    size_t bufferEnlargement = 2;
    while(pid > 0) {
        bufferEnlargement++;
        pid /= 10;
    }

    // first argument - path to the named pipe
    char *pipePath = argv[1];
    int pipeDesc = open(pipePath, O_RDONLY);

    // second argument - path to products to send by pipe
    char *productsPath = argv[2];
    FILE *file = fopen(productsPath, "w");

    // third argument - number of characters to read by consumer from pipe
    char *rest = calloc(SIZE, sizeof(char));
    size_t bufferSize = (size_t) strtol(argv[3], &rest, 10);
    bufferSize += bufferEnlargement;

    if(strcmp(rest, "") != 0)
        perror("invalid third argument in consumer");

    // reading from the pipe
    while(1) {
        char *line = calloc(bufferSize, sizeof(char));

        if(read(pipeDesc, line, bufferSize) == 0)
            break;

        fwrite(line, bufferSize, sizeof(char), file);
        free(line);
    }

    fclose(file);
    close(pipeDesc);

    return 0;
}

