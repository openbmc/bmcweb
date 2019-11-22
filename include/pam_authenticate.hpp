#pragma once

#include <security/pam_appl.h>

#include <boost/utility/string_view.hpp>
#include <cstring>
#include <memory>

/**
   @brief Format of the appdata passed into the PAM conversation function.
   @field password The password value.
   @field messagesFromPamPtr Collected responses from PAM.
 **/
struct PamConvAppData
{
    const char* password;
    std::vector<std::string>* messagesFromPamPtr;
};

// function used to get user input
inline int pamFunctionConversation(int numMsg, const struct pam_message** msg,
                                   struct pam_response** resp, void* appdataPtr)
{
    if (appdataPtr == nullptr)
    {
        return PAM_AUTH_ERR;
    }

    PamConvAppData* pamConvAppData =
        reinterpret_cast<PamConvAppData*>(appdataPtr);
    const char* appPass = pamConvAppData->password;
    std::vector<std::string>* messagesFromPamPtr =
        pamConvAppData->messagesFromPamPtr;
    size_t appPassSize = std::strlen(appPass);
    char* pass = reinterpret_cast<char*>(malloc(appPassSize + 1));
    if (pass == nullptr)
    {
        return PAM_AUTH_ERR;
    }

    std::strcpy(pass, appPass);

    *resp = reinterpret_cast<pam_response*>(
        calloc(static_cast<size_t>(numMsg), sizeof(struct pam_response)));

    if (resp == nullptr)
    {
        return PAM_AUTH_ERR;
    }

    for (int i = 0; i < numMsg; ++i)
    {
        switch (msg[i]->msg_style)
        {
        case PAM_PROMPT_ECHO_OFF:
            /* Assume PAM is only prompting for the password as hidden input */
            resp[i]->resp = pass;
            break;
        case PAM_ERROR_MSG:
        case PAM_TEXT_INFO:
            /* Capture messages from PAM */
            if (messagesFromPamPtr)
            {
                messagesFromPamPtr->push_back(msg[i]->msg);
            }
            break;
        default:
            break;
        }
    }

    return PAM_SUCCESS;
}

inline bool pamAuthenticateUser(const std::string_view username,
                                const std::string_view password)
{
    std::string userStr(username);
    std::string passStr(password);
    PamConvAppData pamConvAppData = {passStr.c_str(), nullptr};
    struct pam_conv localConversation = {
        pamFunctionConversation,
        reinterpret_cast<char*>(&pamConvAppData)};
    pam_handle_t* localAuthHandle = nullptr; // this gets set by pam_start

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

inline int pamUpdatePassword(const std::string& username,
                             const std::string& password,
                             std::vector<std::string>& diagnosticInfo)
{
    diagnosticInfo.clear();
    PamConvAppData pamConvAppData = {password.c_str(), &diagnosticInfo};
    struct pam_conv localConversation = {
        pamFunctionConversation,
        reinterpret_cast<char*>(&pamConvAppData)};
    pam_handle_t* localAuthHandle = nullptr; // this gets set by pam_start

    int retval = pam_start("passwd", username.c_str(), &localConversation,
                           &localAuthHandle);

    if (retval != PAM_SUCCESS)
    {
        return retval;
    }

    retval = pam_chauthtok(localAuthHandle, 0);
    if (retval != PAM_SUCCESS)
    {
        pam_end(localAuthHandle, PAM_SUCCESS);
        return retval;
    }

    return pam_end(localAuthHandle, PAM_SUCCESS);
}
