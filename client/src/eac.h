#pragma once
#include <eos_sdk.h>

// Initialize EAC, print logging to stdout, get a handle to the anti cheat
// interface and register a callback for sending messages to the eac server.
EOS_EResult eac_init(EOS_HPlatform *hPlatform, EOS_HAntiCheatClient *hAntiCheat,
                     EOS_AntiCheatClient_OnMessageToServerCallback onMessage);

// Begin the authentication flow to eventually fill the EOS_ProductUserId ptr
// given.  This function internally uses callbacks and returns prior to the user
// begin set.  Ensure EOS_Platform_Tick is being called regularly to fire
// callbacks as needed.
EOS_EResult eac_auth(EOS_HPlatform hPlatform, EOS_ProductUserId *user);

// Start a EAC protected session with the given local PUID.
EOS_EResult eac_begin(EOS_HAntiCheatClient hAntiCheat, EOS_ProductUserId user);

EOS_EResult eac_end(EOS_HAntiCheatClient hAntiCheat);