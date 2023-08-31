#include "eac.h"

#include <stdio.h>
#include <stdlib.h>
#include <eos_sdk.h>
#include <eos_logging.h>
#include <eos_anticheatclient.h>

EOS_ProductUserId local_user_id = NULL;
EOS_HConnect hConnect = NULL;

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

void integrity_callback(
    const EOS_AntiCheatClient_OnClientIntegrityViolatedCallbackInfo *data) {
    printf("[INTEGRITY] %s\n", data->ViolationMessage);
}

EOS_EResult eac_init(EOS_HPlatform *hPlatform, EOS_HAntiCheatClient *hAntiCheat,
                     EOS_AntiCheatClient_OnMessageToServerCallback onMessage) {

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
    Creds.ClientId = "xyza7891SyUKJJgg4ryKMabvtPJt8V2Y";
    Creds.ClientSecret = "ZZE9Z+D8zcyvA56S4ZKeNBTPTtpa7sSQsF+BoZdN+Yc";

    EOS_Platform_Options PlatformOptions = {};
    PlatformOptions.ClientCredentials = Creds;
    PlatformOptions.EncryptionKey =
        "1111111111111111111111111111111111111111111111111111111111111111";
    PlatformOptions.ProductId = "PRODUCT_ID_HERE";
    PlatformOptions.SandboxId = "SANDBOX_ID_HERE";
    PlatformOptions.DeploymentId = "DEPLOYMENT_ID_HERE";
    PlatformOptions.CacheDirectory = "/tmp/my_eos_cache/";
    PlatformOptions.bIsServer = false;
    PlatformOptions.Flags = 0;
    PlatformOptions.Reserved = NULL;
    PlatformOptions.RTCOptions = NULL;
    PlatformOptions.OverrideLocaleCode = NULL;
    PlatformOptions.OverrideCountryCode = NULL;
    PlatformOptions.TickBudgetInMilliseconds = 0;
    PlatformOptions.IntegratedPlatformOptionsContainerHandle = NULL;
    PlatformOptions.ApiVersion = EOS_PLATFORM_OPTIONS_API_LATEST;

    EOS_AntiCheatClient_AddNotifyClientIntegrityViolatedOptions
        IntegrityOptions = {};
    IntegrityOptions.ApiVersion =
        EOS_ANTICHEATCLIENT_ADDNOTIFYPEERAUTHSTATUSCHANGED_API_LATEST;

    EOS_AntiCheatClient_AddNotifyMessageToServerOptions MsgOptions = {};
    MsgOptions.ApiVersion =
        EOS_ANTICHEATCLIENT_ADDNOTIFYMESSAGETOSERVER_API_LATEST;

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

        if (NULL == (*hAntiCheat = EOS_Platform_GetAntiCheatClientInterface(
                         *hPlatform))) {
            printf(
                "%s\n",
                "[FAIL] Failed to get a handle to the EOS AntiCheat Interface");
            result = EOS_EResult::EOS_InvalidState;
            break;
        }

        // register callbacks

        if (EOS_INVALID_NOTIFICATIONID ==
            EOS_AntiCheatClient_AddNotifyClientIntegrityViolated(
                *hAntiCheat, &IntegrityOptions, NULL, integrity_callback)) {
            printf("%s\n",
                   "[FAIL] Failed to register ClientIntegrity callback");
            result = EOS_EResult::EOS_InvalidState;
            break;
        }

        if (EOS_INVALID_NOTIFICATIONID ==
            EOS_AntiCheatClient_AddNotifyMessageToServer(
                *hAntiCheat, &MsgOptions, NULL, onMessage)) {
            printf("%s\n",
                   "[FAIL] Failed to register MessageToServer callback");
            result = EOS_EResult::EOS_InvalidState;
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

void eac_create_user_callback(const EOS_Connect_CreateUserCallbackInfo *Data) {
    if (EOS_EResult::EOS_Success == Data->ResultCode) {
        printf("[AUTH] %s\n",
               "Created a new EOS user, please restart the game to login");
    } else {
        printf("[AUTH] Failed CreateUser result code: %d\n", Data->ResultCode);
    }
    exit(1);
}

void eac_create_user(EOS_ContinuanceToken token) {
    EOS_Connect_CreateUserOptions Options = {};
    Options.ContinuanceToken = token;
    Options.ApiVersion = EOS_CONNECT_CREATEUSER_API_LATEST;

    EOS_Connect_CreateUser(hConnect, &Options, NULL, eac_create_user_callback);
}

void eac_auth_callback(const EOS_Connect_LoginCallbackInfo *Data) {

    // create user if invalid
    if (EOS_EResult::EOS_InvalidUser == Data->ResultCode) {
        printf("[AUTH] %s\n", "Creating a new EOS user for the OpenID user");
        eac_create_user(Data->ContinuanceToken);
    }

    if (EOS_EResult::EOS_Success != Data->ResultCode) {
        printf("[AUTH] Failed, result code: %d\n", Data->ResultCode);
        exit(1);
    }

    // set logged in PUID - does this need thread safety?
    EOS_ProductUserId *user = (EOS_ProductUserId *)Data->ClientData;
    *user = Data->LocalUserId;
}

void eac_auth_expire_callback(
    const EOS_Connect_AuthExpirationCallbackInfo *Data) {
    printf("[EXPIRING AUTH] %s\n", "callback fired");
}

EOS_EResult eac_auth(EOS_HPlatform hPlatform, EOS_ProductUserId *user) {
    
    EOS_Connect_Credentials Creds = {};
    Creds.Type = EOS_EExternalCredentialType::EOS_ECT_OPENID_ACCESS_TOKEN;
    Creds.Token = "000000000000";
    Creds.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;

    EOS_Connect_LoginOptions LoginOptions = {};
    LoginOptions.Credentials = &Creds;
    LoginOptions.UserLoginInfo = NULL;
    LoginOptions.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;

    EOS_Connect_AddNotifyAuthExpirationOptions ChangeOptions = {};
    ChangeOptions.ApiVersion = EOS_CONNECT_ADDNOTIFYAUTHEXPIRATION_API_LATEST;

    hConnect = NULL;
    if (NULL == (hConnect = EOS_Platform_GetConnectInterface(hPlatform))) {
        printf("%s\n",
               "[FAIL] Failed to get a handle to the EOS Connect Interface");
        return EOS_EResult::EOS_InvalidState;
    }

    if (EOS_INVALID_NOTIFICATIONID ==
        EOS_Connect_AddNotifyAuthExpiration(hConnect, &ChangeOptions, NULL,
                                            eac_auth_expire_callback)) {
        printf("[FAIL] %s\n", "Failed to register callback for auth expiry");
        return EOS_EResult::EOS_InvalidState;
    }

    EOS_Connect_Login(hConnect, &LoginOptions, user, eac_auth_callback);

    return EOS_EResult::EOS_Success;
}

EOS_EResult eac_begin(EOS_HAntiCheatClient hAntiCheat, EOS_ProductUserId user) {

    EOS_AntiCheatClient_BeginSessionOptions Options = {};
    Options.Mode = EOS_EAntiCheatClientMode::EOS_ACCM_ClientServer;
    Options.LocalUserId = user;
    Options.ApiVersion = EOS_ANTICHEATCLIENT_BEGINSESSION_API_LATEST;

    return EOS_AntiCheatClient_BeginSession(hAntiCheat, &Options);
}

EOS_EResult eac_end(EOS_HAntiCheatClient hAntiCheat) {

    EOS_AntiCheatClient_EndSessionOptions Options = {};
    Options.ApiVersion = EOS_ANTICHEATCLIENT_ENDSESSION_API_LATEST;

    return EOS_AntiCheatClient_EndSession(hAntiCheat, &Options);
}
