#include "utilities.c"

void terminate() {
//    if(shmdt(memoryAddress) < 0) perror("Cannot detach memory");
//    else printf("Shared memory detached successfully from sender process\n");

    exit(0);
}

void handleSEGVSignal(int signum) {
    printf("Segmentation fault\n");
    // terminating process after segmentation fault
    exit(0);
}

void handleINTSignal(int signum) {
    // terminating process
    exit(0);
}

int main() {
    // detaching memory before process termination
    if(atexit(terminate) != 0) perror("atexit function error");

    // to stop program properly in case of segmentation fault
    if (signal(SIGSEGV, handleSEGVSignal) == SIG_ERR) perror("Signal function error when setting SIGSEGV handler");

    // to stop program properly in case of SIGINT signal
    if (signal(SIGINT, handleINTSignal) == SIG_ERR) perror("signal function error when setting SIGINT handler");

    while(1) {}

    return 0;
}


