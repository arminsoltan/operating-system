#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <string.h>

#include <signal.h>

struct Game {
    int x1;
    int x2;
    int y1;
    int y2;
    int turn;
    int point[5];
    int step;
    int winner;
};
int matrix[10][10][10][10];

int compute_point(x1, y1, x2, y2, turn) {
    int point = 0;
    if (x1 == x2) {
        if (matrix[x1 + 1][y1][x2 + 1][y2] + matrix[x1][y1][x1 + 1][y1] + matrix[x2][y2][x2 + 1][y2] == 3)
            point++;
        if (matrix[x1 - 1][y1][x2 - 1][y2] + matrix[x1][y1][x1 - 1][y1] + matrix[x2][y2][x2 - 1][y2] == 3)
            point++;
    } else if (y1 == y2) {
        if (matrix[x1][y1 - 1][x2][y2 - 1] + matrix[x1][y1][x1][y1 - 1] + matrix[x2][y2][x2][y2 - 1] == 3)
            point++;
        if (matrix[x1][y1 + 1][x2][y2 + 1] + matrix[x1][y1][x1][y1 + 1] + matrix[x2][y2][x2][y2 + 1] == 3)
            point++;

    }
    return point;
}

void draw_matrix(int client_number) {
    int size;
    for (int i = 1; i <= 2 * client_number + 1; i++) {
        if ((i % 2) == 1) {
            for (int j = 1; j <= client_number; j++) {
                size = write(STDOUT_FILENO, "*", strlen("*"));
                if (matrix[j][(i + 1) / 2][j + 1][(i + 1) / 2] == 1) {
                    size = write(STDOUT_FILENO, "-", strlen("-"));
                } else {
                    size = write(STDOUT_FILENO, " ", strlen(" "));
                }
            }
            size = write(STDOUT_FILENO, "*\n", strlen("*\n"));
        } else {
            for (int j = 1; j <= client_number; j++) {
                if (matrix[j][i / 2][j][i / 2 + 1] == 1) {
                    size = write(STDOUT_FILENO, "| ", strlen("| "));
                } else {
                    size = write(STDOUT_FILENO, "  ", strlen("  "));
                }
            }
            size = write(STDOUT_FILENO, "\n", strlen("\n"));
        }
    }
}


void AlrmSigHnd() {
    ssize_t bytes_written;
    bytes_written = write(STDOUT_FILENO, "Times up!!! \n", strlen("Times up!!! \n"));
}

