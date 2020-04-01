#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <sys/wait.h>

#define SIZE 256

int listDir = 0;

void handlerINT(int signum) {
    printf("Odebrano sygnał SIGINT");
    exit(0);
}

void handlerTSTP(int signum) {
    if(listDir == 0) {
        listDir = 1;
        printf("Oczekuję na CTRL+Z - kontynuacja \n");
        printf("Oczekuję na CTRL+C - zakończenie programu \n");
    }
    else
        listDir = 0;
}

void list() {
    char *path = calloc(SIZE, sizeof(char));
    getcwd(path, SIZE);

    DIR *directory = opendir(path);
    if (!directory)
        perror("cannot get directory");

    struct dirent *currEntry = readdir(directory);

    while(currEntry) {
        if(strcmp(currEntry->d_name, ".") != 0 && strcmp(currEntry->d_name, "..") != 0)
            printf("%s\n", currEntry->d_name);

        currEntry = readdir(directory);
    }

    closedir(directory);
    free(currEntry);

    printf("\n");
}

int main(int argc, char **argv) {
    // SIGINT handled by signal function
    signal(SIGINT, handlerINT);

    // SIGTSTP handled by sigaction function
    struct sigaction action;
    action.sa_handler = handlerTSTP;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    sigaction(SIGTSTP, &action, NULL);

    while(1) {
        if(listDir == 0) {
            list();
            sleep(1);
        }
        else
            pause();
    }
    return 0;
}
