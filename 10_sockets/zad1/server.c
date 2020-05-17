#include "utilities.h"

int unix_socket_fd;
int inet_socket_fd;
int clients_sockets_fds[MAX_CLIENTS];

struct sockaddr_un unix_address;
struct sockaddr_in inet_address;


void terminate() {
    // closing unix socket and unlinking file
    if(close(unix_socket_fd) < 0) perror("Cannot close unix socket");
    if(unlink(unix_address.sun_path) < 0) perror("Cannot unlink path");

    // closing inet socket
    if(close(inet_socket_fd) < 0) perror("Cannot close inet socket");

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

_Noreturn void *communicate() {
    char *clients_names[MAX_CLIENTS];
    int partner[MAX_CLIENTS];
    int waiting_client = -1;

    // initializing arrays
    for(int i=0; i<MAX_CLIENTS; i++) {
        clients_sockets_fds[i] = -1;
        strcpy(clients_names[i], "");
        partner[i] = -1;
    }

    // creating epoll file descriptor
    int epoll_fd = epoll_create1(0);

    // making event structures
    struct epoll_event unix_event;
    unix_event.events = EPOLLIN | EPOLLOUT;
    unix_event.data.fd = unix_socket_fd;

    struct epoll_event inet_event;
    inet_event.events = EPOLLIN | EPOLLOUT;
    inet_event.data.fd = inet_socket_fd;

    struct epoll_event *events = calloc(MAX_CLIENTS, sizeof(struct epoll_event));

    // registering descriptors of sockets
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, unix_socket_fd, &unix_event) < 0) perror("Cannot register unix socket");
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, inet_socket_fd, &inet_event) < 0) perror("Cannot register inet socket");

    // message structure
    message *msg = malloc(sizeof(message));

    // communication with the clients
    while(1) {
        // waiting for events
        int events_number = epoll_wait(epoll_fd, events, MAX_CLIENTS, -1);
        if(events_number < 0) perror("Error in epoll_wait");

        int event_fd = events[events_number].data.fd;

        for(int i=0; i<events_number; i++) {
            // new client is connecting
            if(event_fd == unix_socket_fd || event_fd == inet_socket_fd) {
                int new_client_fd = accept(event_fd, NULL, NULL);

                // saving client's socket file descriptor
                for(int j=0; j<MAX_CLIENTS; j++) {
                    if(clients_sockets_fds[j] == -1)
                        clients_sockets_fds[j] = new_client_fd;
                }
            }
            // message from registered client
            else{
                if(read(event_fd, msg, sizeof(message)) < 0) perror("Cannot read");

                // initial message from client with their name
                if(msg->type == NAME) {
                    // to locate client in arrays
                    int client_ID = 0;

                    // to check if client's name is unique
                    int name_exists = 0;
                    for(int j=0; j<MAX_CLIENTS; j++) {
                        if(event_fd == clients_sockets_fds[j])
                            client_ID = j;

                        if(strcmp(clients_names[j], msg->name) == 0)
                            name_exists = 1;
                    }

                    // name is unique so looking for partner to play tic tac toe
                    if(name_exists == 0) {
                        clients_names[client_ID] = msg->name;

                        // waiting for partner to play tic tac toe
                        if(waiting_client == -1) {
                            msg->type = WAIT;

                            if(write(event_fd, msg, sizeof(message)) < 0) perror("Cannot write to player to wait");
                            waiting_client = client_ID;
                        }
                        // there is one waiting player so connecting them with the new player
                        else {
                            // initializing board
                            for(int j=0; j<3; j++) {
                                for(int k=0; k<3; k++)
                                    msg->board[j][k] = 0;
                            }

                            // getting first player
                            int first_player = rand() % 2;

                            // message for waiting player
                            msg->type = INIT;
                            msg->ID = waiting_client;
                            strcpy(msg->name, clients_names[client_ID]);
                            msg->sign = (first_player == 0) ? X : O;

                            if(write(clients_sockets_fds[waiting_client], msg, sizeof(message)) < 0)
                                perror("Cannot write to waiting player");

                            // message for new player
                            msg->ID = client_ID;
                            strcpy(msg->name, clients_names[waiting_client]);
                            msg->sign = (first_player == 1) ? X : O;

                            if(write(event_fd, msg, sizeof(message)) < 0)
                                perror("Cannot write to new player");

                            // setting variables about new pair
                            partner[client_ID] = waiting_client;
                            partner[waiting_client] = client_ID;
                            waiting_client = -1;
                        }
                    }
                    // name is not unique so letting client unregister
                    else {
                        // deleting client from the array with clients' file descriptors
                        clients_sockets_fds[client_ID] = -1;

                        // STOP message to the client
                        msg->type = STOP;

                        if(write(event_fd, msg, sizeof(message)) < 0)
                            perror("Cannot write STOP message to new player");
                    }
                }
                else if(msg->type == MOVE) {
                    // sending board to the second player
                    int partner_fd = clients_sockets_fds[partner[msg->ID]];
                    if(write(partner_fd, msg, sizeof(message)) < 0)
                        perror("Cannot write MOVE message to player");
                }
            }
        }


    }

    return (void*) 0;
}

