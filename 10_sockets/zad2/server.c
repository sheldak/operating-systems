#include "utilities.h"

int unix_socket_fd;
int inet_socket_fd;

int registered_client[MAX_CLIENTS];
struct sockaddr clients_addresses[MAX_CLIENTS];
int clients_domains[MAX_CLIENTS];

char clients_names[MAX_CLIENTS][MAX_NAME_SIZE];
int partner[MAX_CLIENTS];

struct sockaddr_un unix_address;
struct sockaddr_in inet_address;

int epoll_fd = -1;

int pinged[MAX_CLIENTS];

pthread_mutex_t ping_mutex = PTHREAD_MUTEX_INITIALIZER;

void send_message(message *msg, int ID) {
    int fd = (clients_domains[ID] == UNIX) ? unix_socket_fd : inet_socket_fd;
    socklen_t address_size;

    if(fd == unix_socket_fd)
        address_size = sizeof((struct sockaddr_un*) &clients_addresses[ID]);
    else
        address_size = sizeof(clients_addresses[ID]);

    if(sendto(fd, msg, sizeof(message), 0, &clients_addresses[ID], address_size) < 0)
        perror("Cannot send message");
}

void kill_clients() {
    // message structure
    message *msg = malloc(sizeof(message));
    msg->type = KILL;

    // sending messages to all clients
    for(int i=0; i<MAX_CLIENTS; i++) {
        if(registered_client[i] != 0)
            send_message(msg, i);
    }

    // waiting for clients' responses
    for(int i=0; i<MAX_CLIENTS; i++) {
        if(registered_client[i] != 0) {
            while(registered_client[i] != 0) {
                int fd = (clients_domains[i] == UNIX) ? unix_socket_fd : inet_socket_fd;

                struct sockaddr address;
                socklen_t address_size = sizeof(address);

                if(recvfrom(fd, msg, sizeof(message), MSG_WAITALL, &address, &address_size) < 0)
                    perror("Cannot get message from killed client");

                if(msg->type == STOP)
                    registered_client[msg->ID] = 0;
            }
        }
    }
}

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
    kill_clients();

    // terminating process
    exit(0);
}

void handle_PIPE_signal() {
    // terminating process
    exit(0);
}

_Noreturn void *communicate() {
    int waiting_client = -1;

    // initializing arrays
    for(int i=0; i<MAX_CLIENTS; i++) {
        registered_client[i] = 0;
        clients_domains[i] = -1;
        strcpy(clients_names[i], "");
        partner[i] = -1;
    }

    // creating epoll file descriptor
    epoll_fd = epoll_create1(0);

    // making event structures
    struct epoll_event unix_event;
    unix_event.events =  EPOLLIN;
    unix_event.data.fd = unix_socket_fd;

    struct epoll_event inet_event;
    inet_event.events = EPOLLIN;
    inet_event.data.fd = inet_socket_fd;

    struct epoll_event events[MAX_CLIENTS];

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

        for(int i=0; i<events_number; i++) {
            // file descriptor of the socket on which event occurred
            int event_fd = events[i].data.fd;
            struct sockaddr address;
            socklen_t address_size = sizeof(address);

            if(recvfrom(event_fd, msg, sizeof(message), MSG_WAITALL, &address, &address_size) < 0)
                perror("Cannot receive");

            // initial message from client with their name
            if(msg->type == NAME) {
                // to locate client in arrays
                int client_ID = -1;

                // to check if client's name is unique
                int name_exists = 0;
                for(int j=0; j<MAX_CLIENTS; j++) {
                    if(registered_client[j] == 0 && client_ID == -1)
                        client_ID = j;

                    if(strcmp(clients_names[j], msg->name) == 0)
                        name_exists = 1;
                }

                // registering client
                registered_client[client_ID] = 1;
                clients_addresses[client_ID] = address;
                clients_domains[client_ID] = (event_fd == unix_socket_fd) ? UNIX : INET;
                printf("Player %d registered\n", client_ID);

                // name is unique so looking for partner to play tic tac toe
                if(name_exists == 0) {
                    strcpy(clients_names[client_ID], msg->name);

                    // waiting for partner to play tic tac toe
                    if(waiting_client == -1) {
                        msg->type = WAIT;
                        msg->ID = client_ID;

                        send_message(msg, client_ID);

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

                        send_message(msg, waiting_client);

                        // message for new player
                        msg->ID = client_ID;
                        strcpy(msg->name, clients_names[waiting_client]);
                        msg->sign = (first_player == 1) ? X : O;

                        send_message(msg, client_ID);

                        // setting variables about new pair
                        partner[client_ID] = waiting_client;
                        partner[waiting_client] = client_ID;
                        waiting_client = -1;

                        printf("Players %d and %d connected\n", client_ID, partner[client_ID]);
                    }
                }
                // name is not unique so letting client unregister
                else {
                    // STOP message to the client
                    msg->type = STOP;
                    msg->ID = client_ID;

                    send_message(msg, client_ID);
                }
            }
            else if(msg->type == MOVE) {
                // sending board to the second player
                send_message(msg, partner[msg->ID]);
            }
            else if(msg->type == STOP || msg->type == KILL) {
                if(registered_client[msg->ID] != 0) {
                    if(msg->type == KILL && partner[msg->ID] != -1)
                        send_message(msg, partner[msg->ID]);

                    // deleting client
                    pthread_mutex_lock(&ping_mutex);

                    pinged[msg->ID] = 0;

                    registered_client[msg->ID] = 0;
                    clients_domains[msg->ID] = -1;
                    partner[msg->ID] = -1;
                    strcpy(clients_names[msg->ID], "");

                    printf("Player %d unregistered\n", msg->ID);

                    pthread_mutex_unlock(&ping_mutex);

                    if(waiting_client == msg->ID)
                        waiting_client = -1;
                }
            }
            else if(msg->type == PING) {
                // saving that the client responded to ping
                pthread_mutex_lock(&ping_mutex);
                pinged[msg->ID] = 0;
                pthread_mutex_unlock(&ping_mutex);
            }
        }
    }
}

