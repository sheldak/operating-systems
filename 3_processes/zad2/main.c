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

struct matrix {
    int rows;
    int columns;
    int **table;
};

struct matrixSpecs {
    int blocks;
    int *blocksOfColumns;
};

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

struct matrix getMatrixA(int fileDesc) {
    int rows, columns;
    getFirstLineFromMatrix(fileDesc, &rows, &columns);

    struct matrix m;
    m.rows = rows;
    m.columns = columns;

    // matrix initialization
    m.table = calloc(rows, sizeof(int*));
    for(int i=0; i<rows; i++)
        m.table[i] = calloc(columns, sizeof(int));

    // transcribe from file to matrix of integers
    for(int row=0; row<rows; row++) {
        int column = 0;

        int currValue = 0;
        char *currChar = calloc(1, sizeof(char));
        char *rest = calloc(1, sizeof(char));
        int currDigit;

        read(fileDesc, currChar, 1);
        while(column < columns) {
            if(strcmp(currChar, " ") == 0 || strcmp(currChar, "\n") == 0) {
                m.table[row][column] = currValue;
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

    return m;
}

struct matrix getMatrixBBlock(int fileDesc, int *blockIndex, int *endOfMatrix) {
    int rows, columns, startColumn;
    getFirstLineFromMatrixB(fileDesc, &rows, &columns, &startColumn);

    struct matrix m;

    if(startColumn == columns) // this matrix was all read
        m.table = NULL;
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

        // index of current block
        *blockIndex = startColumn / blockSize;

        // matrix initialization
        m.table = calloc(rows, sizeof(int*));
        for(int i=0; i<rows; i++)
            m.table[i] = calloc(columnsTaken, sizeof(int));

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
                        m.table[row][column-startColumn] = currValue;
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
        m.rows = rows;
        m.columns = columnsTaken;
    }

    return m;
}

struct matrix multiply(struct matrix mA, struct matrix mB, int aBlockRows, int blockIndex) {
    struct matrix m;
    m.table = calloc(aBlockRows, sizeof(int*));
    for(int i=0; i<aBlockRows; i++)
        m.table[i] = calloc(mB.columns, sizeof(int));

    for(int i = blockIndex*blockSize; i < blockIndex*blockSize + aBlockRows; i++) {
        for(int j=0; j<mB.columns; j++) {
            int sum = 0;
            for(int k=0; k<mA.columns; k++)
                sum += mA.table[i][k] * mB.table[k][j];
            m.table[i - blockIndex*blockSize][j] = sum;
        }
    }

    m.rows = aBlockRows;
    m.columns = mB.columns;

    return m;
}

struct matrixSpecs getMatrixSpecifications(int fileDesc) {
    struct matrixSpecs mSpecs;

    mSpecs.blocks = 0;

    int currNum = 0;
    int *tmpTable = calloc(SIZE, sizeof(int));
    int currDigit = 0;
    char *rest = calloc(1, sizeof(char));

    char *currChar = calloc(1, sizeof(char));
    while(read(fileDesc, currChar, 1) != 0) {
        if(strcmp(currChar, "\n") == 0) {
            tmpTable[mSpecs.blocks++] = currNum;
            currNum = 0;
        }
        else {
            currDigit = (int) strtol(currChar, &rest, 10);
            if(strcmp(rest, "") != 0)
                perror("invalid matrix file, there should be just numbers");

            currNum *= 10;
            currNum += currDigit;
        }
    }
    if(mSpecs.blocks == 0 && currNum == 0)
        perror("tmp_out file is built improperly");

    mSpecs.blocksOfColumns = calloc(mSpecs.blocks, sizeof(int));
    for(int i=0; i<mSpecs.blocks; i++)
        mSpecs.blocksOfColumns[i] = tmpTable[i];

    free(tmpTable);

    return mSpecs;
}

struct matrix getMatrixFromOutputFile(int fileDesc, struct matrix mBlock, int rBlockIndex, int cBlockIndex) {
    int rows, columns;
    getFirstLineFromMatrix(fileDesc, &rows, &columns);

    struct matrix outputM;
    outputM.rows = rows;
    if(mBlock.rows + rBlockIndex*blockSize > rows)
        outputM.rows = mBlock.rows + rBlockIndex*blockSize;

    outputM.columns = columns;
    if(mBlock.columns + cBlockIndex*blockSize > columns)
        outputM.columns = mBlock.columns + cBlockIndex*blockSize;

    outputM.table = calloc(outputM.rows, sizeof(int*));
    for(int i=0; i<outputM.rows; i++)
        outputM.table[i] = calloc(outputM.columns, sizeof(int));

    int currNum = 0;
    int currDigit = 0;
    int currRow = 0;
    int currColumn = 0;
    char *rest = calloc(1, sizeof(char));

    char *currChar = calloc(1, sizeof(char));
    while(currRow < rows) {
        if(strcmp(currChar, " ") == 0 || strcmp(currChar, "\n") == 0) {
            outputM.table[currRow][currColumn] = currNum;
            currNum = 0;

            if(strcmp(currChar, " ") == 0)
                currColumn++;
            else {
                currColumn = 0;
                currRow++;
            }
        }
        else {
            currDigit = (int) strtol(currChar, &rest, 10);
            if(strcmp(rest, "") != 0)
                perror("invalid matrix file, there should be just numbers");

            currNum *= 10;
            currNum += currDigit;
        }

        if(read(fileDesc, currChar, 1) == 0)
            currChar = "\n";
    }

    return outputM;
}

void updateMatrix(struct matrix m, struct matrix mBlock, int rBlockIndex, int cBlockIndex) {
    for(int i=0; i<mBlock.rows; i++) {
        for(int j=0; j<mBlock.columns; j++)
            m.table[rBlockIndex*blockSize + i][cBlockIndex*blockSize + j] = mBlock.table[i][j];
    }
}

int min(int a, int b) {
    if(a < b)
        return a;
    else
        return b;
}

void writeOutputMatrixToFile(int fileDesc, struct matrix m, struct matrixSpecs mSpecs) {
    lseek(fileDesc, 0, SEEK_SET);

    char *matrixLine = calloc(SIZE, sizeof(char));

    int size = 2 + getNumSize(m.rows) + getNumSize(m.columns);
    sprintf(matrixLine, "%d %d\n", m.rows, m.columns);

    write(fileDesc, matrixLine, size);
    char *num = calloc(SIZE, sizeof(char));
    for(int i=0; i<m.rows; i++) {
        memset(matrixLine, 0, SIZE);
        size = 0;
        for(int j=0; j<min(mSpecs.blocksOfColumns[i/blockSize]*blockSize, m.columns); j++) {
            sprintf(num, "%d ", m.table[i][j]);
            strcat(matrixLine, num);
            size += getNumSize(m.table[i][j])+1;
        }
        matrixLine[size-1] = '\n';
        write(fileDesc, matrixLine, size);
    }
}

void updateTmpOutputFile(int fileDesc, struct matrixSpecs mSpecs) {
    lseek(fileDesc, 0, SEEK_SET);

    for(int i=0; i<mSpecs.blocks; i++) {
        char *line = calloc(SIZE, sizeof(char));
        sprintf(line, "%d\n", mSpecs.blocksOfColumns[i]);

        write(fileDesc, line, 1 + getNumSize(mSpecs.blocksOfColumns[i]));
    }
}

void writeMatrixBlockToFileCommon(char *fileName, struct matrix m, int rBlockIndex, int cBlockIndex) {
    // files paths
    char *filePath = calloc(SIZE, sizeof(char));
    strcpy(filePath, matricesDir);
    strcat(filePath, fileName);

    char *tmpFilePath = getTmpMatrixPath(fileName);

    // getting data (containing information where to put new matrix block) from tmp file
    int tmpFileDesc = open(tmpFilePath, O_RDWR);
    while(flock(tmpFileDesc, LOCK_EX | LOCK_NB) == -1) {}

    struct matrixSpecs mSpecs = getMatrixSpecifications(tmpFileDesc);
    mSpecs.blocksOfColumns[rBlockIndex]++;

    // getting matrix from output file (made by other processes or other iterations of multiplication)
    int fileDesc = open(filePath, O_RDWR);
    while(flock(fileDesc, LOCK_EX | LOCK_NB) == -1) {}

    struct matrix matrixC = getMatrixFromOutputFile(fileDesc, m, rBlockIndex, cBlockIndex);

    // updating matrix from output file by adding a new block of multiplications (matrix m)
    updateMatrix(matrixC, m, rBlockIndex, cBlockIndex);

    // writing matrix to file
    writeOutputMatrixToFile(fileDesc, matrixC, mSpecs);

    // updating temporary output file
    updateTmpOutputFile(tmpFileDesc, mSpecs);

    // unlocking files
    flock(fileDesc, LOCK_UN);
    flock(tmpFileDesc, LOCK_UN);
    close(fileDesc);
    close(tmpFileDesc);

//    for(int i=0; i<matrixC.rows; i++) {
//        for(int j=0; j<matrixC.columns; j++)
//            printf("%d ", matrixC.table[i][j]);
//        printf("\n");
//    }
//    printf("\n");


    if(!mSpecs.blocksOfColumns)
        printf("S\n");

}

int makeMultiplication(char *listName, int resultsSaving, int *multiplications) {
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

        int blockBIndex, endOfMatrix;
        struct matrix matrixB = getMatrixBBlock(matrixBFileDesc, &blockBIndex, &endOfMatrix);

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

        if(!matrixB.table)
            perror("previous process has not written the fact of the end of matrix file to list file");
        else {
            // getting matrix A
            int matrixAFileDesc = open(tmpMatrixAPath, O_RDONLY);

            struct matrix matrixA = getMatrixA(matrixAFileDesc);

            close(matrixAFileDesc);

            if(matrixA.columns != matrixB.rows)
                perror("invalid size of matrices, cannot multiply them");

            int blocksOfMultiplication = matrixA.rows / blockSize + 1;

            if(matrixA.rows % blockSize == 0)
                blocksOfMultiplication--;

            for(int blockAIndex=0; blockAIndex<blocksOfMultiplication; blockAIndex++) {
                int rowsAToMultiply = blockSize;
                if(blockAIndex == blocksOfMultiplication - 1)
                    rowsAToMultiply = (matrixA.rows-1) % blockSize + 1;

                struct matrix resultMatrix = multiply(matrixA, matrixB, rowsAToMultiply, blockAIndex);

                if(resultsSaving == COMMON)
                    writeMatrixBlockToFileCommon(outputFileName, resultMatrix, blockAIndex, blockBIndex);

                *multiplications += 1;

//                for(int k=0; k<matrixA.rows; k++) {
//                    for(int j=0; j<matrixA.columns; j++)
//                        printf("%d ", matrixA.table[k][j]);
//                    printf("\n");
//                }
//                printf("\n");
//
//                for(int k=0; k<matrixB.rows; k++) {
//                    for(int j=0; j<matrixB.columns; j++)
//                        printf("%d ", matrixB.table[k][j]);
//                    printf("\n");
//                }
//                printf("\n");
//
//                for(int k=0; k<rowsAToMultiply; k++) {
//                    for(int j=0; j<matrixB.columns; j++)
//                        printf("%d ", resultMatrix.table[k][j]);
//                    printf("\n");
//                }
            }



        }


    }
    return 0;
}

// TODO liczby ujemmne
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
    int resultsSaving = -1;
    if(strcmp(argv[4], "common") == 0)
        resultsSaving = COMMON;
    else if(strcmp(argv[4], "separate") == 0)
        resultsSaving = SEPARATE;
    else
        perror("invalid forth argument");

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

