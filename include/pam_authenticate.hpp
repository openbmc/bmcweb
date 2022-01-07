#pragma once

#include <security/pam_appl.h>

#include <boost/utility/string_view.hpp>

#include <cstring>
#include <memory>

// function used to get user input
inline int pamFunctionConversation(int numMsg, const struct pam_message** msg,
                                   struct pam_response** resp, void* appdataPtr)
{
    if (appdataPtr == nullptr)
    {
        return PAM_CONV_ERR;
    }

    if (numMsg <= 0 || numMsg >= PAM_MAX_NUM_MSG)
    {
        return PAM_CONV_ERR;
    }

    for (int i = 0; i < numMsg; ++i)
    {
        /* Ignore all PAM messages except prompting for hidden input */
        if (msg[i]->msg_style != PAM_PROMPT_ECHO_OFF)
        {
            continue;
        }

        /* Assume PAM is only prompting for the password as hidden input */
        /* Allocate memory only when PAM_PROMPT_ECHO_OFF is encounterred */

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        char* appPass = reinterpret_cast<char*>(appdataPtr);
        size_t appPassSize = std::strlen(appPass);

        if ((appPassSize + 1) > PAM_MAX_RESP_SIZE)
        {
            return PAM_CONV_ERR;
        }
        // IDeally we'd like to avoid using malloc here, but because we're
        // passing off ownership of this to a C application, there aren't a lot
        // of sane ways to avoid it.

        // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)'
        void* passPtr = malloc(appPassSize + 1);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        char* pass = reinterpret_cast<char*>(passPtr);
        if (pass == nullptr)
        {
            return PAM_BUF_ERR;
        }

        std::strncpy(pass, appPass, appPassSize + 1);

        size_t numMsgSize = static_cast<size_t>(numMsg);
        void* ptr = calloc(numMsgSize, sizeof(struct pam_response));
        if (ptr == nullptr)
        {
            free(pass);
            return PAM_BUF_ERR;
        }

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        *resp = reinterpret_cast<pam_response*>(ptr);

        resp[i]->resp = pass;

        return PAM_SUCCESS;
    }

    return PAM_CONV_ERR;
}

/**
 * @brief Attempt username/password authentication via PAM.
 * @param username The provided username aka account name.
 * @param password The provided password.
 * @returns PAM error code or PAM_SUCCESS for success. */
inline int pamAuthenticateUser(const std::string_view username,
                               const std::string_view password)
{
    std::string userStr(username);
    std::string passStr(password);
    const struct pam_conv localConversation = {
        pamFunctionConversation, const_cast<char*>(passStr.c_str())};
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
    const struct pam_conv localConversation = {
        pamFunctionConversation, const_cast<char*>(password.c_str())};
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
