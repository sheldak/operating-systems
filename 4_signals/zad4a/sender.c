#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define SIZE 256

int signals = 0;
int signalsCaught = 0;
int caughtAll = 0;

int catchersSignal = -1;

int SIG1 = 0;
int SIG2 = 0;

void handler(int signum, siginfo_t *info, void *ucontext) {
    if(catchersSignal >= 0)
        catchersSignal = info->si_value.sival_int;
    if(signum == SIG1)
        signalsCaught++;
    else if(signum == SIG2)
        caughtAll = 1;
}

int main(int argc, char **argv) {
    if (argc < 4)
        perror("too few arguments");

    char *arguments[3] = {"kill", "sigqueue", "sigrt"};

    char *rest = calloc(SIZE, sizeof(char));
    int catcherPID = (int) strtol(argv[1], &rest, 10);
    if(strcmp(rest, "") != 0)
        perror("invalid first argument, should be number of signals to send");

    signals = (int) strtol(argv[2], &rest, 10);
    if(strcmp(rest, "") != 0)
        perror("invalid first argument, should be number of signals to send");

    if(strcmp(argv[3], arguments[0]) != 0 && strcmp(argv[3], arguments[1]) != 0 && strcmp(argv[3], arguments[2]) != 0)
        perror("invalid third argument");

    if(strcmp(argv[3], arguments[2]) == 0) {
        SIG1 = SIGRTMIN;
        SIG2 = SIGRTMAX;
    }
    else {
        SIG1 = SIGUSR1;
        SIG2 = SIGUSR2;
    }

    sigset_t mask;
    sigfillset(&mask);
    sigdelset(&mask, SIG1);
    sigdelset(&mask, SIG2);
    sigprocmask(SIG_SETMASK, &mask, NULL);

    struct sigaction action;
    action.sa_sigaction = handler;
    action.sa_mask = mask;
    action.sa_flags = SA_SIGINFO;

    sigaction(SIG1, &action, NULL);
    sigaction(SIG2, &action, NULL);

    if(strcmp(argv[3], arguments[1]) != 0) {
        for(int i=0; i<signals; i++)
            kill(catcherPID, SIG1);
        kill(catcherPID, SIG2);
    }
    else {
        catchersSignal = 0;

        union sigval value;
        value.sival_ptr = NULL;

        for(int i=0; i<signals; i++)
            sigqueue(catcherPID, SIG1, value);
        sigqueue(catcherPID, SIG2, value);
    }

    while(caughtAll == 0){}

    printf("Sender has sent %d signals and has got %d signals\n", signals, signalsCaught);

    if(catchersSignal >= 0)
        printf("Catcher has sent the information that it has caught %d signals\n", catchersSignal);

    return 0;
}