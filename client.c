#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define BUFFER_SIZE 64
#define MAX_CONFIG_LINE_LENGTH 1024
#define CLIENT_INFO_FORMAT "Client (id = %d) with delay = %f\n"
#define SERVER_NAME_FORMAT "Server name = %s\n"

char* get_server_socket_path(char config_path[]) {
    static char server_socket_path[MAX_CONFIG_LINE_LENGTH];
    FILE *config_file = fopen(config_path, "r");
    fgets(server_socket_path, MAX_CONFIG_LINE_LENGTH, config_file);
    server_socket_path[strcspn(server_socket_path, "\n\r")] = '\0';
    fclose(config_file);
    return server_socket_path;
}

int get_random_value(int min, int max) {
    return (rand() % (max - min + 1)) + min;
}

int main(int argc, char** argv) {
    int client_id = atoi(argv[2]);
    int request_count = atoi(argv[3]);
    char* server_socket_path = get_server_socket_path(argv[1]);
    printf(SERVER_NAME_FORMAT, server_socket_path);

    srand(time(0));
    float delay_seconds = argc == 5 ? atof(argv[4]) : 0;
    float sleep_microseconds = delay_seconds ? delay_seconds * 1000000 : get_random_value(1, 255) * 10;
    printf(CLIENT_INFO_FORMAT, client_id, delay_seconds);

    struct sockaddr_un server_address = {0};
    server_address.sun_family = AF_UNIX;
    strcpy(server_address.sun_path, server_socket_path);

    int socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    connect(socket_fd, (struct sockaddr*) &server_address, sizeof(server_address));

    time_t start_time = time(0);

    for (int i = 0; i < request_count; i++) {
        char request_buffer[BUFFER_SIZE] = {0};
        char response_buffer[BUFFER_SIZE] = {0};

        scanf("%s", request_buffer);
        usleep(sleep_microseconds);
        write(socket_fd, request_buffer, strlen(request_buffer));
        read(socket_fd, response_buffer, BUFFER_SIZE);
    }

    time_t end_time = time(0);
    printf("client time: %f\n", difftime(end_time, start_time));

    close(socket_fd);
    return 0;
}
