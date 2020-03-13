#include <sys/times.h>
#include <unistd.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define SIZE 150


void error(char *message) {
    printf("Error: %s \n", message);
    exit(1);
}


int isNumber(char *s) {
    for (int i=0; i<strlen(s); i++) {
        if (isdigit(s[i]) == 0)
            return 0;
    }
    return 1;
}

double timeDiff(clock_t start, clock_t end) {
    return ((double) (end - start) / (double) sysconf(_SC_CLK_TCK));
}

void printResult(clock_t start, clock_t end, struct tms* tStart, struct tms* tEnd, FILE *reportFile) {
    printf("\tREAL TIME: %fl\n", timeDiff(start,end));
    printf("\tUSER TIME: %fl\n", timeDiff(tStart->tms_utime, tEnd->tms_utime));
    printf("\tSYSTEM TIME: %fl\n", timeDiff(tStart->tms_stime, tEnd->tms_stime));

    fprintf(reportFile, "\tREAL TIME: %fl\n", timeDiff(start, end));
    fprintf(reportFile, "\tUSER TIME: %fl\n", timeDiff(tStart->tms_utime, tEnd->tms_utime));
    fprintf(reportFile, "\tSYSTEM TIME: %fl\n", timeDiff(tStart->tms_stime, tEnd->tms_stime));
}

int main(int argc, char **argv) {
    if (argc <= 2)
        error("Too few arguments");

    FILE *reportFile = fopen("./raport3a.txt", "a");
    if (reportFile == NULL)
        error("Cannot open file");

    struct tms *tms[argc];
    clock_t time[argc];
    for(int i=0; i<argc; i++) {
        tms[i] = calloc(1, sizeof(struct tms *));
        time[i] = 0;
    }

    void *library = dlopen("./liblibrary.so", RTLD_LAZY);
    if(!library)
        error("Cannot open library");

    struct ArrayOfBlocks *(*createArrayOfBlocks)(int) = dlsym(library, "createArrayOfBlocks");
    int (*validPair)(char *) = dlsym(library, "validPair");
    struct Sequence *(*makeSequence)(char *) = dlsym(library, "makeSequence");
    char **(*compareFiles)(struct Sequence *) = dlsym(library, "compareFiles");
    int (*makeBlock)(struct ArrayOfBlocks *, char *) = dlsym(library, "makeBlock");
    void (*removeOperation)(struct ArrayOfBlocks *, int, int) = dlsym(library, "removeOperation");
    void (*removeBlock)(struct ArrayOfBlocks *, int) = dlsym(library, "removeBlock");

    struct ArrayOfBlocks *arrayOfBlocks = createArrayOfBlocks(argc);
    struct Sequence *sequence;

    int *blockIndices = calloc(SIZE, sizeof(int*));

    int isSthToRemove = 0;

    for(int i=1; i<argc; i++) {
        time[i] = times(tms[i]);

        if(strcmp(argv[i], "create_table") == 0) {

            if (i+1 >= argc || !isNumber(argv[i+1]))
                error("Invalid arguments");

            i += 1;

            printf("Creating table of size %s:\n", argv[i]);
            fprintf(reportFile, "Creating table of size %s:\n", argv[i]);
        }
        else if(strcmp(argv[i], "compare_pairs") == 0) {
            char *pairs = calloc(SIZE*SIZE, sizeof(char*));
            strcpy(pairs, "");

            while(i + 1 < argc && validPair(argv[i+1])) {
                strcat(pairs, argv[i+1]);
                strcat(pairs, " ");
                i += 1;
            }

            sequence = makeSequence(pairs);
            char **tmpFiles;
            tmpFiles = compareFiles(sequence);

            int fileIndex = 0;

            if(arrayOfBlocks == NULL)
                error("Invalid arguments");
            else {
                while(fileIndex < SIZE && strcmp(tmpFiles[fileIndex], "") != 0) {
                    blockIndices[fileIndex] = makeBlock(arrayOfBlocks, tmpFiles[fileIndex]);
                    fileIndex++;
                }
            }

            free(tmpFiles);

            isSthToRemove = 1;

            printf("Comparing %d pairs:\n", fileIndex);
            fprintf(reportFile, "Comparing %d pairs:\n", fileIndex);
        }
        else if(strcmp(argv[i], "remove_block") == 0) {
            if (i+1 >= argc || !isNumber(argv[i+1]) || arrayOfBlocks == NULL)
                error("Invalid arguments");

            char *rest;
            int indexOfBlock = (int) strtol(argv[i+1], &rest, 10);

            removeBlock(arrayOfBlocks, indexOfBlock);

            i += 1;

            printf("Removing block of index %d:\n", indexOfBlock);
            fprintf(reportFile, "Removing block of index %d:\n", indexOfBlock);
        }
        else if(strcmp(argv[i], "remove_operation") == 0) {
            if (i+2 >= argc || !isNumber(argv[i+1]) || !isNumber(argv[i+2]) || isSthToRemove == 0)
                error("Invalid arguments");

            char *rest;
            int indexOfBlock = (int) strtol(argv[i+1], &rest, 10);
            int indexOfOperation = (int) strtol(argv[i+2], &rest, 10);

            removeOperation(arrayOfBlocks, indexOfBlock, indexOfOperation);
            i += 2;

            printf("Removing operation of index %d in the block of index %d:\n", indexOfOperation, indexOfBlock);
            fprintf(reportFile, "Removing operation of index %d in the block of index %d:\n",
                    indexOfOperation, indexOfBlock);
        }
        else
            error("Invalid arguments");


        time[i] = times(tms[i]);
        printResult(time[i-1], time[i], tms[i-1], tms[i], reportFile);
    }

    fclose(reportFile);

    free(reportFile);
}

