#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/times.h>
#include <zconf.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>

#define SIZE 256

struct options {
    int mTime;
    int mTimeSign;
    int aTime;
    int aTimeSign;
    int maxDepth;
};

struct options initializeOptions() {
    struct options o;
    o.mTime = -1;
    o.mTimeSign = -2;
    o.aTime = -1;
    o.aTimeSign = -2;
    o.maxDepth = INT_MAX;

    return o;
}

char *getFileType(unsigned char type) {
    switch(type) {
        case DT_REG:  return "file";
        case DT_DIR:  return "dir";
        case DT_CHR:  return "char dev";
        case DT_BLK:  return "block dev";
        case DT_FIFO: return "fifo";
        case DT_LNK:  return "slink";
        case DT_SOCK: return "sock";
        default:      return "";
    }
}

char *timeSecToDate(time_t time) {
    char *date = calloc(SIZE, sizeof(char));

    struct tm *tmTime = localtime(&time);
    strftime(date, SIZE, "%Y/%m/%d %H:%M:%S", tmTime);

    return date;
}

void printFileInfo(char *absolutePath, struct stat *fileStat, struct dirent *fileEntry) {
    printf("absolute path:     %s\n", absolutePath);
    printf("hardlinks:         %lu\n", fileStat->st_nlink);
    printf("type:              %s\n", getFileType(fileEntry->d_type));
    printf("size:              %lu\n", fileStat->st_size);
    printf("last access:       %s\n", timeSecToDate(fileStat->st_atim.tv_sec));
    printf("last modification: %s\n", timeSecToDate(fileStat->st_mtim.tv_sec));
    printf("\n");
}

char *getAbsoluteDirPath(char *dirPath) {
    if (strlen(dirPath) < 1)
        perror("invalid path");

    char *absoluteDirPath = calloc(SIZE, sizeof(char));

    if (dirPath[0] == '/') {
        strcpy(absoluteDirPath, dirPath);
        strcat(absoluteDirPath, "/");
    }
    else {
        getcwd(absoluteDirPath, SIZE);
        strcat(absoluteDirPath, "/");
        if (dirPath[0] != '.')
            strcat(absoluteDirPath, dirPath);
    }

    return absoluteDirPath;
}
// TODO check if it works
int checkFile(char *fileName, struct options findOptions, struct stat *fileStat) {
    if (strcmp(fileName, ".") == 0 || strcmp(fileName, "..") == 0)
        return -1;

    time_t currTimeDays = time(NULL) / (3600 * 24);
    time_t mTimeDays = fileStat->st_mtim.tv_sec / (3600 * 24);
    time_t aTimeDays = fileStat->st_atim.tv_sec / (3600 * 24);

    if ((findOptions.mTimeSign == 1 && mTimeDays + findOptions.mTime > currTimeDays) ||
        (findOptions.mTimeSign == -1 && mTimeDays + abs(findOptions.mTime) < currTimeDays) ||
        (findOptions.mTimeSign == 0 && mTimeDays + findOptions.mTime != currTimeDays))
        return -1;

    if ((findOptions.aTimeSign == 1 && aTimeDays + findOptions.aTime > currTimeDays) ||
        (findOptions.aTimeSign == -1 && aTimeDays + abs(findOptions.aTime) < currTimeDays) ||
        (findOptions.aTimeSign == 0 && aTimeDays + findOptions.aTime != currTimeDays))
        return -1;

    return 0;
}

void find(char *dirPath, struct options findOptions, int depth) {
    DIR *directory = opendir(dirPath);
    if (!directory)
        perror("invalid directory");

    struct dirent *currEntry = readdir(directory);
    struct stat *statBuffer = calloc(SIZE, sizeof(char));

    while(currEntry) {
        char *fileName = currEntry->d_name;

        char *absolutePath = calloc(SIZE, sizeof(char));
        strcpy(absolutePath, dirPath);
        strcat(absolutePath, fileName);

        int statRes = stat(absolutePath, statBuffer);
        if(statRes == -1)
            perror("error in stat");

        if(checkFile(fileName, findOptions, statBuffer) == 0) {
            printFileInfo(absolutePath, statBuffer, currEntry);

            if (strcmp(getFileType(currEntry->d_type), "dir") == 0 && findOptions.maxDepth > depth) {
                char *newPath = calloc(SIZE, sizeof(char));
                strcpy(newPath, absolutePath);
                strcat(newPath, "/");
                find(newPath, findOptions, depth+1);
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

    char *commands[3] = {"-mtime", "-atime", "-maxdepth"};

    struct options findOptions = initializeOptions();

    for (int i=2; i<argc; i++) {
        if (strcmp(argv[i], commands[0]) == 0) {
            if (i+1 >= argc)
                perror("missed argument for -mtime");

            if(argv[i+1][0] == '-')
                findOptions.mTimeSign = -1;
            else if(argv[i+1][0] == '-')
                findOptions.mTimeSign = 1;
            else
                findOptions.mTimeSign = 0;

            char *rest;
            findOptions.mTime = (int) strtol(argv[i+1], &rest, 10);

            i += 1;
        } else if (strcmp(argv[i], commands[1]) == 0) {
            if (i+1 >= argc)
                perror("missed argument for -mtime");

            if(argv[i+1][0] == '-')
                findOptions.aTimeSign = -1;
            else if(argv[i+1][0] == '-')
                findOptions.aTimeSign = 1;
            else
                findOptions.aTimeSign = 0;

            char *rest;
            findOptions.aTime = (int) strtol(argv[i+1], &rest, 10);

            i += 1;
        } else if (strcmp(argv[i], commands[2]) == 0) {
            if (i+1 >= argc)
                perror("missed argument for -mtime");

            char *rest;
            findOptions.maxDepth = (int) strtol(argv[i+1], &rest, 10);

            i += 1;
        } else {
            perror("invalid arguments");
        }
    }

//    printf("%d\n", findOptions.mTime);
//    printf("%d\n", findOptions.mTimeSign);
//    printf("%d\n", findOptions.aTime);
//    printf("%d\n", findOptions.aTimeSign);
//    printf("%d\n", findOptions.maxDepth);

    if (findOptions.maxDepth >= 1) {
        char *dirPath = getAbsoluteDirPath(argv[1]);
        find(dirPath, findOptions, 1);
    }

    return 0;
}