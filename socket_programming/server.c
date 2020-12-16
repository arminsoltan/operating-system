#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>

#include <netinet/in.h>
#include <unistd.h>

#include <string.h>

char hello[256];
int size;

// maximum number of user that can connect to server
const int max_client_number = 100;

void group_combining(int *client_socket, int *client_status, int *client_group, int *available_ports, int k) {
    int arr[5][3], j = 0;
    for (int i = 0; i < max_client_number; i++) {
        if (client_socket[i] != 0 && client_status[i] == 0 && client_group[i] == k) {
            arr[j][0] = client_socket[i];
            arr[j][1] = i;
            j++;
        }
    }
    int free_port = 0;
    for (int i = 9003; i < 10000; i++) {
        if (available_ports[i] == 0) {
            free_port = i;
            break;
        }
    }
    sprintf(hello, "%d of %d members are ready! \n", j, k);
    size = write(STDOUT_FILENO, hello, strlen(hello));
    size = write(STDOUT_FILENO, "--------------- \n", strlen("--------------- \n"));
    if (j == k) {
        for (int i = 0; i < j; i++) {
            free_port = free_port * 10 + (i + 1);
            char port[100];
            sprintf(port, "%d", free_port);
            sprintf(hello, "(%c,%c,%c,%c,%d) \n", port[0], port[1], port[2], port[3], free_port);
	    size = write(STDOUT_FILENO, hello, strlen(hello));
            if (!send(arr[i][0], &port, sizeof(port), 0)) {
                perror("send failure");
            }
            free_port /= 10;
            client_status[arr[i][1]] = 1;
            available_ports[free_port] = 1;
        }
	puts(" ");
    }
}

int main(int argc, char *argv[]) {
    int client_socket[100], client_status[100], client_group[100], available_ports[10000];
    char server_messaged[256] = "You have reached the server";
    // fill array available ports from ports number 9003 to 9103
    for (int i = 0; i < 1000; i++) {
        available_ports[i] = 0;
    }

    // set of socket descriptor
    fd_set read_fds;

    // initialize clients to zero
    for (int i = 0; i < max_client_number; i++) {
        client_socket[i] = 0;
        client_status[i] = 0;
        client_group[i] = 0;
    }

    // create the server socket
    int server_socket;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == 0) {
        perror("server socket failed");
        exit(EXIT_FAILURE);
    }

    // type of socket created
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(atoi(&argv[1][5]));
    server_address.sin_addr.s_addr = INADDR_ANY;

    // bind the socket to our specified IP and port
    if (bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        perror("bind failure");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 5) < 0) {
        perror("listen failure");
        exit(EXIT_FAILURE);
    }

    while (1 == 1) {
        // clear the socket set
        FD_ZERO(&read_fds);

        // add server socket to set to file descriptor
        FD_SET(server_socket, &read_fds);
        int max_sd = server_socket;

        // add child sockets to set
        for (int i = 0; i < max_client_number; i++) {
            // socket descriptor
            int sd = client_socket[i];

            // if valid socket descriptor should append to list
            if (sd > 0)
                FD_SET(sd, &read_fds);

            // highest socket descriptor should be updated
            if (sd > max_sd)
                max_sd = sd;
        }

        // wait for activity of one of the client socket and timeout is Null so time is indefinitely
        // number of socket descriptor, socket descriptor, write descriptor, exceptional descriptor, timeout
        int activity = select(max_sd + 1, &read_fds, NULL, NULL, NULL);

        if (activity < 0)
            puts("select error");
        int new_socket;
        // check for incoming connection and handle it
        if (FD_ISSET(server_socket, &read_fds)) {
            if ((new_socket = accept(server_socket, NULL, NULL)) < 0) {
                perror("accept failure");
                exit(EXIT_FAILURE);
            }
            // if (send(new_socket, server_messaged, sizeof(server_messaged), 0) != sizeof(server_messaged)) {
            //     perror("send failure");
            // }
            int client_socket_index = 0;
            for (int i = 0; i < max_client_number; i++) {
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    client_socket_index = i;
                    puts("adding to list of sockets");
                    break;
                }
            }
            // receiving message from client for making group
            char client_message[256];
            if (!recv(new_socket, &client_message, sizeof(client_message), 0)) {
                perror("receive failure");
            }
	    size = write(STDOUT_FILENO, "--------------- \n", strlen("--------------- \n"));
            sprintf(hello, "One client connected for %s player game. \n", client_message);
	    size = write(STDOUT_FILENO, hello, strlen(hello));
            if (client_message[0] == '2') {
                client_group[client_socket_index] = 2;
                group_combining(client_socket, client_status, client_group, available_ports, 2);
            } else if (client_message[0] == '3') {
                client_group[client_socket_index] = 3;
                group_combining(client_socket, client_status, client_group, available_ports, 3);
            } else if (client_message[0] == '4') {
                client_group[client_socket_index] = 4;
                group_combining(client_socket, client_status, client_group, available_ports, 4);
            }
        }

    }

    // close the socket
    close(server_socket);

    return 0;
}