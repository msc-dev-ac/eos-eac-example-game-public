#pragma once
#include <eos_sdk.h>

// Initialize EAC, print logging to stdout, get a handle to the anti cheat
// interface and register callbacks for sending messages/actions to the eac
// clients.
EOS_EResult
eac_init(EOS_HPlatform *hPlatform, EOS_HAntiCheatServer *hAntiCheat,
         EOS_AntiCheatServer_OnMessageToClientCallback onMessage,
         EOS_AntiCheatServer_OnClientActionRequiredCallback onAction);

// Register a client to the current EAC session using the client PUID.
EOS_EResult eac_add_client(EOS_HAntiCheatServer hAntiCheat,
                           EOS_ProductUserId id);

EOS_EResult eac_end(EOS_HAntiCheatServer hAntiCheat);