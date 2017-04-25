#include <random>
#include <unordered_map>
#include <boost/algorithm/string/predicate.hpp>

#include <base64.hpp>
#include <token_authorization_middleware.hpp>
#include <crow/logging.h>

namespace crow {

using random_bytes_engine =
    std::independent_bits_engine<std::default_random_engine, CHAR_BIT,
                                 unsigned char>;

TokenAuthorizationMiddleware::TokenAuthorizationMiddleware()
    : auth_token2(""){

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

  CROW_LOG_DEBUG << "Token Auth Got route " << req.url;

  if (req.url == "/" || boost::starts_with(req.url, "/static/")) {
    // TODO this is total hackery to allow the login page to work before the
    // user is authenticated.  Also, it will be quite slow for all pages instead
    // of a one time hit for the whitelist entries.  Ideally, this should be
    // done
    // in the url router handler, with tagged routes for the whitelist entries.
    // Another option would be to whitelist a minimal
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
      if (username == "dude" && password == "dude") {
        crow::json::wvalue x;

        // TODO(ed) the RNG should be initialized at start, not every time we
        // want a token
        std::random_device rand;
        random_bytes_engine rbe;
        std::string token('a', 20);
        // TODO(ed) for some reason clang-tidy finds a divide by zero error in
        // cstdlibc here
        // commented out for now.  Needs investigation
        std::generate(begin(token), end(token), std::ref(rbe));  // NOLINT
        std::string encoded_token;
        base64::base64_encode(token, encoded_token);
        ctx.auth_token = encoded_token;
        this->auth_token2 = encoded_token;

        auto auth_token = ctx.auth_token;
        x["token"] = auth_token;

        res.write(json::dump(x));
        res.add_header("Content-Type", "application/json");
        res.end();
      } else {
        return_unauthorized();
        return;
      }
    }

  } else if (req.url == "/logout") {
    this->auth_token2 = "";
    res.code = 200;
    res.end();
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

    // TODO(ed), use span here instead of constructing a new string
    if (auth_header.substr(6) != this->auth_token2) {
      return_unauthorized();
      return;
    }
    // else let the request continue unharmed
  }
}

void TokenAuthorizationMiddleware::after_handle(request& req, response& res,
                                                context& ctx) {}
}
