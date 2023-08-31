#include "comms.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int server_socket = -1;

void *client_socket_reader(void *args) {
    auto socket = ((client_thread_args_t *)args)->socket;
    auto addr = ((client_thread_args_t *)args)->addr;
    auto onData = ((client_thread_args_t *)args)->onData;
    free(args);

    while (true) {
        ssize_t data_size = 0;
        ssize_t len = read(socket, &data_size, sizeof(data_size));
        if (0 == len) {
            return NULL;
        }

        if (sizeof(data_size) != len) {
            printf("[WARN] %s\n",
                   "Received incomplete size packet from client socket");
            return NULL;
        }

        void *data = malloc(data_size);
        if (nullptr == data) {
            printf("[WARN] %s\n", "Failed to malloc data received from socket");
            return NULL;
        }

        ssize_t data_len = read(socket, data, data_size);
        if (data_len != data_size) {
            printf("[WARN] %s\n",
                   "Received incomplete data packet from client socket");
            free(data);
            return NULL;
        }

        onData(socket, data, data_size);
    }
    return NULL;
}

void *comms_run(void *args) {
    auto server_args = (server_thread_args_t *)args;
    auto onConnection = server_args->onConnection;
    auto onData = server_args->onData;
    free(server_args);

    while (true) {

        auto thread_args =
            (client_thread_args_t *)malloc(sizeof(client_thread_args_t));
        if (nullptr == thread_args) {
            printf("[ERROR] %s\n", "Failed to malloc for thread args.");
            continue;
        }

        uint32_t addr_len = sizeof(sockaddr_in);
        int client_socket = -1;
        if (-1 == (client_socket = (accept(
                       server_socket, (struct sockaddr *)&thread_args->addr,
                       &addr_len)))) {
            printf("[ERROR] %s\n", "Failed to accept connection on socket.");
            continue;
        }

        thread_args->socket = client_socket;
        thread_args->onData = onData;
        onConnection(client_socket);

        pthread_t client_thread;
        if (0 != pthread_create(&client_thread, NULL, client_socket_reader,
                                thread_args)) {
            printf("[ERROR] %s\n",
                   "Failed to create thread for client socket.");
            continue;
        }
    }
}

bool comms_start(CommsConnectionCallback onConnection,
                 CommsDataCallback onData) {

    bool result = false;

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(1234);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    server_thread_args_t *args = NULL;
    pthread_t server_thread;
    int optval = 1;

    do { // while(false)

        args = (server_thread_args_t *)malloc(sizeof(server_thread_args_t));
        if (nullptr == args) {
            printf("[ERROR] %s\n", "Failed to malloc server thread arguments");
            break;
        }

        args->onData = onData;
        args->onConnection = onConnection;

        if (0 > (server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))) {
            printf("[ERROR] %s\n", "Failed to create socket.");
            break;
        }

        if (0 > setsockopt(server_socket, SOL_SOCKET,
                           SO_REUSEADDR | SO_REUSEPORT, &optval,
                           sizeof(optval))) {
            printf("[ERROR] %s\n", "Failed to set socket options.");
            break;
        }

        if (0 > (bind(server_socket, (struct sockaddr *)&server_addr,
                      sizeof(server_addr)))) {
            printf("[ERROR] %s\n", "Failed to bind socket.");
            break;
        }

        if (0 != (listen(server_socket, 5))) {
            printf("[ERROR] %s\n", "Failed to listen on socket.");
            break;
        }

        if (0 != pthread_create(&server_thread, NULL, comms_run, args)) {
            printf("[ERROR] %s\n", "Failed to create thread for server.");
            break;
        }

        result = true;

    } while (false);

    // clean up
    if (!result) {
        pthread_kill(server_thread, SIGKILL);

        if (0 > server_socket) {
            close(server_socket);
        }

        if (NULL != args) {
            free(args);
        }
    }

    return result;
}

bool comms_send_to(int socket, void const *data, ssize_t data_size) {

    if ((-1 == fcntl(socket, F_GETFL)) && (EBADF == errno)) {
        printf("[WARN] %s\n", "Bad client socket file descriptor");
        return false;
    }

    void *packet = malloc(sizeof(ssize_t) + data_size);
    if (nullptr == packet) {
        printf("[ERROR] %s\n", "Failed to malloc packet to send.");
        return false;
    }

    auto ptr = (char *)packet;
    memcpy(ptr, &data_size, sizeof(ssize_t));

    ptr += sizeof(ssize_t);
    memcpy(ptr, data, data_size);

    ssize_t len = send(socket, packet, sizeof(ssize_t) + data_size, 0);
    if (len != sizeof(ssize_t) + data_size) {
        printf("[WARN] %s\n", "Sent incomplete packet to client socket");
        free(packet);
        return false;
    }

    free(packet);
    return true;
}

bool comms_send_named_to(int socket, char const *const name,
                         void const *const data, ssize_t const data_size) {

    ssize_t packet_len = strlen(name) + 1 + data_size;

    void *packet = malloc(packet_len);
    if (nullptr == packet) {
        printf("[FAIL] %s\n", "Failed to malloc for msg->client");
        return false;
    }

    char *ptr = (char *)packet;
    memcpy(ptr, name, strlen(name) + 1);

    ptr += strlen(name) + 1;
    memcpy(ptr, data, data_size);

    auto result = comms_send_to(socket, packet, packet_len);
    free(packet);

    return result;
}