_Noreturn void *ping() {
    for(int i=0; i<MAX_CLIENTS; i++)
        pinged[i] = 0;

    while(1) {
        message *msg = malloc(sizeof(message));

        sleep(1);
        for(int i=0; i<MAX_CLIENTS; i++) {
            if (registered_client[i] != 0) {
                // saving information which clients are pinged
                pthread_mutex_lock(&ping_mutex);
                pinged[i] = 1;

                // pinging
                msg->type = PING;
                send_message(msg, i);

                pthread_mutex_unlock(&ping_mutex);
            }
        }

        // every client has 1 second for response
        sleep(1);

        for(int i=0; i<MAX_CLIENTS; i++) {
            if (pinged[i] == 1) {
                // locking mutexes to delete non-responding client
                pthread_mutex_lock(&ping_mutex);

                // deleting client
                registered_client[msg->ID] = 0;
                clients_domains[msg->ID] = -1;
                partner[i] = -1;
                strcpy(clients_names[i], "");

                pinged[i] = 0;

                printf("Non-responding player %d unregistered\n", i);

                // unlocking mutexes
                pthread_mutex_unlock(&ping_mutex);
            }
        }
    }
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

    // to ignore SIGPIPE signal
    if (signal(SIGPIPE, handle_PIPE_signal) == SIG_ERR) perror("signal function error when setting SIGPIPE handler");

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
    inet_hints.ai_socktype = SOCK_DGRAM;
    inet_hints.ai_flags = AI_PASSIVE;

    struct addrinfo *inet_address_info;
    if(getaddrinfo("localhost", NULL, &inet_hints, &inet_address_info) != 0)
        perror("Cannot get address info");

    inet_address.sin_addr = ((struct sockaddr_in*) inet_address_info->ai_addr)->sin_addr;

    // creating unix socket
    unix_socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(unix_socket_fd < 0) perror("Cannot create unix socket");
    if(bind(unix_socket_fd, (const struct sockaddr*) &unix_address, sizeof(unix_address)) == -1)
        perror("Cannot bind unix socket");

    // creating inet socket
    inet_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(inet_socket_fd < 0) perror("Cannot create IPv4 socket");
    if(bind(inet_socket_fd, (struct sockaddr*) &inet_address, sizeof(inet_address)) == -1)
        perror("Cannot bind inet socket");

    // making thread which is receiving and sending messages
    pthread_t communicationThread;
    if (pthread_create(&communicationThread, NULL, communicate, NULL) != 0)
        perror("Cannot create communication thread");

    // making thread which is pinging
    pthread_t pingThread;
    if (pthread_create(&pingThread, NULL, ping, NULL) != 0)
        perror("Cannot create ping thread");

    while(1) {}

    return 0;
}