        // getting line from list file
        getLineFromList(fileDesc, currLine+1, inputFileName1, inputFileName2, outputFileName);
        lseek(fileDesc, 0, SEEK_SET);

        // creating a copy of matrix A file
        char *inputFilePath1 = calloc(SIZE, sizeof(char));
        strcpy(inputFilePath1, matricesDir);
        strcat(inputFilePath1, inputFileName1);

        char *tmpInputFilePath1 = getTmpMatrixPath(inputFileName1);
        copyFile(inputFilePath1, tmpInputFilePath1);

        // creating a copy of matrix B file
        char *inputFilePath2 = calloc(SIZE, sizeof(char));
        strcpy(inputFilePath2, matricesDir);
        strcat(inputFilePath2, inputFileName2);

        char *tmpInputFilePath2 = getTmpMatrixPath(inputFileName2);
        copyFile(inputFilePath2, tmpInputFilePath2);

        // creating a temporary output file which will be using during multiplication
        char *tmpOutputFilePath = getTmpMatrixPath(outputFileName);
        char *command = calloc(SIZE, sizeof(char));
        strcpy(command, "touch ");
        strcat(command, tmpOutputFilePath);
        system(command);

        // preparing output file
        char *outputFilePath = calloc(SIZE, sizeof(char));
        strcpy(outputFilePath, matricesDir);
        strcat(outputFilePath, outputFileName);
        char *lineToOutputFile = "0 0\n";
        FILE *matrixCFile = fopen(outputFilePath, "w");
        fwrite(lineToOutputFile, 4, sizeof(char), matrixCFile);
        fclose(matrixCFile);

