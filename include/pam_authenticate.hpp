#include <security/pam_appl.h>

// function used to get user input
inline int pam_function_conversation(int num_msg,
                                     const struct pam_message** msg,
                                     struct pam_response** resp,
                                     void* appdata_ptr) {
  char* pass = (char*)malloc(strlen((char*)appdata_ptr) + 1);
  strcpy(pass, (char*)appdata_ptr);

  int i;

  *resp = (pam_response*)calloc(num_msg, sizeof(struct pam_response));

  for (i = 0; i < num_msg; ++i) {
    /* Ignore all PAM messages except prompting for hidden input */
    if (msg[i]->msg_style != PAM_PROMPT_ECHO_OFF) continue;

    /* Assume PAM is only prompting for the password as hidden input */
    resp[i]->resp = pass;
  }

  return PAM_SUCCESS;
}

class PamAuthenticator {
 public:
  inline bool authenticate(const std::string& username,
                           const std::string& password) {
    const struct pam_conv local_conversation = {pam_function_conversation,
                                                (char*)password.c_str()};
    pam_handle_t* local_auth_handle = NULL;  // this gets set by pam_start

    int retval;
    retval = pam_start("su", username.c_str(), &local_conversation,
                       &local_auth_handle);

    if (retval != PAM_SUCCESS) {
      //printf("pam_start returned: %d\n ", retval);
      return false;
    }

    retval = pam_authenticate(local_auth_handle,
                              PAM_SILENT | PAM_DISALLOW_NULL_AUTHTOK);

    if (retval != PAM_SUCCESS) {
      if (retval == PAM_AUTH_ERR) {
        //printf("Authentication failure.\n");
      } else {
        //printf("pam_authenticate returned %d\n", retval);
      }
      return false;
    }

    //printf("Authenticated.\n");
    retval = pam_end(local_auth_handle, retval);

    if (retval != PAM_SUCCESS) {
      //printf("pam_end returned\n");
      return false;
    }

    return true;
  }
};