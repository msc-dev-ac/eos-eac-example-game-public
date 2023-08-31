#include "comms.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int client_socket = -1;
pthread_t reader_thread;

void *socket_reader(void *args) {
    CommsDataCallback onData = (CommsDataCallback)args;
    while (true) {
        ssize_t data_size = 0;
        ssize_t len = read(client_socket, &data_size, sizeof(data_size));
        if (0 == len) {
            return NULL;
        }

        if (sizeof(data_size) != len) {
            printf("[WARN] %s\n",
                   "Received incomplete size packet from server socket");
            return NULL;
        }
        void *data = malloc(data_size);
        if (nullptr == data) {
            printf("[WARN] %s\n",
                   "Failed to malloc data received from server socket");
            return NULL;
        }

        ssize_t data_len = read(client_socket, data, data_size);
        if (data_len != data_size) {
            printf("[WARN] %s\n",
                   "Received incomplete data packet from server socket");
            free(data);
            return NULL;
        }

        onData(data, data_size);
    }
    return NULL;
}

bool comms_start(CommsDataCallback onData, char* server_ip, int server_port) {
    bool result = false;

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    do { // while(false)

        if (0 > (client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))) {
            printf("[ERROR] %s\n", "Failed to create socket.");
            result = false;
            break;
        }

        if (0 != pthread_create(&reader_thread, NULL, socket_reader,
                                (void *)onData)) {
            printf("[ERROR] %s\n", "Failed to create socket reader thread");
            result = false;
            break;
        }

        if (0 >= inet_pton(AF_INET, server_ip, &server_addr.sin_addr)) {
            printf("[ERROR] %s\n",
                   "Failed to convert IP address to binary network format.");
            result = false;
            break;
            ;
        }

        if (0 > connect(client_socket, (struct sockaddr *)&server_addr,
                        sizeof(server_addr))) {
            printf("[ERROR] %s\n", "Failed to connect to server.");
            result = false;
            break;
        }

        result = true;

    } while (false);

    // clean up
    if (!result) {
        pthread_kill(reader_thread, SIGKILL);

        if (0 > client_socket) {
            close(client_socket);
        }
    }

    return result;
}

bool comms_send(void const *const data, ssize_t const data_size) {

    void *packet = malloc(sizeof(ssize_t) + data_size);
    if (nullptr == packet) {
        printf("[ERROR] %s\n", "Failed to malloc packet to send.");
        return false;
    }

    char *ptr = (char *)packet;
    memcpy(ptr, &data_size, sizeof(ssize_t));

    ptr += sizeof(ssize_t);
    memcpy(ptr, data, data_size);

    ssize_t len = send(client_socket, packet, sizeof(ssize_t) + data_size, 0);
    if (len != sizeof(ssize_t) + data_size) {
        printf("[WARN] %s\n", "Sent incomplete packet to server socket");
        free(packet);
        return false;
    }

    free(packet);
    return true;
}

bool comms_send_named(char const *const name, void const *const data,
                      ssize_t const data_size) {
                        
    ssize_t packet_len = strlen(name) + 1 + data_size;
    
    void *packet = malloc(packet_len);
    if (nullptr == packet) {
        printf("[FAIL] %s\n", "Failed to malloc for msg->server");
        return false;
    }

    char *ptr = (char *)packet;
    memcpy(ptr, name, strlen(name) + 1);

    ptr += strlen(name) + 1;
    memcpy(ptr, data, data_size);

    auto result = comms_send(packet, packet_len);
    free(packet);
    
    return result;
}