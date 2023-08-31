#include "game.h"

#include <cstdio>
#include <pthread.h>

pthread_t game_thread;

void *game_run(void* args) {
    GameplaySendHelloCallback sendHello = (GameplaySendHelloCallback)args;

    while(true) {
        printf("%s","Press any key to send HELLO to the server...\n");
        getc(stdin);
        sendHello();
    }
}

bool game_start(GameplaySendHelloCallback sendHello) {
    if (0 != pthread_create(&game_thread, NULL, game_run, (void *)sendHello)) {
        printf("[ERROR] %s\n", "Failed to create gameplay thread");
        return false;
    }
    return true;
}

