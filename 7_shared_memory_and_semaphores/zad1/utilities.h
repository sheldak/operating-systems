#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <mqueue.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/shm.h>

#define SEMAPHORE_ID 'A'
#define MEMORY_ID 'A'

struct sharedVariables {
    int arraySize;
    int *array;
    int firstToPrepare;
    int lastToPrepare;
    int firstToSend;
    int lastToSend;
} typedef sharedVariables;