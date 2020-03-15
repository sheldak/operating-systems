#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <zconf.h>

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

char *getRecordSys(int fileDesc, int recordInx, int recordLen) {
    char *record = calloc(recordLen, sizeof(char));
    lseek(fileDesc, (recordLen+1)*recordInx, SEEK_SET);
    if (read(fileDesc, record, recordLen) < 0)
        perror("failed in getting record (sys)");

    return record;
}

void writeRecordSys(int fileDesc, char *record, int recordInx, int recordLen) {
    lseek(fileDesc, (recordLen + 1) * recordInx, SEEK_SET);
    write(fileDesc, record, recordLen);
}

int compareRecordsSys(int fileDesc, int firstInx, int secondInx, int recordLen) {
    char *firstRecord = getRecordSys(fileDesc, firstInx, recordLen);
    char *secondRecord = getRecordSys(fileDesc, secondInx, recordLen);

    int i=0;
    while(i < recordLen && firstRecord[i] == secondRecord[i])
        i++;

    if(i == recordLen) // same
        return 0;
    else if(firstRecord[i] > secondRecord[i]) // first is further lexicographically 
        return 1;
    else
        return -1;
}


void quickSortSys(int fileDesc, int first, int last, int recordLen) {
    int i, j, pivot;
    char *tmp;

    if(first<last) {
        pivot=first;
        i=first;
        j=last;

        while(i<j) {
            while(i < last && compareRecordsSys(fileDesc, i, pivot, recordLen) <= 0)
                i++;
            while(compareRecordsSys(fileDesc, j, pivot, recordLen) > 0)
                j--;
            if(i<j) {
                tmp = getRecordSys(fileDesc, i, recordLen);
                writeRecordSys(fileDesc, getRecordSys(fileDesc, j, recordLen), i, recordLen);
                writeRecordSys(fileDesc, tmp, j, recordLen);
            }
        }

        tmp = getRecordSys(fileDesc, pivot, recordLen);
        writeRecordSys(fileDesc, getRecordSys(fileDesc, j, recordLen), pivot, recordLen);
        writeRecordSys(fileDesc, tmp, j, recordLen);

        quickSortSys(fileDesc, first, j-1, recordLen);
        quickSortSys(fileDesc, j+1, last, recordLen);
    }
}

void sortSys(char *filename, int recordNum, int recordLen) {
    int fileDesc = open(filename, O_RDWR);
    if(fileDesc < 0)
        perror("problem with opening file by sys to sort");

    quickSortSys(fileDesc, 0, recordNum - 1, recordLen);
}

char *getRecordLib(FILE *file, int recordInx, int recordLen) {
    char *record = calloc(recordLen, sizeof(char));
    fseek(file, (recordLen+1)*recordInx, SEEK_SET);
    if (fread(record, sizeof(char), recordLen, file) < 0)
        perror("failed in getting record (lib)");

    return record;
}

void writeRecordLib(FILE *file, char *record, int recordInx, int recordLen) {
    fseek(file, (recordLen + 1) * recordInx, SEEK_SET);
    fwrite(record, sizeof(char), recordLen, file);
}

int compareRecordsLib(FILE *file, int firstInx, int secondInx, int recordLen) {
    char *firstRecord = getRecordLib(file, firstInx, recordLen);
    char *secondRecord = getRecordLib(file, secondInx, recordLen);

    int i=0;
    while(i < recordLen && firstRecord[i] == secondRecord[i])
        i++;

    if(i == recordLen) // same
        return 0;
    else if(firstRecord[i] > secondRecord[i]) // first is further lexicographically
        return 1;
    else
        return -1;
}


void quickSortLib(FILE *file, int first, int last, int recordLen) {
    int i, j, pivot;
    char *tmp;

    if(first<last) {
        pivot=first;
        i=first;
        j=last;

        while(i<j) {
            while(i < last && compareRecordsLib(file, i, pivot, recordLen) <= 0)
                i++;
            while(compareRecordsLib(file, j, pivot, recordLen) > 0)
                j--;
            if(i<j) {
                tmp = getRecordLib(file, i, recordLen);
                writeRecordLib(file, getRecordLib(file, j, recordLen), i, recordLen);
                writeRecordLib(file, tmp, j, recordLen);
            }
        }

        tmp = getRecordLib(file, pivot, recordLen);
        writeRecordLib(file, getRecordLib(file, j, recordLen), pivot, recordLen);
        writeRecordLib(file, tmp, j, recordLen);

        quickSortLib(file, first, j-1, recordLen);
        quickSortLib(file, j+1, last, recordLen);
    }
}

void sortLib(char *filename, int recordNum, int recordLen) {
    FILE *file = fopen(filename, "r+");
    if(!file)
        perror("problem with opening file by lib to sort");

    quickSortLib(file, 0, recordNum - 1, recordLen);

    fclose(file);
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
            char *recordLen = argv[i+3];

            // generating random text to file
            generate(filename, recordsNum, recordLen);
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

            int recordLen;
            recordLen = (int) strtol(argv[i+3], &endPtr, 10);
            if(recordLen <= 0)
                perror("invalid size of records to generate");

            // sorting
            if(strcmp(argv[i+4], commands[3]) == 0)       // sys
                sortSys(filename, recordsNum, recordLen);
            else if(strcmp(argv[i+4], commands[4]) == 0)  // lib
                sortLib(filename, recordsNum, recordLen);
            else
                perror("invalid type of sort");

        }
        else if(strcmp(argv[i], commands[2]) == 0) {    // copy
            if(i+5 >= argc)
                perror("too few arguments after copy call");
        }
    }
}

