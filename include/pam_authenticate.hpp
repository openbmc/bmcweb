#pragma once

#include <security/pam_appl.h>

#include <cstring>
#include <memory>
#include <span>
#include <string_view>

struct PasswordData
{
    using KVPair = std::pair<std::string_view, std::string_view>;
    // assuming 2 prompts for now based on google authenticator needs.
    static constexpr int MAX_PROMPT = 2;
    using PromptData = std::array<KVPair, MAX_PROMPT>;
    PromptData promptData;
    explicit PasswordData(PromptData data) : promptData(std::move(data)) {}
    void addPrompt(KVPair p)
    {
        auto iter = std::ranges::find_if(promptData, [](auto& v) {
            return v.first.empty();
        });
        if (iter == end(promptData))
        {
            BMCWEB_LOG_ERROR("Max prompt {} Reached", MAX_PROMPT);
            return;
        }
        *iter = std::move(p);
    }

    int makeResponse(const pam_message& msg, pam_response& response)
    {
        switch (msg.msg_style)
        {
            case PAM_PROMPT_ECHO_ON:
                break;
            case PAM_PROMPT_ECHO_OFF:
            {
                std::string prompt(msg.msg);
                if (validatePrompt(prompt) != PAM_SUCCESS)
                {
                    return PAM_CONV_ERR;
                }
                const auto* promptDataIter = std::ranges::find_if(
                    promptData, [&prompt](const KVPair& data) {
                        return prompt.starts_with(data.first);
                    });
                if (promptDataIter == promptData.end())
                {
                    return PAM_CONV_ERR;
                }
                response.resp = strdup(promptDataIter->second.data());
                return PAM_SUCCESS;
            }
            break;
            case PAM_ERROR_MSG:
            {
                BMCWEB_LOG_ERROR("Pam error {}", msg.msg);
            }
            break;
            case PAM_TEXT_INFO:
            {
                BMCWEB_LOG_ERROR("Pam info {}", msg.msg);
            }
            break;
            default:
            {
                return PAM_CONV_ERR;
            }
        }
        return PAM_SUCCESS;
    }

    static int validatePrompt(std::string_view prompt)
    {
        if (prompt.length() + 1 > PAM_MAX_MSG_SIZE)
        {
            BMCWEB_LOG_ERROR("length error", prompt);
            return PAM_CONV_ERR;
        }
        return PAM_SUCCESS;
    }
};

// function used to get user input
inline int pamFunctionConversation(int numMsg, const struct pam_message** msgs,
                                   struct pam_response** resp, void* appdataPtr)
{
    if ((appdataPtr == nullptr) || (msgs == nullptr) || (resp == nullptr))
    {
        return PAM_CONV_ERR;
    }

    if (numMsg <= 0 || numMsg >= PAM_MAX_NUM_MSG)
    {
        return PAM_CONV_ERR;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    PasswordData* appPass = reinterpret_cast<PasswordData*>(appdataPtr);
    auto msgCount = static_cast<size_t>(numMsg);
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
    auto responseArrPtr = std::make_unique<pam_response[]>(msgCount);
    auto responses = std::span(responseArrPtr.get(), msgCount);
    auto messagePtrs = std::span(msgs, msgCount);
    for (size_t i = 0; i < msgCount; ++i)
    {
        const pam_message& msg = *(messagePtrs[i]);

        pam_response& response = responses[i];
        response.resp_retcode = 0;
        response.resp = nullptr;

        int r = appPass->makeResponse(msg, response);
        if (r != PAM_SUCCESS)
        {
            return r;
        }
    }

    *resp = responseArrPtr.release();
    return PAM_SUCCESS;
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
    PasswordData data({PasswordData::KVPair{"Password:", password}});
    if (token)
    {
        data.addPrompt(PasswordData::KVPair{"Token:", *token});
    }

    const struct pam_conv localConversation = {pamFunctionConversation, &data};
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
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    PasswordData data({PasswordData::KVPair{"Password:", password}});
    const struct pam_conv localConversation = {pamFunctionConversation, &data};
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
