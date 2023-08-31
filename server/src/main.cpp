#include "comms.h"
#include "eac.h"

#include <stdio.h>
#include <stdlib.h>
#include <eos_sdk.h>
#include <signal.h>
#include <eos_anticheatserver.h>
#include <map>
#include <pthread.h>
#include <string.h>
#include <vector>

EOS_HPlatform hPlatform = NULL;
EOS_HAntiCheatServer hAntiCheat = NULL;

// last received data packets from clients for processing
std::vector<comms_data_t> comms_data;
pthread_mutex_t comms_data_m = PTHREAD_MUTEX_INITIALIZER;

// two way mapping between client socket and EAC ClientHandle (PUID)
std::map<EOS_ProductUserId, int> clientHandleToSocket;
std::map<int, EOS_ProductUserId> socketToClientHandle;

// cleanup on ctrl-c
bool stop = false;
void sig_handler(int sig) { stop = true; }

// callback for when the eac server wants to send data to an eac client.
void eac_on_send_msg_to_client(
    const EOS_AntiCheatCommon_OnMessageToClientCallbackInfo *Data) {
    // printf("[SEND->CLIENT] %d bytes\n", Data->MessageDataSizeBytes);

    // Data->ClientHandle
    int socket = clientHandleToSocket[(EOS_ProductUserId)Data->ClientHandle];
    char prefix[] = "MSG";
    if (!comms_send_named_to(socket, prefix, Data->MessageData,
                             Data->MessageDataSizeBytes)) {
        printf("[FAIL] %s\n", "Failed to send EAC message to client");
    }
}

// callback for when the eac server wants a client to perform some action.
void eac_on_send_action_to_client(
    const EOS_AntiCheatCommon_OnClientActionRequiredCallbackInfo *Data) {
    printf("[ACTION->CLIENT] %s\n", Data->ActionReasonDetailsString);
}

// callback for when a new connection is made to the comms server
void on_comms_new_connection(int socket) {
    printf("[COMMS] New client socket: %d\n", socket);
}

// callback for when a packet is received from a client via the comms channel.
void on_comms_recv_data(int socket, void *data, ssize_t data_size) {
    pthread_mutex_lock(&comms_data_m);
    comms_data_t comm;
    comm.data = data;
    comm.size = data_size;
    comm.socket = socket;
    comms_data.push_back(comm);
    pthread_mutex_unlock(&comms_data_m);
}

// handle latest comms message received from a client
void handle_comms_msg(comms_data_t comm) {
    char prefix_AUTH[] = "AUTH";
    char prefix_MSG[] = "MSG";
    char prefix_GAME[] = "GAME";

    if (0 == memcmp(prefix_AUTH, comm.data, sizeof(prefix_AUTH))) {
        char *ptr = (char *)comm.data + sizeof(prefix_MSG);

        auto client_id = EOS_ProductUserId_FromString(ptr);
        clientHandleToSocket[client_id] = comm.socket;
        socketToClientHandle[comm.socket] = client_id;
        eac_add_client(hAntiCheat, client_id);

        return;
    }
    if (0 == memcmp(prefix_MSG, comm.data, sizeof(prefix_MSG))) {
        char *ptr = (char *)comm.data + sizeof(prefix_MSG);
        ssize_t size = comm.size - sizeof(prefix_MSG);

        // printf("[RECV->SERVER] %zd bytes\n", size);

        EOS_AntiCheatServer_ReceiveMessageFromClientOptions RecvOptions =
            {};
        RecvOptions.Data = ptr;
        RecvOptions.DataLengthBytes = size;
        RecvOptions.ApiVersion =
            EOS_ANTICHEATCLIENT_RECEIVEMESSAGEFROMSERVER_API_LATEST;
        RecvOptions.ClientHandle = socketToClientHandle[comm.socket];

        if (EOS_EResult::EOS_Success !=
            EOS_AntiCheatServer_ReceiveMessageFromClient(hAntiCheat,
                                                            &RecvOptions)) {
            printf(
                "[FAIL] %s\n",
                "Failed to parse AntiCheat message received from client");
            return;
        }
        return;
    }
    if(0 == memcmp(prefix_GAME, comm.data, sizeof(prefix_GAME))) {
        char *ptr = (char *)comm.data + sizeof(prefix_GAME);
        ssize_t size = comm.size - sizeof(prefix_GAME);

        printf("[GAME] Received %s\n", ptr);
    }
}

int main() {
    signal(SIGINT, sig_handler);

    // setup eos and eac
    if (EOS_EResult::EOS_Success != eac_init(&hPlatform, &hAntiCheat,
                                             eac_on_send_msg_to_client,
                                             eac_on_send_action_to_client)) {
        exit(1);
    }

    if (!comms_start(on_comms_new_connection, on_comms_recv_data)) {
        printf("[FAIL] %s\n", "Failed to start comms server");
        exit(1);
    }

    // game loop
    while (!stop) {
        EOS_Platform_Tick(hPlatform);

        // handle messages from clients
        pthread_mutex_lock(&comms_data_m);
        for(auto comm : comms_data) {
            handle_comms_msg(comm);
            free(comm.data);
        }
        comms_data.clear();
        pthread_mutex_unlock(&comms_data_m);

        // sleep 0.1s
        usleep(100000);
    }

    // cleanup
    eac_end(hAntiCheat);
}
