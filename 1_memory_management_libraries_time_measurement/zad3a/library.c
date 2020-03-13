#include "library.h"

#define SIZE 150
#define DIR "textFiles/"

struct ArrayOfBlocks *createArrayOfBlocks(int size) {
    struct ArrayOfBlocks *arrayOfBlocks = malloc(sizeof(struct ArrayOfBlocks));

    arrayOfBlocks->blocks = (struct Block*) calloc(size, sizeof(struct Block));
    arrayOfBlocks->size = 0;

    return arrayOfBlocks;
};

int validPair(char *potentialPair) {
    for(int i=0; i<strlen(potentialPair); i++) {
        if(potentialPair[i] == ':')
            return 1;
    }
    return 0;
};

struct Sequence *makeSequence(char *pairs) {
    struct Sequence *sequence = malloc(sizeof(struct Sequence*));
    sequence->filesPair = malloc(sizeof(struct FilesPair));

    struct FilesPair *currPair = sequence->filesPair;

    char *file1 = calloc(SIZE, sizeof(char));
    char *file2 = calloc(SIZE, sizeof(char));

    int curr_file = 0;

    int pairsSize = (int) strlen(pairs);

    for(int i=0; i<pairsSize; i++) {
        if (pairs[i] != ' ' && pairs[i] != ':') {
            char charToAdd[1];
            charToAdd[0] = pairs[i];

            if (curr_file == 0)
                strcat(file1, charToAdd);
            else
                strcat(file2, charToAdd);
        }

        if (pairs[i+1] == ' ' || pairs[i+1] == ':') {
            if (curr_file == 0 && pairs[i+1] != ' ') {
                currPair->file1 = calloc(SIZE, sizeof(char));

                strcpy(currPair->file1, file1);
                memset(file1, 0, SIZE);
            }
            else if (curr_file == 1 && pairs[i+1] != ':') {
                currPair->file2 = calloc(SIZE, sizeof(char));
                strcpy(currPair->file2, file2);
                memset(file2, 0, SIZE);

                struct FilesPair *newPair = malloc(sizeof(struct FilesPair));
                currPair->next = newPair;
                currPair = newPair;
            }
            else {
               perror("Invalid names of files");
               exit(0);
            }
            curr_file = (curr_file + 1) % 2;
        }
    }

    free(file1);
    free(file2);

    if (curr_file == 1) {
        perror("Invalid number of files");
        exit(0);
    }

    return sequence;
};

char **compareFiles(struct Sequence *sequence) {
    struct FilesPair *currPair = sequence->filesPair;
    int fileIndex = 0;
    char filename[9] = "tmp00.txt";

    char **tmpFiles = (char**) calloc(SIZE, sizeof(char*));
    for(int i=0; i<SIZE; i++)
        tmpFiles[i] = (char*) calloc(SIZE, sizeof(char*));

    while (currPair != NULL && currPair->file1 != NULL && currPair->file2 != NULL) {
        filename[4] = (char) ((fileIndex % 10) + 48 );
        strcpy(tmpFiles[fileIndex], filename);

        char *command = calloc(SIZE, sizeof(char));
        strcpy(command, "diff ");
        strcat(command, DIR);
        strcat(command, currPair->file1);
        strcat(command, " ");
        strcat(command, DIR);
        strcat(command, currPair->file2);
        strcat(command, " > ");
        strcat(command, filename);

        int res = system(command);
        if (res == -1) {
            perror("Cannot execute command");
            exit(0);
        }

        currPair = currPair -> next;

        fileIndex += 1;
        if(fileIndex >= 10)
            filename[3] = (char) ((fileIndex / 10) + 48);
    }
    currPair = NULL;
    free(currPair);

    return tmpFiles;
}

int makeBlock(struct ArrayOfBlocks *arrayOfBlocks, char *filename) {
    // making new block
    struct Block newBlock;
    newBlock.operations = (struct Operation*) calloc(SIZE, sizeof(struct Operation));
    newBlock.index = arrayOfBlocks->size;

    // opening temporary file
    char *fullFilename = calloc(SIZE, sizeof(char));
    strcat(fullFilename, filename);

    FILE *file;

    if ((file = fopen(fullFilename, "r")) == NULL){
        perror("Problem with opening file");
        exit(1);
    }

    // looking for the size of the file
    fseek(file, 0, SEEK_END);
    int fileSize = (int) ftell(file);
    fseek(file, 0, SEEK_SET);

    // getting contents of the file
    char *operationContents = calloc(fileSize+1, sizeof(char));

    int operationIndex = 0;

    char *line = calloc(fileSize+1, sizeof(char));
    while (!feof(file)) {
        if ((fgets(line, SIZE, file)) != NULL) {
            // checking if new operation
            if ((int) line[0] >= 48 && (int) line[0] <= 57 && (strcmp(operationContents, "") != 0)) {
                newBlock.operations[operationIndex].index = operationIndex;
                newBlock.operations[operationIndex].contents = calloc(fileSize+1, sizeof(char));
                strcpy(newBlock.operations[operationIndex].contents, operationContents);
                operationIndex++;

                memset(operationContents, 0, fileSize+1);
            }
            strcat(operationContents, line);
        }
    }
    newBlock.operations[operationIndex].index = operationIndex;
    newBlock.operations[operationIndex].contents = calloc(fileSize+1, sizeof(char));
    strcpy(newBlock.operations[operationIndex].contents, operationContents);

    newBlock.size = operationIndex + 1;

    fclose(file);

    arrayOfBlocks->blocks[arrayOfBlocks->size] = newBlock;
    arrayOfBlocks->size++;
    return arrayOfBlocks->size - 1;
};

int numberOfOperations(struct Block block) {
    return block.size;
};

void removeOperation(struct ArrayOfBlocks *arrayOfBlocks, int blockIndex, int operationIndex) {
    if (blockIndex >= arrayOfBlocks->size) {
        perror("Invalid index of block");
        exit(0);
    }

    if (operationIndex >= arrayOfBlocks->blocks[blockIndex].size) {
        perror("Invalid index of operation");
        exit(0);
    }

    free(arrayOfBlocks->blocks[blockIndex].operations[operationIndex].contents);
};

void removeBlock(struct ArrayOfBlocks *arrayOfBlocks, int blockIndex) {
    if (blockIndex >= arrayOfBlocks->size) {
        perror("Invalid index of block");
        exit(0);
    }

    for (int i=0; i<arrayOfBlocks->blocks[blockIndex].size; i++)
        removeOperation(arrayOfBlocks, blockIndex, i);

    free(arrayOfBlocks->blocks[blockIndex].operations);
    arrayOfBlocks->blocks[blockIndex].size = 0;
};


