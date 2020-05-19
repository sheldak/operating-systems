#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <pthread.h>

#define SIZE 128


int w, h;
int **matrix;
int histogram[256];
int **threadHistogram;

struct threadSpecs {
    int threadsNum;
    int threadID;
} typedef threadSpecs;

void terminate() {
    exit(0);
}

void handleSEGVSignal(int signum) {
    printf("Segmentation fault\n");

    // terminating process after segmentation fault
    exit(0);
}

void handleINTSignal(int signum) {
    // terminating process and all workers
    exit(0);
}

void getImageMatrix(char *imagePath) {
    // opening file
    int fileDesc = open(imagePath, O_RDONLY);

    // buffer for current character
    char *currChar = calloc(1, sizeof(char));

    // first two lines are not important for building matrix
    int currLine = 0;
    while(currLine < 2) {
        read(fileDesc, currChar, 1);
        if(strcmp(currChar, "\n") == 0)
            currLine++;
    }

    // used in strtol function
    char *rest;

    // w - number of matrix's columns; h - number of matrix's rows
    w = 0;
    h = 0;
    int now_h = 0;

    // getting matrix size, third line of the file
    read(fileDesc, currChar, 1);
    while(strcmp(currChar, "\n") != 0) {
        if(strcmp(currChar, " ") == 0)
            now_h = 1;
        else {
            int currDigit = (int) strtol(currChar, &rest, 10);

            if(strcmp(rest, "") == 0) {
                if(now_h == 0) {
                    w *= 10;
                    w += currDigit;
                }
                else {
                    h *= 10;
                    h += currDigit;
                }
            }
        }
        read(fileDesc, currChar, 1);
    }

    // creating matrix
    matrix = calloc(h, sizeof(int*));
    for(int i=0; i<h; i++)
        matrix[i] = calloc(w, sizeof(int*));

    // we are assuming that M=255 so there is no need to read forth line
    currLine = 3;
    while(currLine < 4) {
        read(fileDesc, currChar, 1);
        if(strcmp(currChar, "\n") == 0)
            currLine++;
    }

    // loading matrix from file
    int curr_w = 0, curr_h = 0;
    int currDigit = -1;

    read(fileDesc, currChar, 1);
    while(curr_h < h) {
        if(strcmp(currChar, " ") == 0 || strcmp(currChar, "\n") == 0) {
            if(currDigit != -1) {
                curr_w++;
                if(curr_w >= w) {
                    curr_h++;
                    curr_w = 0;
                }

                // to avoid errors in case of several spaces
                currDigit = -1;
            }
        }
        else {
            currDigit = (int) strtol(currChar, &rest, 10);

            if(strcmp(rest, "") == 0) {
                matrix[curr_h][curr_w] *= 10;
                matrix[curr_h][curr_w] += currDigit;
            }
        }

        // checking case of EOF
        if (read(fileDesc, currChar, 1) == 0)
            currChar = "\n";
    }

    close(fileDesc);
}

long getTimeDifference(struct timespec startTime, struct timespec endTime) {
    long time = endTime.tv_sec - startTime.tv_sec;
    time = time * 1000000 + (endTime.tv_nsec - startTime.tv_nsec) / 1000;

    return time;
}

void *countShadesSign(void *arg) {
    threadSpecs *specs = (threadSpecs*) arg;

    // getting range of shades which this thread is counting
    int minShade = (256 / specs->threadsNum) * specs->threadID;
    int maxShade = (256 / specs->threadsNum) * (specs->threadID + 1) - 1;

    // for time measurement
    struct timespec startTime;
    struct timespec endTime;
    clock_gettime(CLOCK_REALTIME, &startTime);

    // counting shades
    for(int i=0; i<h; i++) {
        for(int j=0; j<w; j++) {
            int shade = matrix[i][j];
            if(shade >= minShade && shade <= maxShade)
                histogram[shade]++;
        }
    }

    // getting time of counting
    clock_gettime(CLOCK_REALTIME, &endTime);
    long countingTime = getTimeDifference(startTime, endTime);

    return (void*) countingTime;
}

int ceilDiv(int x, int y) {
    int res = x / y;
    if(res * y == x)
        return res;
    else
        return res+1;
}

void *countShadesBlock(void *arg) {
    threadSpecs *specs = (threadSpecs*) arg;

    // getting range of second matrix's coordinate
    int minW = ceilDiv(w, specs->threadsNum) * specs->threadID;
    int maxW = ceilDiv(w, specs->threadsNum) * (specs->threadID + 1) - 1;

    // for time measurement
    struct timespec startTime;
    struct timespec endTime;
    clock_gettime(CLOCK_REALTIME, &startTime);

    // counting shades
    for(int i=0; i<h; i++) {
        for(int j=minW; j<=maxW; j++) {
            int shade = matrix[i][j];
            threadHistogram[specs->threadID][shade]++;
        }
    }

    // getting time of counting
    clock_gettime(CLOCK_REALTIME, &endTime);
    long countingTime = getTimeDifference(startTime, endTime);

    return (void*) countingTime;
}

