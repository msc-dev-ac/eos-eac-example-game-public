#pragma once

typedef void (*GameplaySendHelloCallback)();

// Startup a thread to handle when to send hello messages to the server.
bool game_start(GameplaySendHelloCallback sendHello);