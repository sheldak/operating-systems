#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define SIZE 128

// parsing file to 2d array of commands in lines
int *parseFile(int fileDesc, char ***commands) {
    for(int i=0; i<SIZE; i++) {
        commands[i] = calloc(SIZE, sizeof(char*));
        for(int j=0; j<SIZE; j++)
            commands[i][j] = calloc(SIZE, sizeof(char));
    }

    int *size = calloc(SIZE, sizeof(int));

    char *currChar = calloc(1, sizeof(char));
    char *command = calloc(SIZE, sizeof(char));
    int currLine = 0;
    int currCommand = 0;

    while(read(fileDesc, currChar, 1) != 0) {
        if(strcmp(currChar, "|") == 0) {
            strcpy(commands[currLine][currCommand], command);
            currCommand++;
            memset(command, 0, SIZE);
        }
        else if(strcmp(currChar, "\n") == 0) {
            strcpy(commands[currLine][currCommand], command);
            size[currLine] = currCommand;
            currCommand = 0;
            currLine++;
            memset(command, 0, SIZE);
        }
        else
            strcat(command, currChar);
    }

    return size;
}

// making list of arguments from command string
char **getCommand(char *command) {
    char **commandParsed = calloc(SIZE, sizeof(char));

    char *token;

    int i=0;
    token = strtok(command, " ");
    while(token != NULL) {
        commandParsed[i] = calloc(SIZE, sizeof(char));
        strcpy(commandParsed[i], token);
        token = strtok(NULL, " ");
        i++;
    }
    commandParsed[i] = NULL;

    return commandParsed;
}


int main(int argc, char **argv) {
    if(argc < 2)
        perror("too few arguments");

    // getting contents of file with commands
    char *filepath = argv[1];
    int fileDesc = open(filepath, O_RDONLY);

    char ***commands = calloc(SIZE, sizeof(char**));
    int *commandsSize = parseFile(fileDesc, commands);

    close(fileDesc);

    // making processes to interpret commands
    int i =0;
    while(commandsSize[i] != 0) {  // for every line in the file
        int fd1[2];
        pipe(fd1);

        // first command
        pid_t currPid = fork();
        if(currPid == 0) {
            close(fd1[0]);
            dup2(fd1[1], STDOUT_FILENO);

            char **command = getCommand(commands[i][0]);
            execvp(command[0], command);
        }
        // next commands
        else {
            int fd2[2];

            for(int j=1; j<=commandsSize[i]; j++) {
                pipe(fd2);
                currPid = fork();

                if(currPid == 0) {
                    close(fd1[1]);
                    close(fd2[0]);
                    dup2(fd1[0], STDIN_FILENO);

                    // if not last command
                    if(j < commandsSize[i])
                        dup2(fd2[1], STDOUT_FILENO);
                    else
                        close(fd2[1]);

                    char **command = getCommand(commands[i][j]);
                    execvp(command[0], command);
                }
                else {
                    close(fd1[0]);
                    close(fd1[1]);
                    fd1[0] = fd2[0];
                    fd1[1] = fd2[1];
                }
            }
            close(fd2[0]);
            close(fd2[1]);
        }
        for(int j=1; j<= commandsSize[i]; j++)
            wait(NULL);

        i++;
    }
    return 0;
}
