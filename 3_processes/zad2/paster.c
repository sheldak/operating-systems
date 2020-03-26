#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define SIZE 256
const char *listsDir = "lists/";
const char *matricesDir = "matrices/";
const int blockSize = 5;

int getNumSize(int num) {
    int size = 1;
    if(num < 0) {
        size += 1;
        num *= -1;
    }
    while(num / 10 > 0){
        size++;
        num /= 10;
    }

    return size;
}

void getLineFromList(int fileDesc, int index, char *inNameA, char *inNameB, char *outName) {
    char *currentLetter = calloc(1, sizeof(char));
    int currIndex = 0;
    while(currIndex < index && read(fileDesc, currentLetter, 1) == 1) {
        if(strcmp(currentLetter, "\n") == 0)
            currIndex++;
    }

    int currFile = 0;

    while(currFile < 3 && read(fileDesc, currentLetter, 1) == 1) {
        if (strcmp(currentLetter, " ") == 0 || strcmp(currentLetter, "\n") == 0)
            currFile++;
        else {
            switch (currFile) {
                case 0:
                    strcat(inNameA, currentLetter);
                    break;
                case 1:
                    strcat(inNameB, currentLetter);
                    break;
                case 2:
                    strcat(outName, currentLetter);
                    break;
                default:
                    break;
            }
        }
    }
}

int getMultiplicationsNum(int fileDesc) {
    int multiNum = 0;

    char *currChar = calloc(1, sizeof(char));
    char *rest = calloc(1, sizeof(char));
    int currDigit;

    read(fileDesc, currChar, 1);
    while(strcmp(currChar, " ") != 0) {
        currDigit = (int) strtol(currChar, &rest, 10);
        if(strcmp(rest, "") != 0)
            perror("invalid list file, first word should be a number");

        multiNum *= 10;
        multiNum += currDigit;
        read(fileDesc, currChar, 1);
    }

    return multiNum;
}

void getFirstLineFromMatrix(int fileDesc, int *rows, int *columns) {
    *rows = 0;
    *columns = 0;

    char *currChar = calloc(1, sizeof(char));
    char *rest = calloc(1, sizeof(char));
    int currDigit;

    int currVariable = 0;
    read(fileDesc, currChar, 1);
    while(currVariable < 2 && strcmp(currChar, "\n") != 0) {
        if(strcmp(currChar, " ") == 0)
            currVariable++;
        else {
            currDigit = (int) strtol(currChar, &rest, 10);
            if(strcmp(rest, "") != 0)
                perror("invalid matrix file, first three words should be numbers");

            if(currVariable == 0) {
                *rows *= 10;
                *rows += currDigit;
            }
            else {
                *columns *= 10;
                *columns += currDigit;
            }
        }
        read(fileDesc, currChar, 1);
    }
}

void getFirstLineFromMatrixB(int fileDesc, int *rows, int *columns, int *startColumn) {
    *rows = 0;
    *columns = 0;
    *startColumn = 0;

    char *currChar = calloc(1, sizeof(char));
    char *rest = calloc(1, sizeof(char));
    int currDigit;

    int currVariable = 0;
    read(fileDesc, currChar, 1);
    while(currVariable < 3 && strcmp(currChar, "\n") != 0) {
        if(strcmp(currChar, " ") == 0)
            currVariable++;
        else {
            currDigit = (int) strtol(currChar, &rest, 10);
            if(strcmp(rest, "") != 0)
                perror("invalid matrix file, first three words should be numbers");

            switch(currVariable) {
                case 0:
                    *rows *= 10;
                    *rows += currDigit;
                    break;
                case 1:
                    *columns *= 10;
                    *columns += currDigit;
                    break;
                case 2:
                    *startColumn *= 10;
                    *startColumn += currDigit;
                    break;
                default:
                    break;
            }
        }
        read(fileDesc, currChar, 1);
    }
}

char *getTmpSeparateFile(char *fileName, int columnsBlock) {
    char *tmpNum = calloc(SIZE, sizeof(char));
    sprintf(tmpNum, "tmp%d_", columnsBlock);

    char *filePath = calloc(SIZE, sizeof(char));
    strcpy(filePath, matricesDir);
    strcat(filePath, tmpNum);
    strcat(filePath, fileName);

    return filePath;
}