        // preparing temporary output file
        int matrixAFileDesc = open(inputFilePath1, O_RDONLY);
        int rows, columns;
        getFirstLineFromMatrix(matrixAFileDesc, &rows, &columns);
        close(matrixAFileDesc);

        int matrixCTmpFileDesc = open(tmpOutputFilePath, O_WRONLY);
        for(int i=0; i<rows/blockSize + 1; i++)
            write(matrixCTmpFileDesc, "0\n", 2);
        close(matrixCTmpFileDesc);

        currLine++;
    }

    close(fileDesc);

    int multiplications = 0;

    int myPID = -1;
    for(int i=0; i<processesNum && myPID != 0; i++)
        myPID = fork();

    if(myPID == 0)
        while(makeMultiplication(argv[1], resultsSaving, &multiplications) != -1){}
    else {
        for(int i=0; i<processesNum; i++) {
            int status;
            pid_t curr_pid = wait(&status);

            if(WIFEXITED(status)) {
                int processMultiplications = WEXITSTATUS(status);
                printf("Process %d has made %d multiplications\n", curr_pid, processMultiplications);
            }
        }
    }





//    FILE *file = fopen("a.txt", "r+");
//    fwrite("we", 2, sizeof(char), file);
//    fclose(file);
//
//    file = fopen("a.txt", "r+");
//    fseek(file, 1, SEEK_SET);
//    fwrite("2003", 4, sizeof(char), file);
//    fclose(file);


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

    return multiplications;
}