int main(int argc, char *argv[]) {
    // input from user what kind of group he want join in
    ssize_t bytes_read;
    ssize_t bytes_written;

    bytes_written = write(STDOUT_FILENO, "enter your group: ", strlen("enter your group: "));
    char client_group_message[2];
    bytes_read = read(STDIN_FILENO, client_group_message, 2);
    int client_number;
    sscanf(&client_group_message[0], "%d", &client_number);
    // create socket
    int network_socket;
    network_socket = socket(AF_INET, SOCK_STREAM, 0);

    // specify an address for socket
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(atoi(&argv[1][5]));
    server_address.sin_addr.s_addr = INADDR_ANY;

    int connection_status = connect(network_socket, (struct sockaddr *) &server_address, sizeof(server_address));
    // check for error with connection
    if (connection_status == -1)
        bytes_written = write(STDOUT_FILENO, "There was an error making a connection to the server \n",
                              strlen("There was an error making a connection to the server \n"));

    // send group type to server
    if (!send(network_socket, &client_group_message, sizeof(client_group_message), 0)) {
        perror("send failure");
        exit(EXIT_FAILURE);
    }
    // receive data from server
    char server_response[256];
    char hello[256];
    char *port_str;
    int port, turn, my_turn, udpfd;
    if (recv(network_socket, &server_response, sizeof(server_response), 0)) {
        port_str = server_response;
        turn = 1;
        my_turn = 0;

    }
    close(network_socket);

    sscanf(port_str, "%d", &port);
    sprintf(hello, "%d \n", port);
    bytes_written = write(STDOUT_FILENO, hello, strlen(hello));
    my_turn = port % 10;
    port /= 10;

    // create socket for sending / receiving data
    if ((udpfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("creating socket failure");
        exit(EXIT_FAILURE);
    }

    // construct local address structure
    struct sockaddr_in udp_addr;
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = INADDR_BROADCAST;
    udp_addr.sin_port = htons(port);

    struct sockaddr_in udp_client;
    udp_client.sin_family = AF_INET;
    udp_client.sin_addr.s_addr = INADDR_ANY;
    udp_client.sin_port = htons(port);

    int broadcast_permission = 1;

    if (setsockopt(udpfd, SOL_SOCKET, SO_REUSEADDR, (void *) &broadcast_permission, sizeof(broadcast_permission))) {
        perror("setsockopt() failure");
        exit(EXIT_FAILURE);
    }

    if (bind(udpfd, (struct sockaddr *) &udp_addr, sizeof(udp_addr))) {
        perror("binding socket failure");
        exit(EXIT_FAILURE);
    }


    struct sockaddr_in client_addr;
    int fd = 0;
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);
    signal(SIGALRM, AlrmSigHnd);
    struct sockaddr_in sender_addr;
    struct Game game;
    for (int i = 1; i <= client_number; i++)
        game.point[i] = 0;
    game.turn = turn;
    game.step = client_number * (client_number + 1) * 2;
    while (game.step > 0) {
        sprintf(hello, "your turn: %d game turn: %d \n", my_turn, game.turn);
        bytes_written = write(STDOUT_FILENO, hello, strlen(hello));
        if (my_turn == game.turn) {
            char p[100];

            bytes_written = write(STDOUT_FILENO, "enter your coordinates x1\n", strlen("enter your coordinates x1\n"));
            alarm(20);  //Hnd will called after 20 seconds.
            int sel = select(5, &read_fds, NULL, NULL, NULL);
            if (sel > 0) {
                char posit[8];
                bytes_read = read(STDIN_FILENO, posit, 8);
                sscanf(posit, "%d %d %d %d", &game.x1, &game.y1, &game.x2, &game.y2);
                matrix[game.x1][game.y1][game.x2][game.y2] = 1;
                matrix[game.x2][game.y2][game.x1][game.y1] = 1;
                int point = compute_point(game.x1, game.y1, game.x2, game.y2, my_turn);
                game.point[my_turn] += point;
                if (point == 0)
                    game.turn = game.turn % client_number + 1;
                game.step--;
                int opt_val = 1;
                int opt_len = sizeof(opt_val);
                if (setsockopt(udpfd, SOL_SOCKET, SO_BROADCAST, (void *) &opt_val, opt_len)) {
                    perror("sesocketot() failure");
                    exit(EXIT_FAILURE);
                }
                if (!sendto(udpfd, &game, sizeof(game), 0, (struct sockaddr *) &udp_addr, sizeof(udp_addr))) {
                    perror("send to failure");
                    exit(EXIT_FAILURE);
                }
                struct Game buffer;
                if (!read(udpfd, &buffer, sizeof(buffer))) {
                    perror("receiving failure");
                    exit(EXIT_FAILURE);
                }
                alarm(0);
            } else {
                game.turn = game.turn % client_number + 1;
                game.x1 = 100;
                game.y1 = 100;
                game.x2 = 100;
                game.y2 = 100;
                int opt_val = 1;
                int opt_len = sizeof(opt_val);
                if (setsockopt(udpfd, SOL_SOCKET, SO_BROADCAST, (void *) &opt_val, opt_len)) {
                    perror("sesocketot() failure");
                    exit(EXIT_FAILURE);
                }
                if (!sendto(udpfd, &game, sizeof(game), 0, (struct sockaddr *) &udp_addr, sizeof(udp_addr))) {
                    perror("send to failure");
                    exit(EXIT_FAILURE);
                }
                struct Game buffer;
                if (!read(udpfd, &buffer, sizeof(buffer))) {
                    perror("receiving failure");
                    exit(EXIT_FAILURE);
                }
            }
            system("clear");
            draw_matrix(client_number);
        } else {
            bytes_written = write(STDOUT_FILENO, "waiting for receiving message ... \n",
                                  strlen("waiting for receiving message ... \n"));
            socklen_t udp_addr_size = sizeof(udp_addr);
            int opt_val = 1;
            int opt_len = sizeof(opt_val);
            if (setsockopt(udpfd, IPPROTO_IP, IP_MULTICAST_LOOP, (void *) &opt_val, opt_len)) {
                perror("sesocketot() failure");
                exit(EXIT_FAILURE);
            }
            struct Game buffer;
            if (!read(udpfd, &buffer, sizeof(buffer))) {
                perror("receiving failure");
                exit(EXIT_FAILURE);
            }
            if (game.turn != buffer.turn) {
                game.turn = buffer.turn;
            }
            if (buffer.x1 != 100 && buffer.x2 != 100 && buffer.y1 != 100 && buffer.y2 != 100) {
                game.x1 = buffer.x1;
                game.x2 = buffer.x2;
                game.y1 = buffer.y1;
                game.y2 = buffer.y2;
                game.turn = buffer.turn;
                game.step = buffer.step;
                matrix[game.x1][game.y1][game.x2][game.y2] = 1;
                matrix[game.x2][game.y2][game.x1][game.y1] = 1;
                if (game.turn == 1)
                    game.point[client_number] = buffer.point[client_number];
                else
                    game.point[game.turn - 1] = buffer.point[game.turn - 1];
            }
            system("clear");
            draw_matrix(client_number);
        }
//        printf("point number one: %d, point number two: %d", game.point[1], game.point[2]);
    }
    // Final Points
    int maximum_point = 0, max_idx = 0;
    for (int i = 1; i <= client_number; i++) {
        if (game.point[i] > maximum_point) {
            maximum_point = game.point[i];
            max_idx = i;
        }
    }
    game.winner = max_idx;
    // send final winner to all clients in group
    if (!sendto(udpfd, &game, sizeof(game), 0, (struct sockaddr *) &udp_addr, sizeof(udp_addr))){
        perror("failing to send winner");
        exit(EXIT_FAILURE);
    }
    struct Game buffer;
    if (!read(udpfd, &buffer, sizeof(buffer))){
        perror("failing to receive winner");
        exit(EXIT_FAILURE);
    }
    printf("winner is number: %d\n", game.winner);
    if (close(udpfd) < 0) {
        perror("closing socket failure");
        exit(EXIT_FAILURE);
    }

//    printf("%d \n", game.point[my_turn]);
    return 0;
}