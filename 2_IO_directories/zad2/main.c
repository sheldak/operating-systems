#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zconf.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#define __USE_XOPEN_EXTENDED 1
#include <ftw.h>


#define SIZE 256

struct options {
    int mTime;
    int mTimeSign;
    int aTime;
    int aTimeSign;
    int maxDepth;
};

struct options findOptions;

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

char *getFileTypeNtfw(int type) {
    switch(type) {
        case FTW_F: return "file";
        case FTW_D: return "dir";
        case FTW_SL: return "slink";
        default: return "";
    }
}

char *timeSecToDate(time_t time) {
    char *date = calloc(SIZE, sizeof(char));

    struct tm *tmTime = localtime(&time);
    strftime(date, SIZE, "%Y/%m/%d %H:%M:%S", tmTime);

    return date;
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

time_t timeAbs(time_t time) {
    if (time < 0)
        return -1*time;
    else
        return time;
}

int checkTimeOptions(const struct stat *fileStat) {
    time_t currTime = time(NULL);
    time_t mTime = fileStat->st_mtim.tv_sec;
    time_t aTime = fileStat->st_atim.tv_sec;

    time_t mTimeDiff = (currTime - timeAbs(mTime)) / (3600 * 24);
    time_t aTimeDiff = (currTime - timeAbs(aTime)) / (3600 * 24);

    if ((findOptions.mTimeSign == 1 && mTimeDiff < 1 + findOptions.mTime) ||
        (findOptions.mTimeSign == -1 && mTimeDiff > 1 + findOptions.mTime) ||
        (findOptions.mTimeSign == 0 && mTimeDiff != findOptions.mTime))
        return -1;

    if ((findOptions.aTimeSign == 1 && aTimeDiff < 1 + findOptions.aTime) ||
        (findOptions.aTimeSign == -1 && aTimeDiff > 1 + findOptions.aTime) ||
        (findOptions.aTimeSign == 0 && aTimeDiff != findOptions.aTime))
        return -1;

    return 0;
}

void printFileInfo(const char *absolutePath, const struct stat *fileStat, char *type) {
    printf("absolute path:     %s\n", absolutePath);
    printf("hardlinks:         %lu\n", fileStat->st_nlink);
    printf("type:              %s\n", type);
    printf("size:              %lu\n", fileStat->st_size);
    printf("last access:       %s\n", timeSecToDate(fileStat->st_atim.tv_sec));
    printf("last modification: %s\n", timeSecToDate(fileStat->st_mtim.tv_sec));
    printf("\n");
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

int printFileInfoNftw(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    if(checkName(getNameFromPath(fpath)) == 0 && checkTimeOptions(sb) == 0 && ftwbuf->level <= findOptions.maxDepth)
        printFileInfo(fpath, sb, getFileTypeNtfw(typeflag));

    return 0;
}

char *getFileTypeFromPath(char *dirPath, char *name) {
    if(strcmp(dirPath, "") == 0)
        dirPath = "/";

    DIR *directory = opendir(dirPath);
    if (!directory)
        perror("invalid directory");

    struct dirent *currEntry = readdir(directory);

    while(strcmp(currEntry->d_name, name) != 0)
        currEntry = readdir(directory);

    return getFileType(currEntry->d_type);
}

void find(char *dirPath, int depth) {
    DIR *directory = opendir(dirPath);
    if (!directory)
        perror("invalid directory");

    struct dirent *currEntry = readdir(directory);
    struct stat *statBuffer = calloc(SIZE, sizeof(struct stat));

    while(currEntry) {
        char *fileName = currEntry->d_name;

        char *absolutePath = calloc(SIZE, sizeof(char));
        strcpy(absolutePath, dirPath);
        strcat(absolutePath, fileName);

        int statRes = stat(absolutePath, statBuffer);
        if(statRes == -1 && errno != ENOENT)
            perror("error in stat");

        if(checkName(fileName) == 0) {
            if(checkTimeOptions(statBuffer) == 0)
                printFileInfo(absolutePath, statBuffer, getFileType(currEntry->d_type));

            if (strcmp(getFileType(currEntry->d_type), "dir") == 0 && findOptions.maxDepth > depth) {
                char *newPath = calloc(SIZE, sizeof(char));
                strcpy(newPath, absolutePath);
                strcat(newPath, "/");
                find(newPath, depth + 1);
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

    char *commands[4] = {"-mtime", "-atime", "-maxdepth", "nftw"};

    findOptions = initializeOptions();
    int useNftw = 0;

    for (int i=2; i<argc; i++) {
        if (strcmp(argv[i], commands[0]) == 0) {
            if (i+1 >= argc)
                perror("missed argument for -mtime");

            if(argv[i+1][0] == '-')
                findOptions.mTimeSign = -1;
            else if(argv[i+1][0] == '+')
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
            else if(argv[i+1][0] == '+')
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
        } else if (strcmp(argv[i], commands[3]) == 0 && i == argc-1){
            useNftw = 1;
        } else {
                perror("invalid arguments");
        }
    }

    char *currPath = getAbsolutePath(argv[1]);

    if (useNftw == 0) {
        struct stat *fileStat = calloc(SIZE, sizeof(struct stat));
        int statRes = stat(currPath, fileStat);
        if(statRes == -1)
            perror("error in root stat");

        char *fileType = getFileTypeFromPath(getDirectoryFromPath(currPath), getNameFromPath(argv[1]));

        if(checkTimeOptions(fileStat) == 0)
            printFileInfo(currPath, fileStat, fileType);
        if(strcmp(fileType, "dir") == 0 && findOptions.maxDepth >= 1) {
            strcat(currPath, "/");
            find(currPath, 1);
        }

    } else
        nftw(currPath, printFileInfoNftw, SIZE, FTW_PHYS);

    return 0;
}