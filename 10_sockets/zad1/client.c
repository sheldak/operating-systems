#include "utilities.h"

char name[MAX_NAME_SIZE];
int domain;

struct sockaddr_un unix_address;
struct sockaddr_in inet_address;

int socket_fd;

int my_turn = 0;

int move = -1;
pthread_mutex_t move_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t move_cond = PTHREAD_COND_INITIALIZER;

void terminate() {
    // closing socket
    if(close(socket_fd) < 0) perror("Cannot close socket");

    exit(0);
}

void handle_SEGV_signal() {
    printf("Segmentation fault\n");

    // terminating process after segmentation fault
    exit(0);
}

void handle_INT_signal() {
    // terminating process
    exit(0);
}

void print_board(int board[3][3]) {
    for(int i=0; i<3; i++) {
        if(i == 1 || i == 2)
            printf("------\n");

        for(int j=0; j<3; j++) {
            if(j == 1 || j == 2)
                printf("|");

            if(board[i][j] == X)
                printf("X");
            else if(board[i][j] == O)
                printf("O");
            else
                printf(" ");
        }
        printf("\n");
    }
}

int check_win(int board[3][3], int my_sign) {
    int win = 0;
    for(int i=0; i<3; i++) {
        if(board[i][0] == board[i][1] && board[i][0] == board[i][2]) {
            if(board[i][0] == X)
                win = X;
            else if(board[i][0] == O)
                win = O;
        }

        if(board[0][i] == board[1][i] && board[0][i] == board[2][i]) {
            if(board[0][i] == X)
                win = X;
            else if(board[0][i] == O)
                win = O;
        }
    }

    if(board[0][0] == board[1][1] && board[0][0] == board[2][2]) {
        if(board[0][0] == X)
            win = X;
        else if(board[0][0] == O)
            win = O;
    }

    if(board[0][2] == board[1][1] && board[0][2] == board[2][0]) {
        if(board[0][2] == X)
            win = X;
        else if(board[0][2] == O)
            win = O;
    }

    if(win == 0)
        return 0;
    else if(win == my_sign)
        printf("Victory!\n");
    else
        printf("Defeat!\n");

    return 1;
}

_Noreturn void *communicate() {
    int ID = -1;
    int sign = 0;

    // message structure
    message *msg = malloc(sizeof(message));

    // sending message with name
    msg->type = NAME;
    strcpy(msg->name, name);

    if(write(socket_fd, msg, sizeof(message)) < 0) perror("Cannot write message with name");

    // waiting for name acceptance
    if(read(socket_fd, msg, sizeof(message)) < 0) perror("Cannot read name acceptance");

    // interpreting response after name message
    if(msg->type == STOP) {
        printf("Not unique name\n");
        exit(0);
    }
    else if(msg->type == WAIT && read(socket_fd, msg, sizeof(message)) < 0)
        perror("Cannot read start game message");

    if(msg->type == INIT) {
        ID = msg->ID;
        sign = msg->sign;

        if(msg->sign == X)
            my_turn = 1;
    }

    // communication
    while(1) {
        if(my_turn == 0) {

        }
        else {
            // locking mutex
            pthread_mutex_lock(&move_mutex);

            // waiting for next move
            while (move == -1) {
                pthread_cond_wait(&move_cond, &move_mutex);

                if(msg->board[move/3][move%3] != 0) {
                    move = -1;
                    printf("Can't make this move\n");
                }
            }

            // making move
            msg->board[move/3][move%3] = sign;

            // printing board
            print_board(msg->board);

            // checking if someone wins
            int win = check_win(msg->board, sign);

            // continue game
            if(win == 0) {

            }
            // end game
            else {

            }

            // ending my turn
            my_turn = 0;

            // unlocking mutex
            pthread_mutex_unlock(&move_mutex);
        }
    }

    return (void*) 0;
}

void handle_move(char *move_text) {
    char *rest;
    int move_int = (int) strtol(move_text, &rest, 10);

    if(move_int >= 0 && move_int < 9) {
        if(my_turn == 0)
            printf("It's not my turn\n");
        else {
            // locking mutex
            pthread_mutex_lock(&move_mutex);

            // setting next move
            move = move_int;

            // informing connection thread about new move
            pthread_cond_broadcast(&move_cond);

            // unlocking mutex
            pthread_mutex_unlock(&move_mutex);
        }
    }
    else
        printf("Invalid command\n");
}

int main(int argc, char **argv) {
    if (setvbuf(stdout, NULL, _IONBF, 0) != 0) {
        printf("Error: buffering mode could not be changed!\n");
        exit(1);
    }

    if (argc < 4) perror("Too few arguments");

    if (atexit(terminate) != 0) perror("atexit function error");

    // to stop program properly in case of segmentation fault
    if (signal(SIGSEGV, handle_SEGV_signal) == SIG_ERR) perror("Signal function error when setting SIGSEGV handler");

    // to stop program properly in case of SIGINT signal
    if (signal(SIGINT, handle_INT_signal) == SIG_ERR) perror("signal function error when setting SIGINT handler");

    // first argument - client's name
    strcpy(name, argv[1]);

    // second argument - domain of protocols
    if(strcmp(argv[2], "unix") == 0)
        domain = AF_UNIX;
    else if(strcmp(argv[2], "inet") == 0)
        domain = AF_INET;
    else
        perror("Invalid domain");

    if(domain == AF_UNIX) {
        // third argument - socket's address
        unix_address.sun_family = AF_UNIX;
        strcpy(unix_address.sun_path, argv[3]);

        // creating socket
        socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (socket_fd < 0) perror("Cannot create unix socket");

        // connecting to the server
        if(connect(socket_fd, (struct sockaddr*) &unix_address, sizeof(unix_address)) == -1)
            perror("Cannot connect by unix address");
    }
    else {
        // third argument - socket's address;
        if(argc < 5) perror("Too few arguments");

        inet_address.sin_family = AF_INET;
        if(inet_aton(argv[3], &inet_address.sin_addr) == 0) perror("Invalid IP address");

        // forth argument - port
        char *rest;
        inet_address.sin_port = (uint32_t) strtol(argv[4], &rest, 10);

        // creating socket
        socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fd < 0) perror("Cannot create IPv4 socket");

        // connecting to the server
        if(connect(socket_fd, (struct sockaddr*) &inet_address, sizeof(inet_address)) == -1)
            perror("Cannot connect by inet address");
    }

    // making thread which is receiving and sending messages
    pthread_t communicationThread;
    if (pthread_create(&communicationThread, NULL, communicate, NULL) != 0)
        perror("Cannot create communication thread");

    // main thread is handling terminal commands
    while(1) {
        char *move_text = malloc(TEXT_SIZE * sizeof(char));
        size_t length = 0;

        getline(&move_text, &length, stdin);
        handle_move(move_text);
    }

//    printf("%d\n", inet_address.sin_addr.s_addr);
//    printf("%u\n", inet_address.sin_port);

}