#include "comms.h"
#include "eac.h"
#include "game.h"

#include <stdio.h>
#include <stdlib.h>
#include <eos_sdk.h>
#include <pthread.h>
#include <string.h>
#include <eos_anticheatclient.h>
#include <signal.h>
#include <vector>

EOS_HPlatform hPlatform = NULL;
EOS_HAntiCheatClient hAntiCheat = NULL;

// last received data packets from server for processing
std::vector<comms_data_t> comms_data;
pthread_mutex_t comms_data_m = PTHREAD_MUTEX_INITIALIZER;

// optional eac comms traffic logging file
FILE * log_file = NULL;

// cleanup on ctrl-c
bool stop = false;
void sig_handler(int sig) { stop = true; }

// hello message contents
char msg[] = "HELLO";

// callback for when the eac client wants to send data to the eac server.
void eac_on_send_msg_to_server(
    const EOS_AntiCheatClient_OnMessageToServerCallbackInfo *Data) {
    // printf("[SEND->SERVER] %d bytes\n", Data->MessageDataSizeBytes);

    if(NULL != log_file) {
        fprintf(log_file, "[C->S] (%dB) ", Data->MessageDataSizeBytes);
        for(int i = 0; i < Data->MessageDataSizeBytes; i++) {
            fprintf(log_file, "%02x", *((uint8_t*)Data->MessageData+i));
        }
        fputs("\n", log_file);
    }

    char prefix[] = "MSG";
    if(!comms_send_named(prefix, Data->MessageData, Data->MessageDataSizeBytes)) {
        printf("[FAIL] %s\n", "Failed to send EAC message to server");
    }
}

// callback for when a packet is received from the server via the comms channel
void on_comms_recv_data(void *data, ssize_t data_size) {
    pthread_mutex_lock(&comms_data_m);
    comms_data_t comm = {};
    comm.data = data;
    comm.size = data_size;
    comms_data.push_back(comm);
    pthread_mutex_unlock(&comms_data_m);
}

// send msg to server with the client userid to be registered with eac server
void send_userid(EOS_ProductUserId user) {

    char buffer[EOS_PRODUCTUSERID_MAX_LENGTH + 1];
    int32_t size = sizeof(buffer);
    if (EOS_EResult::EOS_Success !=
        EOS_ProductUserId_ToString(user, buffer, &size)) {
        printf("[FAIL] %s\n", "Failed to convert the local PoductUserId to "
                              "string for network transmission");
        exit(1);
    }

    char prefix[] = "AUTH";
    if(!comms_send_named(prefix, buffer, size)) {
        printf("[FAIL] %s\n", "Failed to send user ID to server");
    }
}

// send a 'hello' message to the server
void send_hello() {
    char prefix[] = "GAME";
    if(!comms_send_named(prefix, msg, sizeof(msg))) {
        printf("[FAIL] %s\n", "Failed to send hello message to server");
    }
}

// handle a comms message received from the server
void handle_comms_msg(comms_data_t comm) {
    char prefix_MSG[] = "MSG";

    if (0 == memcmp(prefix_MSG, comm.data, sizeof(prefix_MSG))) {
        char *ptr = (char *)comm.data + sizeof(prefix_MSG);
        ssize_t size = comm.size - sizeof(prefix_MSG);

        // printf("[RECV->CLIENT] %zd bytes\n", size);
        if(NULL != log_file) {
            fprintf(log_file, "[S->C] (%zdB) ", size);
            for(int i = 0; i < size; i++) {
                fprintf(log_file, "%02x", *(uint8_t*)(ptr+i));
            }
            fputs("\n", log_file);
        }

        EOS_AntiCheatClient_ReceiveMessageFromServerOptions RecvOptions =
            {};
        RecvOptions.Data = ptr;
        RecvOptions.DataLengthBytes = size;
        RecvOptions.ApiVersion =
            EOS_ANTICHEATCLIENT_RECEIVEMESSAGEFROMSERVER_API_LATEST;

        if (EOS_EResult::EOS_Success !=
            EOS_AntiCheatClient_ReceiveMessageFromServer(hAntiCheat,
                                                            &RecvOptions)) {
            printf(
                "[FAIL] %s\n",
                "Failed to parse AntiCheat message received from server");
            return;
        }

        return;
    }

    
}

int main(int argc, char** argv) {
    signal(SIGINT, sig_handler);

    // check command line args
    if(3 > argc) {
        printf("Usage: %s server_ip server_port\n", argv[0]);
        exit(1);
    }
    char * server_ip = argv[1];
    int server_port = atoi(argv[2]);

    // file to log EAC comms messages to
    log_file = fopen("/tmp/eac-comms.log", "w");

    // setup eos and eac
    if (EOS_EResult::EOS_Success !=
        eac_init(&hPlatform, &hAntiCheat, eac_on_send_msg_to_server)) {
        exit(1);
    }

    // connect to server for network comms
    if(!comms_start(&on_comms_recv_data, server_ip, server_port)) {
        printf("[FAIL] %s\n", "Failed to start comms channel");
        exit(1);
    }

    EOS_ProductUserId user = NULL;
    bool started_login = false;
    bool sent_userid = false;
    bool game_started = false;

    // game loop
    while (!stop) {
        EOS_Platform_Tick(hPlatform);

        // start eos login
        if (!started_login) {
            eac_auth(hPlatform, &user);
            started_login = true;
        }

        // check for finished login
        if (NULL == user) {
            continue;
        }

        // send the user id to server to be registered and start local session
        if (!sent_userid) {
            send_userid(user);
            eac_begin(hAntiCheat, user);
            sent_userid = true;
        }

        // start game
        if(!game_started) {
            if(!game_start(&send_hello)) {
                printf("[FAIL] %s\n", "Failed to start game");
                exit(1);
            }
            game_started = true;
        }

        // handle messages from server
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
