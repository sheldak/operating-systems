#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// useful global variables to have following information in handler function
pid_t senderPID = -1;
int signalsCaught = 0;
int caughtAll = 0;

int mode = 0; // 0 - kill function, 1 - sigqueue function

int SIG1 = 0;
int SIG2 = 0;

void sendSignal(int signum, int sigValue) {
    // using kill function to send signals (modes: "kill" and "sigrt")
    if(mode == 0)
        kill(senderPID, signum);
        // using sigqueue function to send signals (mode: "sigqueue")
    else {
        union sigval value;
        value.sival_int = sigValue;

        sigqueue(senderPID, signum, value);
    }
}

void handler(int signum, siginfo_t *info, void *ucontext) {
    // saving sender's PID useful for sending signals later
    if(senderPID == -1)
        senderPID = info->si_pid;

    // counting signals
    if(signum == SIG1) {
        signalsCaught++;
        sendSignal(SIG1, -1);
    }
    else if(signum == SIG2)
        caughtAll = 1;
}

int main(int argc, char **argv) {
    if (argc < 2)
        perror("too few arguments");

    // printing information about catcher's PID
    printf("Catcher's PID: %d\n", getpid());

    // modes
    char *arguments[3] = {"kill", "sigqueue", "sigrt"};

    // setting signals which will be sent
    if(strcmp(argv[1], arguments[2]) == 0) {
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

    if(strcmp(argv[1], arguments[1]) == 0)
        mode = 1;

    // waiting for all signals from sender
    while(caughtAll == 0) {}

    if(strcmp(argv[1], arguments[1]) != 0) {
        // using kill function to send signals (modes: "kill" and "sigrt")
        for(int i=0; i<signalsCaught; i++)
            sendSignal(SIG1, -1);
        sendSignal(SIG2, -1);
    }
    else {
        // using sigqueue function to send signals (mode: "sigqueue")
        for(int i=0; i<signalsCaught; i++)
            sendSignal(SIG1, i+1);

        sendSignal(SIG2, signalsCaught);
    }

    // result message
    printf("Catcher has caught %d signals\n", signalsCaught);

    return 0;
}