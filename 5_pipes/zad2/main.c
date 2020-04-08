#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE 128

int main(int argc, char **argv) {
    // we need a path to file to sort it
    if(argc < 2)
        perror("too few arguments");

    // sorting command
    char *command = calloc(SIZE, sizeof(char));
    strcpy(command, "sort ");
    strcat(command, argv[1]);

    // making new file and new pipe which will run the command
    FILE *sortInput = popen(command, "w");

    // waiting for process termination
    if(pclose(sortInput) == -1)
        perror("error when waiting for process termination");

    return 0;
}
