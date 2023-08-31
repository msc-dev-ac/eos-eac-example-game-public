#include "eac.h"

#include <eos_anticheatserver.h>
#include <eos_logging.h>
#include <eos_sdk.h>
#include <set>
#include <stdio.h>
#include <stdlib.h>

std::set<EOS_ProductUserId> clients;

void logging_callback(const EOS_LogMessage *msgData) {
    if (msgData->Level != EOS_ELogLevel::EOS_LOG_Off) {
        if (msgData->Level == EOS_ELogLevel::EOS_LOG_Error) {
            printf("[ERROR] [%s] %s\n", msgData->Category, msgData->Message);
        } else if (msgData->Level == EOS_ELogLevel::EOS_LOG_Fatal) {
            printf("[FATAL] [%s] %s\n", msgData->Category, msgData->Message);
        } else if (msgData->Level == EOS_ELogLevel::EOS_LOG_Warning) {
            printf("[WARNING] [%s] %s\n", msgData->Category, msgData->Message);
        } else if (msgData->Level == EOS_ELogLevel::EOS_LOG_Info) {
            printf("[INFO] [%s] %s\n", msgData->Category, msgData->Message);
        } else if (msgData->Level == EOS_ELogLevel::EOS_LOG_Verbose) {
            printf("[VERBOSE] [%s] %s\n", msgData->Category, msgData->Message);
        } else if (msgData->Level == EOS_ELogLevel::EOS_LOG_VeryVerbose) {
            printf("[VERYVERBOSE] [%s] %s\n", msgData->Category,
                   msgData->Message);
        } else {
            printf("[-] [%s] %s\n", msgData->Category, msgData->Message);
        }
    }
}

void eac_auth_change_callback(
    const EOS_AntiCheatCommon_OnClientAuthStatusChangedCallbackInfo *Data) {
    printf("[AUTH STATUS CHANGE] status: %d\n", Data->ClientAuthStatus);
}

EOS_EResult
eac_init(EOS_HPlatform *hPlatform, EOS_HAntiCheatServer *hAntiCheat,
         EOS_AntiCheatServer_OnMessageToClientCallback onMessage,
         EOS_AntiCheatServer_OnClientActionRequiredCallback onAction) {

    auto result = EOS_EResult::EOS_Disabled;

    *hPlatform = NULL;
    *hAntiCheat = NULL;

    EOS_InitializeOptions SDKOptions = {};
    SDKOptions.ProductName = "TestGame";
    SDKOptions.ProductVersion = "1.0";
    SDKOptions.Reserved = NULL;
    SDKOptions.ReleaseMemoryFunction = NULL;
    SDKOptions.OverrideThreadAffinity = NULL;
    SDKOptions.AllocateMemoryFunction = NULL;
    SDKOptions.SystemInitializeOptions = NULL;
    SDKOptions.ReallocateMemoryFunction = NULL;
    SDKOptions.ApiVersion = EOS_INITIALIZE_API_LATEST;

    EOS_Platform_ClientCredentials Creds = {};
    Creds.ClientId = "xyza7891x0b6YVnUdx2TmJ1mNvIeX0c7";
    Creds.ClientSecret = "bLjUWPsZDgp/O0ez7LS1S21jIp1EBFKSvqKySBh6WYM";

    EOS_Platform_Options PlatformOptions = {};
    PlatformOptions.ClientCredentials = Creds;
    PlatformOptions.EncryptionKey =
        "1111111111111111111111111111111111111111111111111111111111111111";
    PlatformOptions.ProductId = "PRODUCT_ID_HERE";
    PlatformOptions.SandboxId = "SANDBOX_ID_HERE";
    PlatformOptions.DeploymentId = "DEPLOYMENT_ID_HERE";
    PlatformOptions.CacheDirectory = "/tmp/my_eos_cache/";
    PlatformOptions.bIsServer = true;
    PlatformOptions.Flags = 0;
    PlatformOptions.Reserved = NULL;
    PlatformOptions.RTCOptions = NULL;
    PlatformOptions.OverrideLocaleCode = NULL;
    PlatformOptions.OverrideCountryCode = NULL;
    PlatformOptions.TickBudgetInMilliseconds = 0;
    PlatformOptions.IntegratedPlatformOptionsContainerHandle = NULL;
    PlatformOptions.ApiVersion = EOS_PLATFORM_OPTIONS_API_LATEST;

    EOS_AntiCheatServer_BeginSessionOptions SessionOptions = {};
    SessionOptions.ServerName = "Test Game Server";
    SessionOptions.RegisterTimeoutSeconds = 60;
    SessionOptions.LocalUserId = NULL;
    SessionOptions.bEnableGameplayData = false;
    SessionOptions.ApiVersion = EOS_ANTICHEATSERVER_BEGINSESSION_API_LATEST;

    EOS_AntiCheatServer_AddNotifyMessageToClientOptions MessageToClientOptions =
        {};
    MessageToClientOptions.ApiVersion =
        EOS_ANTICHEATSERVER_ADDNOTIFYMESSAGETOCLIENT_API_LATEST;

    EOS_AntiCheatServer_AddNotifyClientActionRequiredOptions
        ActionRequiredOptions = {};
    ActionRequiredOptions.ApiVersion =
        EOS_ANTICHEATSERVER_ADDNOTIFYCLIENTACTIONREQUIRED_API_LATEST;

    EOS_AntiCheatServer_AddNotifyClientAuthStatusChangedOptions
        AuthChangedOptions = {};
    AuthChangedOptions.ApiVersion =
        EOS_ANTICHEATSERVER_ADDNOTIFYCLIENTAUTHSTATUSCHANGED_API_LATEST;

    do { // while(false)

        if (EOS_EResult::EOS_Success !=
            (result = EOS_Initialize(&SDKOptions))) {
            printf("%s\n", "[FAIL] Failed to initialise EOS");
            break;
        }

        if (EOS_EResult::EOS_Success !=
            (result = EOS_Logging_SetCallback(&logging_callback))) {
            printf("%s\n", "[FAIL] Failed to set EOS logging callback");
            break;
        }

        if (EOS_EResult::EOS_Success !=
            (result = EOS_Logging_SetLogLevel(
                 EOS_ELogCategory::EOS_LC_ALL_CATEGORIES,
                 EOS_ELogLevel::EOS_LOG_Info))) {
            printf("%s\n", "[FAIL] Failed to set EOS log level");
            break;
        }

        if (NULL == (*hPlatform = EOS_Platform_Create(&PlatformOptions))) {
            printf(
                "%s\n",
                "[FAIL] Failed to get a handle to the EOS Platform Interface");
            result = EOS_EResult::EOS_InvalidState;
            break;
        }

        if (NULL == (*hAntiCheat = EOS_Platform_GetAntiCheatServerInterface(
                         *hPlatform))) {
            printf(
                "%s\n",
                "[FAIL] Failed to get a handle to the EOS AntiCheat Interface");
            result = EOS_EResult::EOS_InvalidState;
            break;
        }

        // register anti cheat session callbacks

        if (EOS_INVALID_NOTIFICATIONID ==
            EOS_AntiCheatServer_AddNotifyMessageToClient(
                *hAntiCheat, &MessageToClientOptions, nullptr, onMessage)) {
            printf("%s\n",
                   "[FAIL] Failed to register MessageToClient callback");
            result = EOS_EResult::EOS_InvalidState;
            break;
        }

        if (EOS_INVALID_NOTIFICATIONID ==
            EOS_AntiCheatServer_AddNotifyClientActionRequired(
                *hAntiCheat, &ActionRequiredOptions, NULL, onAction)) {
            printf("%s\n",
                   "[FAIL] Failed to register ClientActionRequired callback");
            result = EOS_EResult::EOS_InvalidState;
            break;
        }

        if (EOS_INVALID_NOTIFICATIONID ==
            EOS_AntiCheatServer_AddNotifyClientAuthStatusChanged(
                *hAntiCheat, &AuthChangedOptions, NULL,
                eac_auth_change_callback)) {
            printf(
                "%s\n",
                "[FAIL] Failed to register ClientAuthStatusChanged callback");
            result = EOS_EResult::EOS_InvalidState;
            break;
        }

        // begin the game session

        if (EOS_EResult::EOS_Success !=
            (result = EOS_AntiCheatServer_BeginSession(*hAntiCheat,
                                                       &SessionOptions))) {
            printf("%s\n", "[FAIL] Failed to start the EAC session.");
            break;
        }

    } while (false);

    // clean up on error
    if (EOS_EResult::EOS_Success != result) {
        if (NULL != *hPlatform) {
            EOS_Platform_Release(*hPlatform);
        }
    }

    return result;
}

