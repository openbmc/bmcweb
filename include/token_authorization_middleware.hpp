#pragma once

#include <crow/http_request.h>
#include <crow/http_response.h>
#include <boost/container/flat_set.hpp>

#include <base64.hpp>

#include <pam_authenticate.hpp>

namespace crow {

struct User {};

using random_bytes_engine =
    std::independent_bits_engine<std::default_random_engine, CHAR_BIT,
                                 unsigned char>;

template <class AuthenticationFunction>
struct TokenAuthorization {
 private:
  random_bytes_engine rbe;

 public:
  struct context {
    // std::string auth_token;
  };

  TokenAuthorization(){};

  void before_handle(crow::request& req, response& res, context& ctx) {
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
      // user is authenticated.  Also, it will be quite slow for all pages
      // instead of a one time hit for the whitelist entries.  Ideally, this
      // should be
      // done in the url router handler, with tagged routes for the whitelist
      // entries. Another option would be to whitelist a minimal for based page
      // that didn't load the full angular UI until after login
      return;
    }

    if (req.url == "/login") {
      if (req.method != HTTPMethod::POST) {
        return_unauthorized();
        return;
      } else {
        std::string username;
        std::string password;
        try {
          auto login_credentials = nlohmann::json::parse(req.body);
          username = login_credentials["username"];
          password = login_credentials["password"];
        } catch (...) {
          return_bad_request();
          return;
        }

        auto p = AuthenticationFunction();
        if (p.authenticate(username, password)) {
          nlohmann::json x;

          std::string token('a', 20);
          // TODO(ed) for some reason clang-tidy finds a divide by zero error in
          // cstdlibc here commented out for now.  Needs investigation
          std::generate(std::begin(token), std::end(token),
                        std::ref(rbe));  // NOLINT
          std::string encoded_token;
          base64::base64_encode(token, encoded_token);
          // ctx.auth_token = encoded_token;
          this->auth_token2.insert(encoded_token);

          nlohmann::json ret{{"token", encoded_token}};

          res.write(ret.dump());
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

  void after_handle(request& req, response& res, context& ctx) {
    // Do nothing
  }

 private:
  boost::container::flat_set<std::string> auth_token2;
};

using TokenAuthorizationMiddleware = TokenAuthorization<PamAuthenticator>;
}