void *countShadesInterleaved(void *arg) {
    threadSpecs *specs = (threadSpecs*) arg;

    // getting range of shades which this thread is counting
    int startW = specs->threadID - 1;
    int stepW = specs->threadsNum;

    // for time measurement
    struct timespec startTime;
    struct timespec endTime;
    clock_gettime(CLOCK_REALTIME, &startTime);

    // counting shades
    for(int i=0; i<h; i++) {
        for(int j=startW; j<w; j += stepW) {
            int shade = matrix[i][j];
            threadHistogram[specs->threadID][shade]++;
        }
    }

    // getting time of counting
    clock_gettime(CLOCK_REALTIME, &endTime);
    long countingTime = getTimeDifference(startTime, endTime);

    return (void*) countingTime;
}

void saveHistogram(char *histogramPath) {
    // opening file
    int fileDesc = open(histogramPath, O_WRONLY);

    // shades will be written line by line
    char *line = calloc(SIZE, sizeof(char));

    // writing lines
    for(int i=0; i<256; i++) {
        sprintf(line, "%d %d\n", i, histogram[i]);
        write(fileDesc, line, SIZE);

        memset(line, 0, SIZE);
    }

    close(fileDesc);
}

int main(int argc, char **argv) {
    if (setvbuf(stdout, NULL, _IONBF, 0) != 0) {
        printf("Error: buffering mode could not be changed!\n");
        exit(1);
    }

    if(argc < 5) perror("Too few arguments");

    if(atexit(terminate) != 0) perror("atexit function error");

    // to stop program properly in case of segmentation fault
    if (signal(SIGSEGV, handleSEGVSignal) == SIG_ERR) perror("Signal function error when setting SIGSEGV handler");

    // to stop program properly in case of SIGINT signal
    if (signal(SIGINT, handleINTSignal) == SIG_ERR) perror("signal function error when setting SIGINT handler");

    // first argument - number of threads
    char *rest;
    int threads = (int) strtol(argv[1], &rest, 10);

    // second argument - type of job division
    char *divisionType = argv[2];

    // third argument - file with the image
    char *imagePath = argv[3];

    // forth argument - file with result histogram
    char *histogramPath = argv[4];

    // getting image matrix from file and saving it to the global variable
    getImageMatrix(imagePath);

    // initializing histogram array
    for(int i=0; i<256; i++)
        histogram[i] = 0;

    if(strcmp(divisionType, "sign") != 0) {
        threadHistogram = calloc(threads, sizeof(int*));
        for(int i=0; i<threads; i++)
            threadHistogram[i] = calloc(256, sizeof(int*));
    }

    // array with threads' IDs
    pthread_t *threadsIDs = calloc(threads, sizeof(pthread_t));

    // for time measurement
    struct timespec startTime;
    struct timespec endTime;
    clock_gettime(CLOCK_REALTIME, &startTime);

    // making threads
    for(int i=0; i<threads; i++) {
        threadSpecs *specs = malloc(sizeof(threadSpecs));
        specs->threadsNum = threads;
        specs->threadID = i;

        if(strcmp(divisionType, "sign") == 0) {
            if (pthread_create(&threadsIDs[i], NULL, countShadesSign, (void*) specs) != 0)
                perror("Cannot create thread");
        }
        else if(strcmp(divisionType, "block") == 0) {
            if (pthread_create(&threadsIDs[i], NULL, countShadesBlock, (void*) specs) != 0)
                perror("Cannot create thread");
        }
        else {
            if (pthread_create(&threadsIDs[i], NULL, countShadesInterleaved, (void*) specs) != 0)
                perror("Cannot create thread");
        }
    }

    // getting times of threads working
    for(int i=0; i<threads; i++) {
        long time;
        if (pthread_join(threadsIDs[i], (void *) &time) != 0) perror("Cannot get time from thread");

        printf("Thread %d. Time: %ld\n", i, time);
    }

    // merging histograms in case of block or interleaved type of task division
    if(strcmp(divisionType, "sign") != 0) {
        for(int i=0; i<threads; i++) {
            for(int j=0; j<256; j++)
                histogram[j] += threadHistogram[i][j];
        }
    }

    // getting time of counting all shades
    clock_gettime(CLOCK_REALTIME, &endTime);
    long time = getTimeDifference(startTime, endTime);

    // saving histogram to the file
    saveHistogram(histogramPath);

    // printing time
    printf("All time: %ld\n", time);

    // freeing memory
    free(threadsIDs);
    for(int i=0; i<h; i++)
        free(matrix[i]);
    free(matrix);

    return 0;
}
