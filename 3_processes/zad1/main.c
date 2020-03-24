#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <ftw.h>
#include <sys/wait.h>

#define SIZE 256

int checkIfDir(unsigned char type) {
    if (type == DT_DIR)
        return 0;
    else
        return -1;
}

char *getAbsolutePath(char *path) {
    if (strlen(path) < 1)
        perror("invalid path");

    char *absoluteDirPath = calloc(SIZE, sizeof(char));

    if (path[0] == '/') {
        strcpy(absoluteDirPath, path);
    }
    else {
        getcwd(absoluteDirPath, SIZE);
        if (path[0] != '.') {
            strcat(absoluteDirPath, "/");
            strcat(absoluteDirPath, path);
        }
    }

    return absoluteDirPath;
}

int checkName(char *fileName) {
    if (strcmp(fileName, ".") == 0 || strcmp(fileName, "..") == 0)
        return -1;

    return 0;
}


char *getDirectoryFromPath(const char *path) {
    int lastSlash = -1;
    for(int i=0; i<strlen(path); i++) {
        if(path[i] == '/')
            lastSlash = i;
    }

    char *directory = calloc(SIZE, sizeof(char));

    for(int i=0; i<lastSlash; i++)
        directory[i] = path[i];

    return directory;
}

char *getNameFromPath(const char *path) {
    int lastSlash = -1;
    for(int i=0; i<strlen(path); i++) {
        if(path[i] == '/')
            lastSlash = i;
    }

    char *name = calloc(SIZE, sizeof(char));

    for(int i=lastSlash+1; i<strlen(path); i++)
        name[i - lastSlash - 1] = path[i];

    return name;
}

int checkIfDirFromPath(char *dirPath, char *name) {
    if(strcmp(dirPath, "") == 0)
        dirPath = "/";

    DIR *directory = opendir(dirPath);
    if (!directory)
        perror("invalid directory");

    struct dirent *currEntry = readdir(directory);

    while(strcmp(currEntry->d_name, name) != 0)
        currEntry = readdir(directory);

    return checkIfDir(currEntry->d_type);
}

void list(char *absPath, char *relPath) {
    DIR *directory = opendir(absPath);
    if (!directory)
        perror("invalid directory");

    struct dirent *currEntry = readdir(directory);
    struct stat *statBuffer = calloc(SIZE, sizeof(struct stat));

    while(currEntry) {
        char *dirName = currEntry->d_name;
        if (checkIfDir(currEntry->d_type) == 0 && checkName(dirName) == 0) {
            char *currAbsPath = calloc(SIZE, sizeof(char));
            char *currRelPath = calloc(SIZE, sizeof(char));
            strcpy(currAbsPath, absPath);
            strcat(currAbsPath, dirName);

            strcpy(currRelPath, relPath);
            strcat(currRelPath, dirName);

            pid_t pid = fork();

            if(pid < 0)
                perror("problem with forking");
            else if(pid == 0) {
                char *command = calloc(SIZE, sizeof(char));
                strcpy(command, "ls -l ");
                strcat(command, currAbsPath);

                printf("Relative path: %s\n", currRelPath);
                printf("PID: %d\n", getpid());
                system(command);
                printf("\n");

                strcat(currAbsPath, "/");
                strcat(currRelPath, "/");

                list(currAbsPath, currRelPath);

                exit(0);
            }
            else{
                wait(&pid);
            }
        }
        currEntry = readdir(directory);
    }

    free(statBuffer);
    closedir(directory);
}

int main(int argc, char **argv) {
    if(argc < 2)
        perror("too few arguments");

    char *absPath = getAbsolutePath(argv[1]);
    char *relPath = calloc(SIZE, sizeof(char));
    strcpy(relPath, "");


    if(checkIfDirFromPath(getDirectoryFromPath(absPath), getNameFromPath(argv[1])) == 0) {
        printf("\n");
        strcat(absPath, "/");
        list(absPath, relPath);
    }

    return 0;
}