#pragma once

#include <pam_authenticate.hpp>
#include <persistent_data_middleware.hpp>
#include <webassets.hpp>
#include <random>
#include <crow/app.h>
#include <crow/common.h>
#include <crow/http_request.h>
#include <crow/http_response.h>
#include <boost/container/flat_set.hpp>

namespace crow {

namespace TokenAuthorization {

class Middleware {
 public:
  struct context {
    std::shared_ptr<crow::PersistentData::UserSession> session;
  };

  void before_handle(crow::request& req, response& res, context& ctx) {
    if (is_on_whitelist(req)) {
      return;
    }

    ctx.session = perform_xtoken_auth(req);
    if (ctx.session == nullptr) {
      ctx.session = perform_cookie_auth(req);
    }
    if (ctx.session == nullptr) {
      boost::string_view auth_header = req.get_header_value("Authorization");
      if (!auth_header.empty()) {
        // Reject any kind of auth other than basic or token
        if (boost::starts_with(auth_header, "Token ")) {
          ctx.session = perform_token_auth(auth_header);
        } else if (boost::starts_with(auth_header, "Basic ")) {
          ctx.session = perform_basic_auth(auth_header);
        }
      }
    }

    if (ctx.session == nullptr) {
      CROW_LOG_WARNING << "[AuthMiddleware] authorization failed";

      // If it's a browser connecting, don't send the HTTP authenticate header,
      // to avoid possible CSRF attacks with basic auth
      if (http_helpers::request_prefers_html(req)) {
        res.result(boost::beast::http::status::temporary_redirect);
        res.add_header("Location", "/#/login");
      } else {
        res.result(boost::beast::http::status::unauthorized);
        // only send the WWW-authenticate header if this isn't a xhr from the
        // browser.  most scripts,
        if (req.get_header_value("User-Agent").empty()) {
          res.add_header("WWW-Authenticate", "Basic");
        }
      }

      res.end();
      return;
    }

    // TODO get user privileges here and propagate it via MW context
    // else let the request continue unharmed
  }

  template <typename AllContext>
  void after_handle(request& req, response& res, context& ctx,
                    AllContext& allctx) {
    // TODO(ed) THis should really be handled by the persistent data
    // middleware, but because it is upstream, it doesn't have access to the
    // session information.  Should the data middleware persist the current
    // user session?
    if (ctx.session != nullptr &&
        ctx.session->persistence ==
            crow::PersistentData::PersistenceType::SINGLE_REQUEST) {
      PersistentData::session_store->remove_session(ctx.session);
    }
  }

 private:
  const std::shared_ptr<crow::PersistentData::UserSession> perform_basic_auth(
      boost::string_view auth_header) const {
    CROW_LOG_DEBUG << "[AuthMiddleware] Basic authentication";

    std::string auth_data;
    boost::string_view param = auth_header.substr(strlen("Basic "));
    if (!crow::utility::base64_decode(param, auth_data)) {
      return nullptr;
    }
    std::size_t separator = auth_data.find(':');
    if (separator == std::string::npos) {
      return nullptr;
    }

    std::string user = auth_data.substr(0, separator);
    separator += 1;
    if (separator > auth_data.size()) {
      return nullptr;
    }
    std::string pass = auth_data.substr(separator);

    CROW_LOG_DEBUG << "[AuthMiddleware] Authenticating user: " << user;

    if (!pam_authenticate_user(user, pass)) {
      return nullptr;
    }

    // TODO(ed) generate_user_session is a little expensive for basic
    // auth, as it generates some random identifiers that will never be
    // used.  This should have a "fast" path for when user tokens aren't
    // needed.
    // This whole flow needs to be revisited anyway, as we can't be
    // calling directly into pam for every request
    return PersistentData::session_store->generate_user_session(
        user, crow::PersistentData::PersistenceType::SINGLE_REQUEST);
  }

  const std::shared_ptr<crow::PersistentData::UserSession> perform_token_auth(
      boost::string_view auth_header) const {
    CROW_LOG_DEBUG << "[AuthMiddleware] Token authentication";

    boost::string_view token = auth_header.substr(strlen("Token "));
    auto session = PersistentData::session_store->login_session_by_token(token);
    return session;
  }

  const std::shared_ptr<crow::PersistentData::UserSession> perform_xtoken_auth(
      const crow::request& req) const {
    CROW_LOG_DEBUG << "[AuthMiddleware] X-Auth-Token authentication";

    boost::string_view token = req.get_header_value("X-Auth-Token");
    if (token.empty()) {
      return nullptr;
    }
    auto session = PersistentData::session_store->login_session_by_token(token);
    return session;
  }

  const std::shared_ptr<crow::PersistentData::UserSession> perform_cookie_auth(
      const crow::request& req) const {
    CROW_LOG_DEBUG << "[AuthMiddleware] Cookie authentication";

    boost::string_view cookie_value = req.get_header_value("Cookie");
    if (cookie_value.empty()) {
      return nullptr;
    }

    auto start_index = cookie_value.find("SESSION=");
    if (start_index == std::string::npos) {
      return nullptr;
    }
    start_index += sizeof("SESSION=") - 1;
    auto end_index = cookie_value.find(";", start_index);
    if (end_index == std::string::npos) {
      end_index = cookie_value.size();
    }
    boost::string_view auth_key =
        cookie_value.substr(start_index, end_index - start_index);

    const std::shared_ptr<crow::PersistentData::UserSession> session =
        PersistentData::session_store->login_session_by_token(auth_key);
    if (session == nullptr) {
      return nullptr;
    }
#ifndef BMCWEB_INSECURE_DISABLE_CSRF_PREVENTION
    // RFC7231 defines methods that need csrf protection
    if (req.method() != "GET"_method) {
      boost::string_view csrf = req.get_header_value("X-XSRF-TOKEN");
      // Make sure both tokens are filled
      if (csrf.empty() || session->csrf_token.empty()) {
        return nullptr;
      }
      // Reject if csrf token not available
      if (csrf != session->csrf_token) {
        return nullptr;
      }
    }
#endif
    return session;
  }

