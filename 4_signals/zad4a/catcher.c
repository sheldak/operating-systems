#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define SIZE 256

pid_t senderPID = -1;
int signalsCaught = 0;
int caughtAll = 0;

int SIG1 = 0;
int SIG2 = 0;

void handler(int signum, siginfo_t *info, void *ucontext) {
    if(senderPID == -1)
        senderPID = info->si_pid;

    if(signum == SIG1)
        signalsCaught++;
    else if(signum == SIG2)
        caughtAll = 1;
}

int main(int argc, char **argv) {
    if (argc < 2)
        perror("too few arguments");

    printf("Catcher's PID: %d\n", getpid());

    char *arguments[3] = {"kill", "sigqueue", "sigrt"};

    if(strcmp(argv[1], arguments[2]) == 0) {
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

//    signal(SIG1, handler2);
//    signal(SIG2, handler2);

    // -------- send signals from catcher ...
    while(caughtAll == 0) {}
//
    if(strcmp(argv[1], arguments[1]) != 0) {
        for(int i=0; i<signalsCaught; i++)
            kill(senderPID, SIG1);
        kill(senderPID, SIG2);
    }
    else {
        for(int i=0; i<signalsCaught; i++) {
            union sigval value;
            value.sival_int = i+1;

            sigqueue(senderPID, SIG1, value);
        }

        union sigval value;
        value.sival_int = signalsCaught;

        sigqueue(senderPID, SIG2, value);
    }

    printf("Catcher has caught %d signals\n", signalsCaught);

    return 0;
}