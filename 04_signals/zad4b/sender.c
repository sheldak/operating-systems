#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE 256

// useful global variables to have following information in handler function
int signals = 0;
int signalsCaught = 0;
int caughtAll = 0;

int sendingEnded = 0;
int canSend = 1;

int catchersSignal = -1;

int SIG1 = 0;
int SIG2 = 0;

void handler(int signum, siginfo_t *info, void *ucontext) {
    // saving number of current catcher's signal (in case of mode "sigqueue")
    if(sendingEnded == 1 && catchersSignal >= 0)
        catchersSignal = info->si_value.sival_int;

    // getting first signal
    if(signum == SIG1) {
        // counting signal when sender has sent all of its signals
        if(sendingEnded)
            signalsCaught++;
        // getting information that catcher has confirmed catching signal
        else
            canSend = 1;
    }
    else if(signum == SIG2)
        caughtAll = 1;
}

int main(int argc, char **argv) {
    if (argc < 4)
        perror("too few arguments");

    // modes
    char *arguments[3] = {"kill", "sigqueue", "sigrt"};

    // getting catcher PID
    char *rest = calloc(SIZE, sizeof(char));
    int catcherPID = (int) strtol(argv[1], &rest, 10);
    if(strcmp(rest, "") != 0)
        perror("invalid first argument, should be number of signals to send");

    // getting number of signals to send
    signals = (int) strtol(argv[2], &rest, 10);
    if(strcmp(rest, "") != 0)
        perror("invalid first argument, should be number of signals to send");

    // checking correctness of mode
    if(strcmp(argv[3], arguments[0]) != 0 && strcmp(argv[3], arguments[1]) != 0 && strcmp(argv[3], arguments[2]) != 0)
        perror("invalid third argument");

    // setting signals which will be sent
    if(strcmp(argv[3], arguments[2]) == 0) {
        SIG1 = SIGRTMIN;
        SIG2 = SIGRTMAX;
    }
    else {
        SIG1 = SIGUSR1;
        SIG2 = SIGUSR2;
    }

    // making mask to avoid catching unnecessary signals
    sigset_t mask;
    sigfillset(&mask);
    sigdelset(&mask, SIG1);
    sigdelset(&mask, SIG2);
    sigprocmask(SIG_SETMASK, &mask, NULL);

    // sigaction for signals handling
    struct sigaction action;
    action.sa_sigaction = handler;
    action.sa_mask = mask;
    action.sa_flags = SA_SIGINFO;

    // setting sigactions for both signals
    sigaction(SIG1, &action, NULL);
    sigaction(SIG2, &action, NULL);

    if(strcmp(argv[3], arguments[1]) != 0) {
        // using kill function to send signals (modes: "kill" and "sigrt")
        for(int i=0; i<signals; i++) {
            while (canSend == 0) {}

            kill(catcherPID, SIG1);
            canSend = 0;
        }

        while (canSend == 0) {}
        kill(catcherPID, SIG2);
    }
    else {
        // using sigqueue function to send signals (mode: "sigqueue")
        catchersSignal = 0;

        union sigval value;
        value.sival_ptr = NULL;

        for(int i=0; i<signals; i++) {
            while(canSend == 0) {}

            sigqueue(catcherPID, SIG1, value);
            canSend = 0;
        }

        while(canSend == 0) {}
        sigqueue(catcherPID, SIG2, value);
    }
    sendingEnded = 1;

    // to wait for all signals from catcher
    while(caughtAll == 0){}

    // result messages
    printf("Sender has sent %d signals and has got %d signals\n", signals, signalsCaught);

    if(catchersSignal >= 0)
        printf("Catcher has sent the information that it has caught %d signals\n", catchersSignal);

    return 0;
}