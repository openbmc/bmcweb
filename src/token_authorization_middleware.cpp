#include <random>
#include <unordered_map>
#include <boost/algorithm/string/predicate.hpp>

#include <security/pam_appl.h>
#include <base64.hpp>
#include <token_authorization_middleware.hpp>
#include <crow/logging.h>

namespace crow {

using random_bytes_engine =
    std::independent_bits_engine<std::default_random_engine, CHAR_BIT,
                                 unsigned char>;

// function used to get user input
int pam_function_conversation(int num_msg, const struct pam_message** msg,
                              struct pam_response** resp, void* appdata_ptr) {
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

bool authenticate_user_pam(const std::string& username,
                           const std::string& password) {
  const struct pam_conv local_conversation = {pam_function_conversation,
                                              (char*)password.c_str()};
  pam_handle_t* local_auth_handle = NULL;  // this gets set by pam_start

  int retval;
  retval = pam_start("su", username.c_str(), &local_conversation,
                     &local_auth_handle);

  if (retval != PAM_SUCCESS) {
    printf("pam_start returned: %d\n ", retval);
    return false;
  }

  retval = pam_authenticate(local_auth_handle,
                            PAM_SILENT | PAM_DISALLOW_NULL_AUTHTOK);

  if (retval != PAM_SUCCESS) {
    if (retval == PAM_AUTH_ERR) {
      printf("Authentication failure.\n");
    } else {
      printf("pam_authenticate returned %d\n", retval);
    }
    return false;
  }

  printf("Authenticated.\n");
  retval = pam_end(local_auth_handle, retval);

  if (retval != PAM_SUCCESS) {
    printf("pam_end returned\n");
    return false;
  }

  return true;
}

TokenAuthorizationMiddleware::TokenAuthorizationMiddleware(){
};

void TokenAuthorizationMiddleware::before_handle(crow::request& req,
                                                 response& res, context& ctx) {
  auto return_unauthorized = [&req, &res]() {
    res.code = 401;
    res.end();
  };

  auto return_bad_request = [&req, &res]() {
    res.code = 400;
    res.end();
  };

  auto return_internal_error = [&req, &res]() {
    res.code = 500;
    res.end();
  };

  if (req.url == "/" || boost::starts_with(req.url, "/static/")) {
    // TODO this is total hackery to allow the login page to work before the
    // user is authenticated.  Also, it will be quite slow for all pages instead
    // of a one time hit for the whitelist entries.  Ideally, this should be
    // done in the url router handler, with tagged routes for the whitelist
    // entries. Another option would be to whitelist a minimal for based page
    // that didn't
    // load the full angular UI until after login
    return;
  }

  if (req.url == "/login") {
    if (req.method != HTTPMethod::POST) {
      return_unauthorized();
      return;
    } else {
      auto login_credentials = crow::json::load(req.body);
      if (!login_credentials) {
        return_bad_request();
        return;
      }
      if (!login_credentials.has("username") ||
          !login_credentials.has("password")) {
        return_bad_request();
        return;
      }
      auto username = login_credentials["username"].s();
      auto password = login_credentials["password"].s();

      // TODO(ed) pull real passwords from PAM
      if (authenticate_user_pam(username, password)) {
        crow::json::wvalue x;

        // TODO(ed) the RNG should be initialized at start, not every time we
        // want a token
        std::random_device rand;
        random_bytes_engine rbe;
        std::string token('a', 20);
        // TODO(ed) for some reason clang-tidy finds a divide by zero error in
        // cstdlibc here commented out for now.  Needs investigation
        std::generate(begin(token), end(token), std::ref(rbe));  // NOLINT
        std::string encoded_token;
        base64::base64_encode(token, encoded_token);
        // ctx.auth_token = encoded_token;
        this->auth_token2.insert(encoded_token);

        x["token"] = encoded_token;

        res.write(json::dump(x));
        res.add_header("Content-Type", "application/json");
        res.end();
      } else {
        return_unauthorized();
        return;
      }
    }

  } else {  // Normal, non login, non static file request
    // Check to make sure we're logged in
    if (this->auth_token2.empty()) {
      return_unauthorized();
      return;
    }
    // Check for an authorization header, reject if not present
    if (req.headers.count("Authorization") != 1) {
      return_unauthorized();
      return;
    }

    std::string auth_header = req.get_header_value("Authorization");
    // If the user is attempting any kind of auth other than token, reject
    if (!boost::starts_with(auth_header, "Token ")) {
      return_unauthorized();
      return;
    }
    std::string auth_key = auth_header.substr(6);
    // TODO(ed), use span here instead of constructing a new string
    if (this->auth_token2.find(auth_key) == this->auth_token2.end()) {
      return_unauthorized();
      return;
    }

    if (req.url == "/logout") {
      this->auth_token2.erase(auth_key);
      res.code = 200;
      res.end();
      return;
    }

    // else let the request continue unharmed
  }
}

void TokenAuthorizationMiddleware::after_handle(request& req, response& res,
                                                context& ctx) {
  // Do nothing
}
}
