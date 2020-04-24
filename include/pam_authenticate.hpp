#pragma once

#include <security/pam_appl.h>

#include <boost/utility/string_view.hpp>
#include <chrono>
#include <cstring>
#include <memory>
#include <queue>

// function used to get user input
inline int pamFunctionConversation(int numMsg, const struct pam_message** msg,
                                   struct pam_response** resp, void* appdataPtr)
{
    if (appdataPtr == nullptr)
    {
        return PAM_AUTH_ERR;
    }
    char* appPass = reinterpret_cast<char*>(appdataPtr);
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

/**
 * @brief Helps you rate-limit events, for example, 1 login per second.
 * Uses a sliding time window with 1 second granularity.
 * Usage:
 *   Create a RateLimiter object.
 *   Invoke addEvent() every time the event happens.
 *   Use isEventAllowed() to learn if you should deny due to rate-limiting.
 **/
template <unsigned int MaxEvents, unsigned int PeriodInSeconds>
class RateLimiter
{
    std::queue<std::chrono::steady_clock::time_point> eventQueue;
    void expireOldEvents()
    {
        std::chrono::steady_clock::time_point expirationTime =
            std::chrono::steady_clock::now() -
            std::chrono::seconds(PeriodInSeconds);
        while ((!eventQueue.empty()) && (eventQueue.front() < expirationTime))
        {
            eventQueue.pop();
        }
    }

  public:
    bool isEventAllowed()
    {
        expireOldEvents();
        return (eventQueue.size() < MaxEvents);
    }
    void addEvent()
    {
        expireOldEvents();
        eventQueue.push(std::chrono::steady_clock::now());
    }
};

// See https://github.com/linux-pam/linux-pam/issues/216
#define PSEUDO_PAM_RATE_LIMITING (_PAM_RETURN_VALUES + 1)
/* Pam authentication unavailable due to rate limiting */

namespace pam_auth
{
namespace rate_limiter
{
/**
 * @brief Define parameters of the authentication rate limiter. Apply
 * rate-limiting when there are EventThreshold events in the past
 * PeriodInSeconds.
 * @const EventThreshold The number of events to trigger rate-limiting.
 * @const PeriodInSeconds The maximum duration to rate-limit.
 */
constexpr unsigned int EventThreshold = 5;
constexpr unsigned int PeriodInSeconds = 30;
} // namespace rate_limiter
} // namespace pam_auth

/**
 * @brief Attempt username/password authentication via PAM.
 * @param username The provided username aka account name.
 * @param password The provided password.
 * @returns PAM error code or PAM_SUCCESS for success. */
inline int pamAuthenticateUser(const std::string_view username,
                               const std::string_view password)
{
    // Rate limit authentication failures without regard to account name or
    // request origin.  Note the tricky usage: This only counts authentication
    // *failures* but applies the rate-limit to all attempts.  This allows
    // authentication to run at full speed until there are too many failures.
    static RateLimiter<pam_auth::rate_limiter::EventThreshold,
                       pam_auth::rate_limiter::PeriodInSeconds>
        failedAuthRateLimiter;

    if (!failedAuthRateLimiter.isEventAllowed())
    {
        return PSEUDO_PAM_RATE_LIMITING; // PAM_MAXTRIES?
    }

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
        failedAuthRateLimiter.addEvent();
        return retval;
    }

    /* Authentication is successful. Check if the account is healthy */
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
