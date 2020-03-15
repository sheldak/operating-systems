#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define SIZE 150

void generate(char *filename, char *recordsNum, char *recordSize) {
    char *command = calloc(SIZE, sizeof(char));

    strcpy(command, "head -c 10000000 /dev/urandom | tr -dc 'a-z'");
    strcat(command, " | fold -w ");
    strcat(command, recordSize);
    strcat(command, " | head -n ");
    strcat(command, recordsNum);
    strcat(command, " > ");
    strcat(command, filename);
    system(command);

    free(command);
}

int main(int argc, char **argv) {

    if(argc < 5)
        perror("too few arguments");

    char *commands[5] = {"generate", "sort", "copy", "sys", "lib"};


    for(int i=1; i<argc; i++) {
        if(strcmp(argv[i], commands[0]) == 0) {         // generate
            if(i+3 >= argc)
                perror("too few arguments after generate call");

            // extracting parameters
            char *filename = argv[i+1];
            char *recordsNum = argv[i+2];
            char *recordSize = argv[i+3];

            // generating random text to file
            generate(filename, recordsNum, recordSize);
        }
        else if(strcmp(argv[i], commands[1]) == 0) {    // sort
            if(i+4 >= argc)
                perror("too few arguments after sort call");


            // extracting parameters
            char *filename = argv[i+1];

            int recordsNum;
            char *endPtr;
            recordsNum = (int) strtol(argv[i+2], &endPtr, 10);
            if(recordsNum <= 0)
                perror("invalid number of records to generate");

            int recordSize;
            recordSize = (int) strtol(argv[i+3], &endPtr, 10);
            if(recordSize <= 0)
                perror("invalid size of records to generate");

        }
        else if(strcmp(argv[i], commands[2]) == 0) {    // copy
            if(i+5 >= argc)
                perror("too few arguments after copy call");
        }
    }
}

