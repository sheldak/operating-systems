#include <stdio.h>
#include <stdlib.h>

#define SIZE 256
#define MATRIX_MIN -100
#define MATRIX_MAX 100

struct matrix {
    int rows;
    int columns;
    int **table;
};

int getNumSize(int num) {
    int size = 1;
    while(num / 10 > 0){
        size++;
        num /= 10;
    }

    return size;
}

void writeMatrixToFile(char *filePath, struct matrix m) {
    FILE *file = fopen(filePath, "w");

    char *line = calloc(SIZE, sizeof(char));
    int size = 2 + getNumSize(m.rows) + getNumSize(m.columns);
    sprintf(line, "%d %d\n", m.rows, m.columns);
    fwrite(line, size, sizeof(char), file);

    char *num = calloc(SIZE, sizeof(char));
    for(int i=0; i<m.rows; i++) {
        memset(line, 0, SIZE);
        for(int j=0; j<m.columns; j++) {
            sprintf(num, "%d ", m.table[i][j]);
            strcat(line, num);
            size += getNumSize(m.table[i][j])+1;
        }
        line[size-1] = '\n';
        fwrite(line, size, sizeof(char), file);
    }
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
        mA.table = calloc(mA.rows, sizeof(int));
        for(int row=0; row < mA.rows; row++) {
            mA.table[row] = calloc(mA.columns, sizeof(int));
            for(int column=0; column < mA.columns; column++)
                mA.table[row][column] = rand() % (MATRIX_MAX - MATRIX_MIN + 1) + MATRIX_MIN;
        }

        mB.table = calloc(mB.rows, sizeof(int));
        for(int row=0; row < mB.rows; row++) {
            mB.table[row] = calloc(mB.columns, sizeof(int));
            for(int column=0; column < mB.columns; column++)
                mB.table[row][column] = rand() % (MATRIX_MAX - MATRIX_MIN + 1) + MATRIX_MIN;
        }



    }
}