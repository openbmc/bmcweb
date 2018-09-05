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
        return PAM_AUTH_ERR;
    }
    auto* pass = reinterpret_cast<char*>(
        malloc(std::strlen(reinterpret_cast<char*>(appdataPtr)) + 1));
    std::strcpy(pass, reinterpret_cast<char*>(appdataPtr));

    *resp = reinterpret_cast<pam_response*>(
        calloc(numMsg, sizeof(struct pam_response)));

    for (int i = 0; i < numMsg; ++i)
    {
        /* Ignore all PAM messages except prompting for hidden input */
        if (msg[i]->msg_style != PAM_PROMPT_ECHO_OFF)
        {
            continue;
        }

        /* Assume PAM is only prompting for the password as hidden input */
        resp[i]->resp = pass;
    }

    return PAM_SUCCESS;
}

inline bool pamAuthenticateUser(const boost::string_view username,
                                const boost::string_view password)
{
    std::string userStr(username);
    std::string passStr(password);
    const struct pam_conv localConversation = {
        pamFunctionConversation, const_cast<char*>(passStr.c_str())};
    pam_handle_t* localAuthHandle = NULL; // this gets set by pam_start

    if (pam_start("webserver", userStr.c_str(), &localConversation,
                  &localAuthHandle) != PAM_SUCCESS)
    {
        return false;
    }
    int retval = pam_authenticate(localAuthHandle,
                                  PAM_SILENT | PAM_DISALLOW_NULL_AUTHTOK);

    if (retval != PAM_SUCCESS)
    {
        pam_end(localAuthHandle, PAM_SUCCESS);
        return false;
    }

    /* check that the account is healthy */
    if (pam_acct_mgmt(localAuthHandle, PAM_DISALLOW_NULL_AUTHTOK) !=
        PAM_SUCCESS)
    {
        pam_end(localAuthHandle, PAM_SUCCESS);
        return false;
    }

    if (pam_end(localAuthHandle, PAM_SUCCESS) != PAM_SUCCESS)
    {
        return false;
    }

    return true;
}

inline bool pamUpdatePassword(const std::string& username,
                              const std::string& password)
{
    const struct pam_conv localConversation = {
        pamFunctionConversation, const_cast<char*>(password.c_str())};
    pam_handle_t* localAuthHandle = NULL; // this gets set by pam_start

    if (pam_start("passwd", username.c_str(), &localConversation,
                  &localAuthHandle) != PAM_SUCCESS)
    {
        return false;
    }
    int retval = pam_chauthtok(localAuthHandle, PAM_SILENT);

    if (retval != PAM_SUCCESS)
    {
        pam_end(localAuthHandle, PAM_SUCCESS);
        return false;
    }

    if (pam_end(localAuthHandle, PAM_SUCCESS) != PAM_SUCCESS)
    {
        return false;
    }

    return true;
}
