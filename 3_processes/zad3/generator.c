#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SIZE 256
#define MATRIX_MIN 0
#define MATRIX_MAX 100

struct matrix {
    int rows;
    int columns;
    int **table;
};

void freeMatrix(struct matrix m) {
    for(int row=0; row<m.rows; row++)
        free(m.table[row]);

    free(m.table);
}

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

void writeMatrixToFile(char *filePath, struct matrix m, int isB) {
    FILE *file = fopen(filePath, "w");

    char *firstLine = calloc(SIZE, sizeof(char));
    int size;

    if(isB == 0) {
        size = 5 + getNumSize(m.rows) + getNumSize(m.columns);
        sprintf(firstLine, "%d %d 00\n", m.rows, m.columns);
    }
    else {
        size = 2 + getNumSize(m.rows) + getNumSize(m.columns);
        sprintf(firstLine, "%d %d\n", m.rows, m.columns);
    }

    fwrite(firstLine, size, sizeof(char), file);

    char *num = calloc(SIZE, sizeof(char));
    for(int i=0; i<m.rows; i++) {
        char *line = calloc(SIZE, sizeof(char));
        strcpy(line, "");
        size = 0;
        for(int j=0; j<m.columns; j++) {
            sprintf(num, "%d ", m.table[i][j]);
            strcat(line, num);
            size += getNumSize(m.table[i][j])+1;
        }
        line[size-1] = '\n';
        fwrite(line, size, sizeof(char), file);
        free(line);
    }

    fclose(file);
}

void writeEmptyMatrixToFile(char *filePath) {
    char *line = "0 0\n";
    FILE *file = fopen(filePath, "w");
    fwrite(line, 4, sizeof(char), file);
    fclose(file);
}

void generateMatrices(int n, int min, int max, char **filePathsA, char **filePathsB, char **filePathsC) {
    for(int i=0; i<n; i++) {
        struct matrix mA;
        struct matrix mB;

        // getting random size of matrices
        mA.rows = rand() % (max - min + 1) + min;
        mA.columns = rand() % (max - min + 1) + min;

        mB.rows = mA.columns;
        mB.columns = rand() % (max - min + 1) + min;

        // getting random content of matrices
        mA.table = calloc(mA.rows, sizeof(int*));
        for(int row=0; row < mA.rows; row++) {
            mA.table[row] = calloc(mA.columns, sizeof(int));
            for(int column=0; column < mA.columns; column++)
                mA.table[row][column] = rand() % (MATRIX_MAX - MATRIX_MIN + 1) + MATRIX_MIN;
        }

        mB.table = calloc(mB.rows, sizeof(int*));
        for(int row=0; row < mB.rows; row++) {
            mB.table[row] = calloc(mB.columns, sizeof(int));
            for(int column=0; column < mB.columns; column++)
                mB.table[row][column] = rand() % (MATRIX_MAX - MATRIX_MIN + 1) + MATRIX_MIN;

        }

        writeMatrixToFile(filePathsA[i], mA, -1);
        writeMatrixToFile(filePathsB[i], mB, 0);

//        writeEmptyMatrixToFile(filePathsC[i]);

        freeMatrix(mA);
        freeMatrix(mB);
    }
}

int main(int argc, char **argv) {
    if(argc < 4)
        perror("too few arguments");

    srand(time(0));

    // first argument, number of pairs of matrices
    char *rest;
    int pairsNum = (int) strtol(argv[1], &rest ,10);

    if(strcmp(rest, "") != 0)
        perror("invalid first argument");

    rest = "";
    int minSize = (int) strtol(argv[2], &rest ,10);

    if(strcmp(rest, "") != 0)
        perror("invalid second argument");

    rest = "";
    int maxSize = (int) strtol(argv[3], &rest ,10);

    if(strcmp(rest, "") != 0)
        perror("invalid third argument");

    char **filePathsA = calloc(pairsNum, sizeof(char*));
    char **filePathsB = calloc(pairsNum, sizeof(char*));
    char **filePathsC = calloc(pairsNum, sizeof(char*));
    for(int i=0; i<pairsNum; i++) {
        filePathsA[i] = calloc(SIZE, sizeof(char));
        sprintf(filePathsA[i], "matrices/in%d.csv", i*2+1);

        filePathsB[i] = calloc(SIZE, sizeof(char));
        sprintf(filePathsB[i], "matrices/in%d.csv", i*2+2);

        filePathsC[i] = calloc(SIZE, sizeof(char));
        sprintf(filePathsC[i], "matrices/out%d.csv", i+1);
    }

    generateMatrices(pairsNum, minSize, maxSize, filePathsA, filePathsB, filePathsC);

    return 0;
}