int main(int argc, char **argv) {
    srand(time(NULL));

    if (setvbuf(stdout, NULL, _IONBF, 0) != 0) {
        printf("Error: buffering mode could not be changed!\n");
        exit(1);
    }

    if (argc < 3) perror("Too few arguments");

    if (atexit(terminate) != 0) perror("atexit function error");

    // to stop program properly in case of segmentation fault
    if (signal(SIGSEGV, handle_SEGV_signal) == SIG_ERR) perror("Signal function error when setting SIGSEGV handler");

    // to stop program properly in case of SIGINT signal
    if (signal(SIGINT, handle_INT_signal) == SIG_ERR) perror("signal function error when setting SIGINT handler");

    // first argument - port
    char *rest;
    inet_address.sin_port = htons((uint16_t) strtol(argv[1], &rest, 10));

    // second argument - unix socket path
    strcpy(unix_address.sun_path, argv[2]);

    // configuring unix address
    unix_address.sun_family = AF_UNIX;

    // configuring inet address
    inet_address.sin_family = AF_INET;

    struct addrinfo inet_hints;
    memset(&inet_hints, 0, sizeof(struct addrinfo));
    inet_hints.ai_family = AF_INET;
    inet_hints.ai_socktype = SOCK_STREAM;
    inet_hints.ai_flags = AI_PASSIVE;

    struct addrinfo *inet_address_info;
    if(getaddrinfo("localhost", NULL, &inet_hints, &inet_address_info) != 0)
        perror("Cannot get address info");

    inet_address.sin_addr = ((struct sockaddr_in*) inet_address_info->ai_addr)->sin_addr;

    // creating unix socket
    unix_socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(unix_socket_fd < 0) perror("Cannot create unix socket");
    if(bind(unix_socket_fd, (struct sockaddr*) &unix_address, sizeof(unix_address)) == -1)
        perror("Cannot bind unix socket");
    if(listen(unix_socket_fd, MAX_CLIENTS) < 0) perror("Error in listening (unix)");

    // creating inet socket
    inet_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(inet_socket_fd < 0) perror("Cannot create IPv4 socket");
    if(bind(inet_socket_fd, (struct sockaddr*) &inet_address, sizeof(inet_address)) == -1)
        perror("Cannot bind inet socket");
    if(listen(inet_socket_fd, MAX_CLIENTS) < 0) perror("Error in listening (inet)");

    // making thread which is receiving and sending messages
    pthread_t communicationThread;
    if (pthread_create(&communicationThread, NULL, communicate, NULL) != 0)
        perror("Cannot create communication thread");


//    // first argument - number of chairs in waiting room
//    char *rest;
//    chairs = (int) strtol(argv[1], &rest, 10);
//
//    // second argument - number of clients
//    int clients = (int) strtol(argv[2], &rest, 10);
//
//    // initialing waiting room array
//    waitingRoom = calloc(chairs, sizeof(int));
//    for(int i=0; i<chairs; i++)
//        waitingRoom[i] = -1;
//    firstClient = 0;
//
//    // barber thread ID
//    pthread_t barberThreadID;
//
//    // array with clients threads' IDs
//    clientsThreadsIDs = calloc(clients, sizeof(pthread_t));
//
//    // making barber thread
//    if (pthread_create(&barberThreadID, NULL, barber, NULL) != 0) perror("Cannot create barber's thread");
//
//    // making clients threads
//    for (int i = 0; i < clients; i++) {
//        waitRandomTime(0);
//
//        clientID *ID = malloc(sizeof(clientID));
//        ID->ID = i;
//
//        if (pthread_create(&clientsThreadsIDs[i], NULL, client, ID) != 0)
//            perror("Cannot create client's thread");
//    }
//
//    // waiting for threads termination
//    for (int i = 0; i < clients; i++) {
//        if (pthread_join(clientsThreadsIDs[i], NULL) != 0) perror("pthread_join error");
//    }
//
//    // canceling barber thread
//    pthread_cancel(barberThreadID);
//
//    // destroying mutex and condition variable
//    pthread_mutex_destroy(&waitingRoomMutex);
//    pthread_cond_destroy(&waitingRoomEmptiness);
//
//    // freeing memory
//    free(waitingRoom);
//    free(clientsThreadsIDs);

    return 0;
}
