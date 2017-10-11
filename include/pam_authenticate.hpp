#pragma once

#include <security/pam_appl.h>
#include <cstring>

// function used to get user input
inline int pam_function_conversation(int num_msg,
                                     const struct pam_message** msg,
                                     struct pam_response** resp,
                                     void* appdata_ptr) {
  if (appdata_ptr == nullptr) {
    return PAM_AUTH_ERR;
  }
  auto* pass = reinterpret_cast<char*>(
      malloc(std::strlen(reinterpret_cast<char*>(appdata_ptr)) + 1));
  std::strcpy(pass, reinterpret_cast<char*>(appdata_ptr));

  *resp = reinterpret_cast<pam_response*>(
      calloc(num_msg, sizeof(struct pam_response)));

  for (int i = 0; i < num_msg; ++i) {
    /* Ignore all PAM messages except prompting for hidden input */
    if (msg[i]->msg_style != PAM_PROMPT_ECHO_OFF) {
      continue;
    }

    /* Assume PAM is only prompting for the password as hidden input */
    resp[i]->resp = pass;
  }

  return PAM_SUCCESS;
}

inline bool pam_authenticate_user(const std::string& username,
                                  const std::string& password) {
  const struct pam_conv local_conversation = {
      pam_function_conversation, const_cast<char*>(password.c_str())};
  pam_handle_t* local_auth_handle = NULL;  // this gets set by pam_start

  if (pam_start("dropbear", username.c_str(), &local_conversation,
                &local_auth_handle) != PAM_SUCCESS) {
    return false;
  }
  int retval = pam_authenticate(local_auth_handle,
                                PAM_SILENT | PAM_DISALLOW_NULL_AUTHTOK);

  if (retval != PAM_SUCCESS) {
    if (retval == PAM_AUTH_ERR) {
      // printf("Authentication failure.\n");
    } else {
      // printf("pam_authenticate returned %d\n", retval);
    }
    pam_end(local_auth_handle, PAM_SUCCESS);
    return false;
  }

  /* check that the account is healthy */
  if (pam_acct_mgmt(local_auth_handle, PAM_DISALLOW_NULL_AUTHTOK) !=
      PAM_SUCCESS) {
    pam_end(local_auth_handle, PAM_SUCCESS);
    return false;
  }

  if (pam_end(local_auth_handle, PAM_SUCCESS) != PAM_SUCCESS) {
    return false;
  }

  return true;
}
