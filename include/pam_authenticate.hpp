#pragma once

#include <security/pam_appl.h>

#include <cstring>
#include <memory>

#include <string_view>

// function used to get user input
int pamFunctionConversation(int numMsg, const struct pam_message** msg,
                                   struct pam_response** resp, void* appdataPtr);

/**
 * @brief Attempt username/password authentication via PAM.
 * @param username The provided username aka account name.
 * @param password The provided password.
 * @returns PAM error code or PAM_SUCCESS for success. */
int pamAuthenticateUser(const std::string_view username,
                               const std::string_view password);

int pamUpdatePassword(const std::string& username,
                             const std::string& password);