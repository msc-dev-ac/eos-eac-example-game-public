#pragma once
#include <netinet/in.h>
#include <unistd.h>

typedef void (*CommsConnectionCallback)(int socket);
typedef void (*CommsDataCallback)(int socket, void *data, ssize_t data_size);

typedef struct comms_data {
    int socket;
    void * data;
    ssize_t size;
} comms_data_t;

typedef struct client_thread_args {
    int socket;
    struct sockaddr_in addr;
    CommsDataCallback onData;
} client_thread_args_t;

typedef struct server_thread_args {
    CommsDataCallback onData;
    CommsConnectionCallback onConnection;
} server_thread_args_t;

// Startup a thread to handle incoming connections, spawning a thread to handle
// each request.  Callbacks registered to fire on new connection and on receipt
// of data from a client socket.  Returns true on success.
bool comms_start(CommsConnectionCallback onConnection,
                 CommsDataCallback onData);

// Send the given data to the client socket provided over the previously started
// comms channel. Returns true on success.
bool comms_send_to(int const socket, void const *const data,
                   ssize_t const data_size);

// Send the given data prefixed with a null terminated name string to the client
// socket provided over the previously started comms channel. Returns true on
// success.
bool comms_send_named_to(int const socket, char const *const name,
                         void const *const data, ssize_t const data_size);