char *getTmpPasteFile(int i) {
    char *tmpName = calloc(SIZE, sizeof(char));
    sprintf(tmpName, "tmp%d.csv", i);

    char *filePath = calloc(SIZE, sizeof(char));
    strcpy(filePath, matricesDir);
    strcat(filePath, tmpName);

    return filePath;
}

int main(int argc, char **argv) {
    if(argc < 2)
        perror("too few arguments in paster");

    char *listPath = calloc(SIZE, sizeof(char));
    strcpy(listPath, listsDir);
    strcat(listPath, argv[1]);

    int fileDesc = open(listPath, O_RDONLY);

    if(fileDesc < 0)
        perror("problem with opening file, maybe invalid first argument");

    int linesNum = getMultiplicationsNum(fileDesc);
    lseek(fileDesc, 0, SEEK_SET);

    int currLine = 0;
    while(currLine < linesNum) {
        char *inputFileName1 = calloc(SIZE, sizeof(char));
        char *inputFileName2 = calloc(SIZE, sizeof(char));
        char *outputFileName = calloc(SIZE, sizeof(char));

        // getting line from list file
        getLineFromList(fileDesc, currLine+1, inputFileName1, inputFileName2, outputFileName);
        lseek(fileDesc, 0, SEEK_SET);

        // getting path of matrices
        char *inputFilePath1 = calloc(SIZE, sizeof(char));
        strcpy(inputFilePath1, matricesDir);
        strcat(inputFilePath1, inputFileName1);

        char *inputFilePath2 = calloc(SIZE, sizeof(char));
        strcpy(inputFilePath2, matricesDir);
        strcat(inputFilePath2, inputFileName2);

        // getting size of matrices
        int matrixAFileDesc = open(inputFilePath1, O_RDONLY);
        int rowsA, columnsA;
        getFirstLineFromMatrix(matrixAFileDesc, &rowsA, &columnsA);
        close(matrixAFileDesc);

        int matrixBFileDesc = open(inputFilePath2, O_RDONLY);
        int rowsB, columnsB, st;
        getFirstLineFromMatrixB(matrixBFileDesc, &rowsB, &columnsB, &st);
        close(matrixBFileDesc);

        // size of matrix to output file
        char *outputFilePath = calloc(SIZE, sizeof(char));
        strcpy(outputFilePath, matricesDir);
        strcat(outputFilePath, outputFileName);

        char *lineToOutputFile = calloc(SIZE, sizeof(char));
        sprintf(lineToOutputFile, "%d %d\n", rowsA, columnsB);
        int size = 2 + getNumSize(rowsA) + getNumSize(columnsB);
        FILE *matrixCFile = fopen(outputFilePath, "w");
        fwrite(lineToOutputFile, size, sizeof(char), matrixCFile);
        fclose(matrixCFile);


        // pasting files together
        int i = 0;
        if ((columnsB-1)/blockSize + 1 > 1) {
            char *command = calloc(SIZE, sizeof(char));
            strcpy(command, "paste -d \" \" ");
            strcat(command, getTmpSeparateFile(outputFileName, 0));
            strcat(command, " ");
            strcat(command, getTmpSeparateFile(outputFileName, 1));
            strcat(command, " > ");
            strcat(command, getTmpPasteFile(0));

            system(command);

            for(i=1; i < (columnsB-1)/blockSize; i++) {
                memset(command, 0, sizeof(char));
                strcpy(command, "paste -d \" \" ");
                strcat(command, getTmpPasteFile(i-1));
                strcat(command, " ");
                strcat(command, getTmpSeparateFile(outputFileName, i+1));
                strcat(command, " > ");
                strcat(command, getTmpPasteFile(i));

                system(command);
            }

            memset(command, 0, sizeof(char));
            strcpy(command, "cat ");
            strcat(command, getTmpPasteFile(i-1));
            strcat(command, " >> ");
            strcat(command, outputFilePath);

            system(command);
        }
        currLine++;
    }

    return 0;
}