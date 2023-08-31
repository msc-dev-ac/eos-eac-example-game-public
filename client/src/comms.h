#pragma once
#include <unistd.h>

typedef void (*CommsDataCallback)(void *data, ssize_t data_size);

typedef struct comms_data {
    void * data;
    ssize_t size;
} comms_data_t;

// Startup a TCP stream with the server and create a thread to read data from
// the socket calling the given callback when a message is received. Returns
// true on success. The buffer passed to the callback should be freed after use.
bool comms_start(CommsDataCallback onData, char* server_ip, int server_port);

// Send the given data to the server over the previously started comms channel.
// Returns true on success.
bool comms_send(void const *const data, ssize_t const data_size);

// Send the given data prefixed with a null terminated name string to the server
// over the previously started comms channel.  Returns true on success.
bool comms_send_named(char const *const name, void const *const data,
                      ssize_t const data_size);