EOS_EResult eac_add_client(EOS_HAntiCheatServer hAntiCheat,
                           EOS_ProductUserId id) {

    EOS_AntiCheatServer_RegisterClientOptions Options = {};
    Options.ClientHandle = id;
    Options.ClientType =
        EOS_EAntiCheatCommonClientType::EOS_ACCCT_ProtectedClient;
    Options.ClientPlatform =
        EOS_EAntiCheatCommonClientPlatform::EOS_ACCCP_Linux;
    Options.UserId = id;
    Options.ApiVersion = EOS_ANTICHEATSERVER_REGISTERCLIENT_API_LATEST;

    auto result = EOS_AntiCheatServer_RegisterClient(hAntiCheat, &Options);
    if (EOS_EResult::EOS_Success == result) {
        clients.insert(id);
    }
    return result;
}

EOS_EResult eac_end(EOS_HAntiCheatServer hAntiCheat) {
    auto result = EOS_EResult::EOS_InvalidParameters;

    // unregister all clients
    for (auto client : clients) {

        EOS_AntiCheatServer_UnregisterClientOptions Options = {};
        Options.ClientHandle = client;
        Options.ApiVersion = EOS_ANTICHEATSERVER_UNREGISTERCLIENT_API_LATEST;

        if (EOS_EResult::EOS_Success !=
            (result =
                 EOS_AntiCheatServer_UnregisterClient(hAntiCheat, &Options))) {
            printf("[FAIL] %s\n", "Failed to unregister client");
            return result;
        }
    }

    EOS_AntiCheatServer_EndSessionOptions EndOptions = {};
    EndOptions.ApiVersion = EOS_ANTICHEATSERVER_ENDSESSION_API_LATEST;

    if (EOS_EResult::EOS_Success !=
        (result = EOS_AntiCheatServer_EndSession(hAntiCheat, &EndOptions))) {
        printf("[FAIL] %s\n", "Failed to end session");
    }

    return result;
}