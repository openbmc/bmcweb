#pragma once

#include <base64.hpp>
#include <pam_authenticate.hpp>
#include <persistent_data_middleware.hpp>
#include <webassets.hpp>
#include <random>
#include <crow/app.h>
#include <crow/http_request.h>
#include <crow/http_response.h>
#include <boost/bimap.hpp>
#include <boost/container/flat_set.hpp>
namespace crow {

namespace TokenAuthorization {
struct User {};

class Middleware {
 public:
  struct context {};
  template <typename AllContext>
  void before_handle(crow::request& req, response& res, context& ctx,
                     AllContext& allctx) {
    auto return_unauthorized = [&req, &res]() {
      res.code = 401;
      res.end();
    };

    if (crow::webassets::routes.find(req.url) !=
        crow::webassets::routes.end()) {
      // TODO this is total hackery to allow the login page to work before the
      // user is authenticated.  Also, it will be quite slow for all pages
      // instead of a one time hit for the whitelist entries.  Ideally, this
      // should be done in the url router handler, with tagged routes for the
      // whitelist entries. Another option would be to whitelist a minimal form
      // based page that didn't load the full angular UI until after login
    } else if (req.url == "/login" || req.url == "/redfish/v1" ||
               req.url == "/redfish/v1/" ||
               req.url == "/redfish/v1/$metadata" ||
               (req.url == "/redfish/v1/SessionService/Sessions/" &&
                req.method == "POST"_method)) {
    } else {
      // Normal, non login, non static file request
      // Check for an authorization header, reject if not present
      std::string auth_key;
      bool require_csrf = true;
      if (req.headers.count("Authorization") == 1) {
        std::string auth_header = req.get_header_value("Authorization");
        // If the user is attempting any kind of auth other than token, reject
        if (!boost::starts_with(auth_header, "Token ")) {
          return_unauthorized();
          return;
        }
        auth_key = auth_header.substr(6);
        require_csrf = false;
      } else if (req.headers.count("X-Auth-Token") == 1) {
        auth_key = req.get_header_value("X-Auth-Token");
        require_csrf = false;
      } else {
        int count = req.headers.count("Cookie");
        if (count == 1) {
          auto& cookie_value = req.get_header_value("Cookie");
          auto start_index = cookie_value.find("SESSION=");
          if (start_index != std::string::npos) {
            start_index += 8;
            auto end_index = cookie_value.find(";", start_index);
            if (end_index == std::string::npos) {
              end_index = cookie_value.size();
            }
            auth_key =
                cookie_value.substr(start_index, end_index - start_index);
          }
        }
        require_csrf = true;  // Cookies require CSRF
      }
      if (auth_key.empty()) {
        res.code = 400;
        res.end();
        return;
      }
      auto& data_mw = allctx.template get<PersistentData::Middleware>();
      const PersistentData::UserSession* session =
          data_mw.sessions->login_session_by_token(auth_key);
      if (session == nullptr) {
        return_unauthorized();
        return;
      }

      if (require_csrf) {
        // RFC7231 defines methods that need csrf protection
        if (req.method != "GET"_method) {
          const std::string& csrf = req.get_header_value("X-XSRF-TOKEN");
          // Make sure both tokens are filled
          if (csrf.empty() || session->csrf_token.empty()) {
            return_unauthorized();
            return;
          }
          // Reject if csrf token not available
          if (csrf != session->csrf_token) {
            return_unauthorized();
            return;
          }
        }
      }

      if (req.url == "/logout" && req.method == "POST"_method) {
        data_mw.sessions->remove_session(session);
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

  boost::container::flat_set<std::string> allowed_routes;
};

// TODO(ed) see if there is a better way to allow middlewares to request
// routes.
// Possibly an init function on first construction?
template <typename... Middlewares>
void request_routes(Crow<Middlewares...>& app) {
  static_assert(
      black_magic::contains<PersistentData::Middleware, Middlewares...>::value,
      "TokenAuthorization middleware must be enabled in app to use "
      "auth routes");
  CROW_ROUTE(app, "/login")
      .methods(
          "POST"_method)([&](const crow::request& req, crow::response& res) {
        std::string content_type;
        auto content_type_it = req.headers.find("content-type");
        if (content_type_it != req.headers.end()) {
          content_type = content_type_it->second;
          boost::algorithm::to_lower(content_type);
        }
        std::string username;
        std::string password;
        bool looks_like_ibm = false;
        // Check if auth was provided by a payload
        if (content_type == "application/json") {
          auto login_credentials =
              nlohmann::json::parse(req.body, nullptr, false);
          if (login_credentials.is_discarded()) {
            res.code = 400;
            res.end();
            return;
          }
          // check for username/password in the root object
          // THis method is how intel APIs authenticate
          auto user_it = login_credentials.find("username");
          auto pass_it = login_credentials.find("password");
          if (user_it != login_credentials.end() &&
              pass_it != login_credentials.end()) {
            username = user_it->get<const std::string>();
            password = pass_it->get<const std::string>();
          } else {
            // Openbmc appears to push a data object that contains the same
            // keys (username and password), attempt to use that
            auto data_it = login_credentials.find("data");
            if (data_it != login_credentials.end()) {
              // Some apis produce an array of value ["username",
              // "password"]
              if (data_it->is_array()) {
                if (data_it->size() == 2) {
                  username = (*data_it)[0].get<const std::string>();
                  password = (*data_it)[1].get<const std::string>();
                  looks_like_ibm = true;
                }
              } else if (data_it->is_object()) {
                auto user_it = data_it->find("username");
                auto pass_it = data_it->find("password");
                if (user_it != data_it->end() && pass_it != data_it->end()) {
                  username = user_it->get<const std::string>();
                  password = pass_it->get<const std::string>();
                }
              }
            }
          }
        } else {
          // check if auth was provided as a query string
          auto user_it = req.headers.find("username");
          auto pass_it = req.headers.find("password");
          if (user_it != req.headers.end() && pass_it != req.headers.end()) {
            username = user_it->second;
            password = pass_it->second;
          }
        }

        if (!username.empty() && !password.empty()) {
          if (!pam_authenticate_user(username, password)) {
            res.code = 401;
          } else {
            auto& context =
                app.template get_context<PersistentData::Middleware>(req);
            auto& session_store = context.sessions;
            auto& session = session_store->generate_user_session(username);

            if (looks_like_ibm) {
              // IBM requires a very specific login structure, and doesn't
              // actually look at the status code.  TODO(ed).... Fix that
              // upstream
              nlohmann::json ret{{"data", "User '" + username + "' logged in"},
                                 {"message", "200 OK"},
                                 {"status", "ok"}};
              res.add_header(
                  "Set-Cookie",
                  "SESSION=" + session.session_token + "; Secure; HttpOnly");
              res.add_header("Set-Cookie", "XSRF-TOKEN=" + session.csrf_token);
              res.write(ret.dump());
            } else {
              // if content type is json, assume json token
              nlohmann::json ret{{"token", session.session_token}};

              res.write(ret.dump());
              res.add_header("Content-Type", "application/json");
            }
          }

        } else {
          res.code = 400;
        }
        res.end();
      });
}
}  // namespaec TokenAuthorization
}  // namespace crow
