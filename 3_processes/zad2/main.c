#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <ftw.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/file.h>
#include <errno.h>

#define SIZE 256
#define COMMON 0
#define SEPARATE 1

const char *listsDir = "lists/";
const char *matricesDir = "matrices/";
const int blockSize = 5;

char *getTmpListPath(char *listName) {
    char *tmpListPath = calloc(SIZE, sizeof(char));
    strcpy(tmpListPath, listsDir);
    strcat(tmpListPath, "tmp_");
    strcat(tmpListPath, listName);

    return tmpListPath;
}

char *getTmpMatrixPath(char *matrixName) {
    char *tmpMatrixPath = calloc(SIZE, sizeof(char));
    strcpy(tmpMatrixPath, matricesDir);
    strcat(tmpMatrixPath, "tmp_");
    strcat(tmpMatrixPath, matrixName);

    return tmpMatrixPath;
}

void copyFile(char *filename1, char *filename2) {
    int fileDesc1 = open(filename1, O_RDONLY);
    if(fileDesc1 < 0)
        perror("problem with opening first file when copying (sys)");

    creat(filename2, 0666);
    int fileDesc2 = open(filename2, O_WRONLY);
    if(fileDesc2 < 0)
        perror("problem with opening second file when copying (sys)");

    char *currLetter = calloc(SIZE, sizeof(char));

    while(read(fileDesc1, currLetter, 1) == 1)
        write(fileDesc2, currLetter, 1);

    close(fileDesc1);
    close(fileDesc2);
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

int getCurrMultiplicationIndex(int fileDesc) {
    int multiIndex = 0;

    char *currChar = calloc(1, sizeof(char));
    char *rest = calloc(1, sizeof(char));
    int currDigit;

    read(fileDesc, currChar, 1);
    while(strcmp(currChar, "\n") != 0) {
        currDigit = (int) strtol(currChar, &rest, 10);
        if(strcmp(rest, "") != 0)
            perror("invalid list file, second word should be a number and end with newline character");

        multiIndex *= 10;
        multiIndex += currDigit;
        read(fileDesc, currChar, 1);
    }

    return multiIndex;
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

void getFirstLineFromMatrixA(int fileDesc, int *rows, int *columns) {
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

int **getMatrixA(int fileDesc, int *rows, int *columns) {
    getFirstLineFromMatrixA(fileDesc, rows, columns);

    // matrix initialization
    int **matrix = calloc(*rows, sizeof(int*));
    for(int i=0; i<*rows; i++)
        matrix[i] = calloc(*columns, sizeof(int));

    // transcribe from file to matrix of integers
    for(int row=0; row<*rows; row++) {
        int column = 0;

        int currValue = 0;
        char *currChar = calloc(1, sizeof(char));
        char *rest = calloc(1, sizeof(char));
        int currDigit;

        read(fileDesc, currChar, 1);
        while(column < *columns) {
            if(strcmp(currChar, " ") == 0 || strcmp(currChar, "\n") == 0) {
                matrix[row][column] = currValue;
                currValue = 0;

                column++;

                if(strcmp(currChar, "\n") == 0)
                    break;
            }
            else {
                currDigit = (int) strtol(currChar, &rest, 10);
                if(strcmp(rest, "") != 0)
                    perror("invalid matrix file, there should be just numbers");

                currValue *= 10;
                currValue += currDigit;
            }

            if (read(fileDesc, currChar, 1) == 0) // checking case of EOF
                currChar = "\n";
        }
    }

    return matrix;
}

int **getMatrixBBlock(int fileDesc, int *blockRows, int *blockColumns, int *endOfMatrix) {
    int rows, columns, startColumn;
    getFirstLineFromMatrixB(fileDesc, &rows, &columns, &startColumn);

    if(startColumn == columns) // this matrix was all read
        return NULL;
    else {
        // number of columns to take
        int columnsTaken;
        if(columns - startColumn > blockSize) {
            columnsTaken = blockSize;
            *endOfMatrix = 0;
        }
        else {
            columnsTaken = columns - startColumn;
            *endOfMatrix = 1;
        }

        // matrix initialization
        int **matrix = calloc(rows, sizeof(int*));
        for(int i=0; i<rows; i++)
            matrix[i] = calloc(columnsTaken, sizeof(int));

        // transcribe 'columnsTaken' columns from file to matrix
        for(int row=0; row<rows; row++) {
            int column = 0;

            int currValue = 0;
            char *currChar = calloc(1, sizeof(char));
            char *rest = calloc(1, sizeof(char));
            int currDigit;

            read(fileDesc, currChar, 1);
            while(column < columns) {
                if(strcmp(currChar, " ") == 0 || strcmp(currChar, "\n") == 0) {
                    if(column >= startColumn && column < startColumn + columnsTaken) {
                        matrix[row][column-startColumn] = currValue;
                        currValue = 0;
                    }
                    column++;

                    if(strcmp(currChar, "\n") == 0)
                        break;
                }
                else if(column >= startColumn && column < startColumn + columnsTaken) {
                    currDigit = (int) strtol(currChar, &rest, 10);
                    if(strcmp(rest, "") != 0)
                        perror("invalid matrix file, there should be just numbers");

                    currValue *= 10;
                    currValue += currDigit;
                }

                if (read(fileDesc, currChar, 1) == 0) // checking case of EOF
                    currChar = "\n";
            }
        }

        // updating file
        lseek(fileDesc, 0, SEEK_SET);
        char *currChar = calloc(1, sizeof(char));
        int spaces = 0;

        read(fileDesc, currChar, 1);
        while(spaces < 2) {
            if(strcmp(currChar, " ") == 0)
                spaces++;

            read(fileDesc, currChar, 1);
        }
        lseek(fileDesc, -1, SEEK_CUR);

        char *newStartColumn = calloc(3, sizeof(char));

        if(startColumn + columnsTaken >= 10)
            sprintf(newStartColumn, "%d", startColumn + columnsTaken);
        else {
            char *tmpColumn = calloc(2, sizeof(char));
            sprintf(tmpColumn, "%d", startColumn + columnsTaken);
            strcpy(newStartColumn, "0");
            strcat(newStartColumn, tmpColumn);
        }
        newStartColumn[2] = '\n';

        write(fileDesc, newStartColumn, 3);

        // matrix values and size
        *blockRows = rows;
        *blockColumns = columnsTaken;

        return matrix;
    }
}

int makeMultiplication(char *listName) {
    // reading list file
    char *tmpListPath = getTmpListPath(listName);

    int listFileDesc = open(tmpListPath, O_RDWR);
    while(flock(listFileDesc, LOCK_EX | LOCK_NB) == -1) {}

    int multiNum = getMultiplicationsNum(listFileDesc);
    int multiIndex = getCurrMultiplicationIndex(listFileDesc);

    if (multiNum == multiIndex) { // all multiplications are done or other processes will end them
        flock(listFileDesc, LOCK_UN);
        close(listFileDesc);
        return -1;
    }
    else {
        // files with matrices
        char *inputFileNameA = calloc(SIZE, sizeof(char));
        char *inputFileNameB = calloc(SIZE, sizeof(char));
        char *outputFileName = calloc(SIZE, sizeof(char));

        getLineFromList(listFileDesc, multiIndex, inputFileNameA, inputFileNameB, outputFileName);
        char *tmpMatrixAPath = getTmpMatrixPath(inputFileNameA);
        char *tmpMatrixBPath = getTmpMatrixPath(inputFileNameB);

        // opening file with matrix B to read a block (it is necessary to block other processes from editing)
        // starting with B because matrix B may cause list file edition and it is better to unlock list file earlier
        int matrixBFileDesc = open(tmpMatrixBPath, O_RDWR);
        while(flock(matrixBFileDesc, LOCK_EX | LOCK_NB) == -1) {}

        int matrixBRows, matrixBColumns, endOfMatrix;
        int **matrixB = getMatrixBBlock(matrixBFileDesc, &matrixBRows, &matrixBColumns, &endOfMatrix);

        flock(matrixBFileDesc, LOCK_UN);
        close(matrixBFileDesc);

        // checking if matrix B is all read
        if(endOfMatrix == 1) {
            // updating file
            int multiNumOffset = 1;
            if(multiNum >= 10)
                multiNumOffset++;

            lseek(listFileDesc, multiNumOffset+1, SEEK_SET);

            char *newIndex = calloc(3, sizeof(char));

            if(multiIndex + 1 >= 10)
                sprintf(newIndex, "%d", multiIndex + 1);
            else {
                char *tmpIndex = calloc(2, sizeof(char));
                sprintf(tmpIndex, "%d", multiIndex + 1);
                strcpy(newIndex, "0");
                strcat(newIndex, tmpIndex);
            }
            newIndex[2] = '\n';

            write(listFileDesc, newIndex, 3);
        }

        flock(listFileDesc, LOCK_UN);
        close(listFileDesc);

        if(!matrixB)
            perror("previous process has not written the fact of the end of matrix file to list file");
        else {
            // getting matrix A
            int matrixAFileDesc = open(tmpMatrixAPath, O_RDONLY);

            int matrixARows, matrixAColumns;
            int **matrixA = getMatrixA(matrixAFileDesc, &matrixARows, &matrixAColumns);

            close(matrixAFileDesc);

            for(int i=0; i<matrixARows; i++) {
                for(int j=0; j<matrixAColumns; j++)
                    printf("%d ", matrixA[i][j]);
                printf("\n");
            }
            printf("\n");

            for(int i=0; i<matrixBRows; i++) {
                for(int j=0; j<matrixBColumns; j++)
                    printf("%d ", matrixB[i][j]);
                printf("\n");
            }

        }


    }
    return 0;
}


int main(int argc, char **argv) {
    if(argc < 5)
        perror("too few arguments");

    // first argument, list file containing names of files with matrices and output file with future result

    // copying list file to tmp file
    char *listPath = calloc(SIZE, sizeof(char));
    strcpy(listPath, listsDir);
    strcat(listPath, argv[1]);

    char *tmpListPath = getTmpListPath(argv[1]);
    copyFile(listPath, tmpListPath);

    // copying matrices files to tmp files
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

        getLineFromList(fileDesc, currLine+1, inputFileName1, inputFileName2, outputFileName);
        lseek(fileDesc, 0, SEEK_SET);

        char *inputFilePath1 = calloc(SIZE, sizeof(char));
        strcpy(inputFilePath1, matricesDir);
        strcat(inputFilePath1, inputFileName1);

        char *tmpInputFilePath1 = getTmpMatrixPath(inputFileName1);
        copyFile(inputFilePath1, tmpInputFilePath1);

        char *inputFilePath2 = calloc(SIZE, sizeof(char));
        strcpy(inputFilePath2, matricesDir);
        strcat(inputFilePath2, inputFileName2);

        char *tmpInputFilePath2 = getTmpMatrixPath(inputFileName2);
        copyFile(inputFilePath2, tmpInputFilePath2);

        currLine++;
    }

    close(fileDesc);



    makeMultiplication(argv[1]);



    // second argument, number of processes
    char *rest;
    int processesNum = (int) strtol(argv[2], &rest ,10);

    if(strcmp(rest, "") != 0)
        perror("invalid second argument");

    // third argument, max time for multiplication
    rest = "";
    int maxTime = (int) strtol(argv[3], &rest ,10);

    if(strcmp(rest, "") != 0)
        perror("invalid third argument");

    // forth argument, results of multiplication are in COMMON file, or in SEPARATE files
    int resultsSaving;
    if(strcmp(argv[4], "common") == 0)
        resultsSaving = COMMON;
    else if(strcmp(argv[4], "separate") == 0)
        resultsSaving = SEPARATE;
    else
        perror("invalid forth argument");

//    pid_t pid1 = fork();
//    pid_t pid2 = -1;
//    if(pid1 != 0)
//        pid2 = fork();

//    if(pid1 == 0 || pid2 == 0) {
//        fileDesc = open("lists/listTmp.txt", O_RDWR);
////        printf("%d\n", flock(fileDesc, LOCK_NB));
//        while(flock(fileDesc, LOCK_EX | LOCK_NB) == -1) {
//
//        }
//        printf("%d\n", flock(fileDesc, LOCK_EX | LOCK_NB));
////        printf("%s\n", errno);
//        char *tr = calloc(1, sizeof(char));
//        read(fileDesc, tr, 1);
//        lseek(fileDesc, 0, SEEK_SET);
//        printf("%s\n", tr);
////
//        if(strcmp(tr, "3") ==0)
//            write(fileDesc, "4", 1);
//        else {
//            write(fileDesc, "3", 1);
//        }
//
//        flock(fileDesc, LOCK_UN);
//    }

    return 0;
}