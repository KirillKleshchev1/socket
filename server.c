#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>

#define START_SERVER "Start server\n"
#define START_LISTENING "Server start listening at %s\n"
#define ACCEPT_CLIENT "Accept client %d, client sbrk %ld\n"
#define RECEIVE_FROM_CLIENT "Received from client: %d. Current server state = %d\n"
#define INFO_SERVER_NAME "Server name = %s\n"
#define SERVER_LOG_FILENAME "/tmp/server.log"
#define MAX_CLIENTS 126
#define SOCK_BUFFER_SIZE 64
#define MAX_LINE_LENGTH 1024

static char buffer_global[MAX_LINE_LENGTH * 2];
static FILE *log_file = NULL;
static int STATE = 0;

static char* get_socket_name(const char *config) {
    static char sock_name[MAX_LINE_LENGTH];
    FILE *config_file = fopen(config, "r");
    if (config_file == NULL) {
        perror("Failed to open config file");
        exit(EXIT_FAILURE);
    }

    if (!fgets(sock_name, MAX_LINE_LENGTH, config_file)) {
        fclose(config_file);
        perror("Failed to read from config file");
        exit(EXIT_FAILURE);
    }
    fclose(config_file);

    sock_name[strcspn(sock_name, "\n\r")] = '\0';
    return strdup(sock_name);
}

static void write_log(const char *msg) {
    fputs(msg, log_file);
    fflush(log_file);
}

static int setup_server(const char *server_name) {
    unlink(server_name);

    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un address = {
        .sun_family = AF_UNIX,
    };
    strncpy(address.sun_path, server_name, sizeof(address.sun_path) - 1);

    if (bind(sock_fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        perror("Failed to bind address");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    return sock_fd;
}

static void accept_client(int server_fd, int clients[]) {
    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd == -1) {
        perror("Failed to accept client");
        return;
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == 0) {
            clients[i] = client_fd;
            snprintf(buffer_global, sizeof(buffer_global), ACCEPT_CLIENT, client_fd, (long)sbrk(0));
            write_log(buffer_global);
            return;
        }
    }
    close(client_fd);
}

static int handle_client(int client_fd) {
    char sock_buffer[SOCK_BUFFER_SIZE] = {0};
    ssize_t recv_len = recv(client_fd, sock_buffer, sizeof(sock_buffer) - 1, 0);

    if (recv_len <= 0) {
        return -1;
    }

    int number = atoi(sock_buffer);
    STATE += number;

    snprintf(sock_buffer, sizeof(sock_buffer), "%d", STATE);
    if (send(client_fd, sock_buffer, strlen(sock_buffer) + 1, 0) <= 0) {
        perror("Failed to send to client");
        return -1;
    }

    snprintf(buffer_global, sizeof(buffer_global), RECEIVE_FROM_CLIENT, number, STATE);
    write_log(buffer_global);
    return 0;
}

static void run_server(int server_fd, const char *server_address) {
    fd_set read_fds;
    int clients[MAX_CLIENTS] = {0};
    int max_fd = server_fd;

    listen(server_fd, MAX_CLIENTS);
    snprintf(buffer_global, sizeof(buffer_global), START_LISTENING, server_address);
    write_log(buffer_global);

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] > 0) {
                FD_SET(clients[i], &read_fds);
                if (clients[i] > max_fd) {
                    max_fd = clients[i];
                }
            }
        }

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("Error in select");
            continue;
        }

        if (FD_ISSET(server_fd, &read_fds)) {
            accept_client(server_fd, clients);
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] > 0 && FD_ISSET(clients[i], &read_fds)) {
                if (handle_client(clients[i]) < 0) {
                    close(clients[i]);
                    clients[i] = 0;
                }
            }
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <config_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *socket_name = get_socket_name(argv[1]);
    log_file = fopen(SERVER_LOG_FILENAME, "w");
    if (!log_file) {
        perror("Failed to open log file");
        free(socket_name);
        return EXIT_FAILURE;
    }

    snprintf(buffer_global, sizeof(buffer_global), INFO_SERVER_NAME, socket_name);
    write_log(buffer_global);

    int sock_fd = setup_server(socket_name);
    if (sock_fd == -1) {
        write_log("Failed to create server");
        fclose(log_file);
        free(socket_name);
        return EXIT_FAILURE;
    }

    write_log(START_SERVER);
    run_server(sock_fd, socket_name);

    close(sock_fd);
    fclose(log_file);
    free(socket_name);
    return EXIT_SUCCESS;
}