  // checks if request can be forwarded without authentication
  bool is_on_whitelist(const crow::request& req) const {
    // it's allowed to GET root node without authentica tion
    if ("GET"_method == req.method()) {
      if (req.url == "/redfish/v1" || req.url == "/redfish/v1/") {
        return true;
      } else if (crow::webassets::routes.find(std::string(req.url)) !=
                 crow::webassets::routes.end()) {
        return true;
      }
    }

    // it's allowed to POST on session collection & login without
    // authentication
    if ("POST"_method == req.method()) {
      if ((req.url == "/redfish/v1/SessionService/Sessions") ||
          (req.url == "/redfish/v1/SessionService/Sessions/") ||
          (req.url == "/login")) {
        return true;
      }
    }

    return false;
  }
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
        boost::string_view content_type = req.get_header_value("content-type");
        boost::string_view username;
        boost::string_view password;

        bool looks_like_ibm = false;

        // This object needs to be declared at this scope so the strings
        // within it are not destroyed before we can use them
        nlohmann::json login_credentials;
        // Check if auth was provided by a payload
        if (content_type == "application/json") {
          login_credentials = nlohmann::json::parse(req.body, nullptr, false);
          if (login_credentials.is_discarded()) {
            res.result(boost::beast::http::status::bad_request);
            res.end();
            return;
          }

          // check for username/password in the root object
          // THis method is how intel APIs authenticate
          nlohmann::json::iterator user_it = login_credentials.find("username");
          nlohmann::json::iterator pass_it = login_credentials.find("password");
          if (user_it != login_credentials.end() &&
              pass_it != login_credentials.end()) {
            const std::string* user_str =
                user_it->get_ptr<const std::string*>();
            const std::string* pass_str =
                pass_it->get_ptr<const std::string*>();
            if (user_str != nullptr && pass_str != nullptr) {
              username = *user_str;
              password = *pass_str;
            }
          } else {
            // Openbmc appears to push a data object that contains the same
            // keys (username and password), attempt to use that
            auto data_it = login_credentials.find("data");
            if (data_it != login_credentials.end()) {
              // Some apis produce an array of value ["username",
              // "password"]
              if (data_it->is_array()) {
                if (data_it->size() == 2) {
                  nlohmann::json::iterator user_it2 = data_it->begin();
                  nlohmann::json::iterator pass_it2 = data_it->begin() + 1;
                  looks_like_ibm = true;
                  if (user_it2 != data_it->end() &&
                      pass_it2 != data_it->end()) {
                    const std::string* user_str =
                        user_it2->get_ptr<const std::string*>();
                    const std::string* pass_str =
                        pass_it2->get_ptr<const std::string*>();
                    if (user_str != nullptr && pass_str != nullptr) {
                      username = *user_str;
                      password = *pass_str;
                    }
                  }
                }

              } else if (data_it->is_object()) {
                nlohmann::json::iterator user_it2 = data_it->find("username");
                nlohmann::json::iterator pass_it2 = data_it->find("password");
                if (user_it2 != data_it->end() && pass_it2 != data_it->end()) {
                  const std::string* user_str =
                      user_it2->get_ptr<const std::string*>();
                  const std::string* pass_str =
                      pass_it2->get_ptr<const std::string*>();
                  if (user_str != nullptr && pass_str != nullptr) {
                    username = *user_str;
                    password = *pass_str;
                  }
                }
              }
            }
          }
        } else {
          // check if auth was provided as a headers
          username = req.get_header_value("username");
          password = req.get_header_value("password");
        }

        if (!username.empty() && !password.empty()) {
          if (!pam_authenticate_user(username, password)) {
            res.result(boost::beast::http::status::unauthorized);
          } else {
            auto session =
                PersistentData::session_store->generate_user_session(username);

            if (looks_like_ibm) {
              // IBM requires a very specific login structure, and doesn't
              // actually look at the status code.  TODO(ed).... Fix that
              // upstream
              res.json_value = {
                  {"data", "User '" + std::string(username) + "' logged in"},
                  {"message", "200 OK"},
                  {"status", "ok"}};

              // Hack alert.  Boost beast by default doesn't let you declare
              // multiple headers of the same name, and in most cases this is
              // fine.  Unfortunately here we need to set the Session cookie,
              // which requires the httpOnly attribute, as well as the XSRF
              // cookie, which requires it to not have an httpOnly attribute.
              // To get the behavior we want, we simply inject the second
              // "set-cookie" string into the value header, and get the result
              // we want, even though we are technicaly declaring two headers
              // here.
              res.add_header("Set-Cookie",
                             "XSRF-TOKEN=" + session->csrf_token +
                                 "; Secure\r\nSet-Cookie: SESSION=" +
                                 session->session_token + "; Secure; HttpOnly");
            } else {
              // if content type is json, assume json token
              res.json_value = {{"token", session->session_token}};
            }
          }

        } else {
          res.result(boost::beast::http::status::bad_request);
        }
        res.end();
      });

  CROW_ROUTE(app, "/logout")
      .methods("POST"_method)(
          [&](const crow::request& req, crow::response& res) {
            auto& session =
                app.template get_context<TokenAuthorization::Middleware>(req)
                    .session;
            if (session != nullptr) {
              PersistentData::session_store->remove_session(session);
            }
            res.end();
            return;

          });
}
}  // namespace TokenAuthorization
}  // namespace crow
