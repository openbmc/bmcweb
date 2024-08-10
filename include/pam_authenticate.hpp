#pragma once

#include <security/pam_appl.h>

#include <cstring>
#include <memory>
#include <span>
#include <string_view>

// function used to get user input
inline int pamFunctionConversation(int numMsg, const struct pam_message** msg,
                                   struct pam_response** resp, void* appdataPtr)
{
    if ((appdataPtr == nullptr) || (msg == nullptr) || (resp == nullptr))
    {
        return PAM_CONV_ERR;
    }

    if (numMsg <= 0 || numMsg >= PAM_MAX_NUM_MSG)
    {
        return PAM_CONV_ERR;
    }

    auto msgCount = static_cast<size_t>(numMsg);
    auto messages = std::span(msg, msgCount);
    auto responses = std::span(resp, msgCount);

    for (size_t i = 0; i < msgCount; ++i)
    {
        /* Ignore all PAM messages except prompting for hidden input */
        if (messages[i]->msg_style != PAM_PROMPT_ECHO_OFF)
        {
            continue;
        }

        size_t numMsgSize = static_cast<size_t>(numMsg);
        // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
        void* ptr = calloc(numMsgSize, sizeof(struct pam_response));
        if (ptr == nullptr)
        {
            return PAM_BUF_ERR;
        }

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        *resp = reinterpret_cast<pam_response*>(ptr);

        char **authData = (char **)appdataPtr;  // Array containing password and TOTP
        if (strstr(msg[i]->msg, "Password") != NULL)
        {
             responses[i]->resp = strdup(authData[0]);  // Password input
        }
        if (strstr(msg[i]->msg, "Verification code") != NULL || strstr(msg[i]->msg, "TOTP") != NULL)
        {
             responses[i]->resp = strdup(authData[1]);  // TOTP input
        }

        return PAM_SUCCESS;
    }

    return PAM_CONV_ERR;
}

/**
 * @brief Attempt username/password authentication via PAM.
 * @param username The provided username aka account name.
 * @param password The provided password.
 * @param token The provided MFA token.
 * @returns PAM error code or PAM_SUCCESS for success. */
inline int pamAuthenticateUser(std::string_view username,
                               std::string_view password,
                               std::optional<std::string> token)
{
    std::string userStr(username);
    std::string passStr(password);

    char *authData[2];
    authData[0] = passStr.data();
    if(token)
    {
        std::string passcode(*token);
        authData[1] = passcode.data();
    }

    const struct pam_conv localConversation = {pamFunctionConversation,
                                               authData};
    pam_handle_t* localAuthHandle = nullptr; // this gets set by pam_start

    int retval = pam_start("webserver", userStr.c_str(), &localConversation,
                           &localAuthHandle);
    if (retval != PAM_SUCCESS)
    {
        return retval;
    }

    retval = pam_authenticate(localAuthHandle,
                              PAM_SILENT | PAM_DISALLOW_NULL_AUTHTOK);
    if (retval != PAM_SUCCESS)
    {
        pam_end(localAuthHandle, PAM_SUCCESS); // ignore retval
        return retval;
    }

    /* check that the account is healthy */
    retval = pam_acct_mgmt(localAuthHandle, PAM_DISALLOW_NULL_AUTHTOK);
    if (retval != PAM_SUCCESS)
    {
        pam_end(localAuthHandle, PAM_SUCCESS); // ignore retval
        return retval;
    }

    return pam_end(localAuthHandle, PAM_SUCCESS);
}

inline int pamUpdatePassword(const std::string& username,
                             const std::string& password)
{

    char *authData[2];
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    authData[0] = const_cast<char*>(password.c_str());
    const struct pam_conv localConversation = {pamFunctionConversation,
                                               authData};
    pam_handle_t* localAuthHandle = nullptr; // this gets set by pam_start

    int retval = pam_start("webserver", username.c_str(), &localConversation,
                           &localAuthHandle);

    if (retval != PAM_SUCCESS)
    {
        return retval;
    }

    retval = pam_chauthtok(localAuthHandle, PAM_SILENT);
    if (retval != PAM_SUCCESS)
    {
        pam_end(localAuthHandle, PAM_SUCCESS);
        return retval;
    }

    return pam_end(localAuthHandle, PAM_SUCCESS);
}
