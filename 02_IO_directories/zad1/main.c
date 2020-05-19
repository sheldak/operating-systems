#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/times.h>
#include <zconf.h>

#define SIZE 150

void generate(char *filename, char *recordsNum, char *recordSize) {
    char *command = calloc(SIZE, sizeof(char));

    strcpy(command, "head -c 100000000000 /dev/urandom | tr -dc 'a-z'");
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

void copySys(char *filename1, char *filename2, int records, int bufferSize) {
    int fileDesc1 = open(filename1, O_RDONLY);
    if(fileDesc1 < 0)
        perror("problem with opening first file when copying (sys)");

    creat(filename2, 0666);
    int fileDesc2 = open(filename2, O_WRONLY);
    if(fileDesc2 < 0)
        perror("problem with opening second file when copying (sys)");

    char *buffer = calloc(bufferSize+1, sizeof(char));

    for(int i=0; i < records; i++) {
        lseek(fileDesc1, (bufferSize+1) * i, SEEK_SET);
        if (read(fileDesc1, buffer, bufferSize+1) < 0)
            perror("failed in getting record (sys)");

        lseek(fileDesc2, (bufferSize + 1) * i, SEEK_SET);
        write(fileDesc2, buffer, bufferSize+1);
    }
}

void copyLib(char *filename1, char *filename2, int records, int bufferSize) {
    FILE *file1 = fopen(filename1, "r");
    if(!file1)
        perror("problem with opening first file when copying (lib)");

    FILE *file2 = fopen(filename2, "w");
    if(!file2)
        perror("problem with opening second file when copying (lib)");

    char *buffer = calloc(bufferSize+1, sizeof(char));

    for(int i=0; i < records; i++) {
        fseek(file1, (bufferSize+1) * i, SEEK_SET);
        if (fread(buffer, sizeof(char), bufferSize+1, file1) < 0)
            perror("failed in getting record (lib)");

        fseek(file2, (bufferSize + 1) * i, SEEK_SET);
        fwrite(buffer, sizeof(char), bufferSize+1, file2);
    }

    fclose(file1);
    fclose(file2);
}

void writeHeader(FILE *file, char *recordsNum, char *recordLen) {
    fprintf(file, "--- %s records of length %s ---\n\n", recordsNum, recordLen);
}

double timeDifference(clock_t start, clock_t end) {
    return (double) (end - start) / (double) sysconf(_SC_CLK_TCK);
}

void writeResults(FILE *file, double realT, double userT, double systemT) {
    fprintf(file, "REAL       USER       SYSTEM\n");
    fprintf(file, "%f  %f  %f\n\n", realT, userT, systemT);
}

int main(int argc, char **argv) {
    if(argc < 5)
        perror("too few arguments");

    char *commands[5] = {"generate", "sort", "copy", "sys", "lib"};

    // time
    struct tms *tmsTime[2];
    for(int i=0; i<2; i++)
        tmsTime[i] = calloc(1, sizeof(struct tms *));

    clock_t realTime[2];

    // result file
    FILE *resultFile = fopen("wyniki.txt", "a");
    if(!resultFile)
        perror("cannot open result file");


    for(int i=1; i<argc; i++) {
        realTime[0] = times(tmsTime[0]);
        if(strcmp(argv[i], commands[0]) == 0) {         // generate
            if(i+3 >= argc)
                perror("too few arguments after generate call");

            // extracting parameters
            char *filename = argv[i+1];
            char *recordsNum = argv[i+2];
            char *recordLen = argv[i+3];

            // generating random text to file
            generate(filename, recordsNum, recordLen);

            // writing header to result file
            writeHeader(resultFile, recordsNum, recordLen);

            i += 3;
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
                perror("invalid number of records to sort");

            int recordLen;
            recordLen = (int) strtol(argv[i+3], &endPtr, 10);
            if(recordLen <= 0)
                perror("invalid size of records to sort");

            // sorting
            if(strcmp(argv[i+4], commands[3]) == 0) {     // sys
                fprintf(resultFile, "SORTING USING SYSTEM FUNCTIONS\n\n");
                sortSys(filename, recordsNum, recordLen);
            }
            else if(strcmp(argv[i+4], commands[4]) == 0) {  // lib
                fprintf(resultFile, "SORTING USING LIBRARY FUNCTIONS\n\n");
                sortLib(filename, recordsNum, recordLen);
            }
            else
                perror("invalid type of sort");

            // writing time to result file
            realTime[1] = times(tmsTime[1]);
            writeResults(resultFile, timeDifference(realTime[0], realTime[1]),
                         timeDifference(tmsTime[0]->tms_utime, tmsTime[1]->tms_utime),
                         timeDifference(tmsTime[0]->tms_stime, tmsTime[1]->tms_stime));
            fprintf(resultFile, "\n");

            i += 4;
        }
        else if(strcmp(argv[i], commands[2]) == 0) {    // copy
            if(i+5 >= argc)
                perror("too few arguments after copy call");

            // extracting parameters
            char *filename1 = argv[i+1];
            char *filename2 = argv[i+2];

            int recordsNum;
            char *endPtr;
            recordsNum = (int) strtol(argv[i+3], &endPtr, 10);
            if(recordsNum <= 0)
                perror("invalid number of records to copy");

            int recordLen;
            recordLen = (int) strtol(argv[i+4], &endPtr, 10);
            if(recordLen <= 0)
                perror("invalid size of records to copy");

            // copying
            if(strcmp(argv[i+5], commands[3]) == 0) {      // sys
                fprintf(resultFile, "COPYING USING SYSTEM FUNCTIONS\n\n");
                copySys(filename1, filename2, recordsNum, recordLen);
            }
            else if(strcmp(argv[i+5], commands[4]) == 0) { // lib
                fprintf(resultFile, "COPYING USING LIBRARY FUNCTIONS\n\n");
                copyLib(filename1, filename2, recordsNum, recordLen);
            }
            else
                perror("invalid type of copying");

            // writing time to result file
            realTime[1] = times(tmsTime[1]);
            writeResults(resultFile, timeDifference(realTime[0], realTime[1]),
                         timeDifference(tmsTime[0]->tms_utime, tmsTime[1]->tms_utime),
                         timeDifference(tmsTime[0]->tms_stime, tmsTime[1]->tms_stime));
            fprintf(resultFile, "\n");

            i += 5;
        }
    }
    fprintf(resultFile, "\n");
}

