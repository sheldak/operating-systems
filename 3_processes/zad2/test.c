#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define SIZE 256

const char *matricesDir = "matrices/";

struct matrix {
    int rows;
    int columns;
    int **table;
};

void getFirstLine(int fileDesc, int *rows, int *columns) {
    *rows = 0;
    *columns = 0;

    char *currChar = calloc(1, sizeof(char));
    char *rest = calloc(1, sizeof(char));
    int currDigit;

    int currVariable = 0;
    read(fileDesc, currChar, 1);
    while(strcmp(currChar, "\n") != 0) {
        if(strcmp(currChar, " ") == 0)
            currVariable++;
        else if (currVariable < 2){
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

struct matrix getMatrix(int fileDesc) {
    int rows, columns;
    getFirstLine(fileDesc, &rows, &columns);

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

int makeTest(struct matrix mA, struct matrix mB, struct matrix mC) {
    struct matrix mR;
    mR.rows = mA.rows;
    mR.columns = mB.columns;

    if(mA.columns != mB.rows)
        perror("invalid size of matrices, cannot multiply them");

    mR.table = calloc(mA.rows, sizeof(int*));
    for(int i=0; i < mA.rows; i++)
        mR.table[i] = calloc(mB.columns, sizeof(int));

    for(int i = 0; i < mR.rows; i++) {
        for(int j=0; j < mR.columns; j++) {
            int sum = 0;
            for(int k=0; k < mA.columns; k++)
                sum += mA.table[i][k] * mB.table[k][j];
            mR.table[i][j] = sum;
        }
    }

    if(mR.rows == mC.rows && mR.columns == mC.columns) {
        for(int row=0; row < mR.rows; row++) {
            for(int col=0; col < mR.columns; col++) {
                if (mR.table[row][col] != mC.table[row][col])
                    return -1;
            }
        }
        return 0;
    }
    else
        return -1;
}

int main(int argc, char **argv) {
    if(argc < 4)
        perror("too few arguments");

    // first argument, first file (matrix A)
    char *fileA = calloc(SIZE, sizeof(char));
    strcpy(fileA, matricesDir);
    strcat(fileA, argv[1]);

    // second argument, second file (matrix B)
    char *fileB = calloc(SIZE, sizeof(char));
    strcpy(fileB, matricesDir);
    strcat(fileB, argv[2]);

    // third argument, third file (output matrix C)
    char *fileC = calloc(SIZE, sizeof(char));
    strcpy(fileC, matricesDir);
    strcat(fileC, argv[3]);

    // getting matrices from files
    int fileADesc = open(fileA, O_RDONLY);
    struct matrix mA = getMatrix(fileADesc);

    int fileBDesc = open(fileB, O_RDONLY);
    struct matrix mB = getMatrix(fileBDesc);

    int fileCDesc = open(fileC, O_RDONLY);
    struct matrix mC = getMatrix(fileCDesc);

    // checking if multiplied properly
    int testResult = makeTest(mA, mB, mC);

    if(testResult == 0)
        printf("Matrix in file %s is correct\n", fileC);
    else
        printf("Matrix in file %s is incorrect\n", fileC);

    return 0;
}