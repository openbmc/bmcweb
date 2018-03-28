#pragma once

#include <pam_authenticate.hpp>
#include <persistent_data_middleware.hpp>
#include <webassets.hpp>
#include <random>
#include <crow/app.h>
#include <crow/http_codes.h>
#include <crow/http_request.h>
#include <crow/http_response.h>
#include <boost/bimap.hpp>
#include <boost/container/flat_set.hpp>

namespace crow {

namespace TokenAuthorization {

class Middleware {
 public:
  struct context {
    const crow::PersistentData::UserSession* session;
  };

  void before_handle(crow::request& req, response& res, context& ctx) {
    if (is_on_whitelist(req)) {
      return;
    }

    ctx.session = perform_xtoken_auth(req);

    if (ctx.session == nullptr) {
      ctx.session = perform_cookie_auth(req);
    }

    const std::string& auth_header = req.get_header_value("Authorization");
    // Reject any kind of auth other than basic or token
    if (ctx.session == nullptr && boost::starts_with(auth_header, "Token ")) {
      ctx.session = perform_token_auth(auth_header);
    }

    if (ctx.session == nullptr && boost::starts_with(auth_header, "Basic ")) {
      ctx.session = perform_basic_auth(auth_header);
    }

    if (ctx.session == nullptr) {
      CROW_LOG_WARNING << "[AuthMiddleware] authorization failed";
      res.code = static_cast<int>(HttpRespCode::UNAUTHORIZED);
      res.add_header("WWW-Authenticate", "Basic");
      res.end();
      return;
    }

    // TODO get user privileges here and propagate it via MW context
    // else let the request continue unharmed
  }

  template <typename AllContext>
  void after_handle(request& req, response& res, context& ctx,
                    AllContext& allctx) {
    // TODO(ed) THis should really be handled by the persistent data middleware,
    // but because it is upstream, it doesn't have access to the session
    // information.  Should the data middleware persist the current user
    // session?
    if (ctx.session != nullptr &&
        ctx.session->persistence ==
            crow::PersistentData::PersistenceType::SINGLE_REQUEST) {
      PersistentData::session_store->remove_session(ctx.session);
    }
  }

 private:
  const crow::PersistentData::UserSession* perform_basic_auth(
      const std::string& auth_header) const {
    CROW_LOG_DEBUG << "[AuthMiddleware] Basic authentication";

    std::string auth_data;
    std::string param = auth_header.substr(strlen("Basic "));
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
    return &(PersistentData::session_store->generate_user_session(
        user, crow::PersistentData::PersistenceType::SINGLE_REQUEST));
  }

  const crow::PersistentData::UserSession* perform_token_auth(
      const std::string& auth_header) const {
    CROW_LOG_DEBUG << "[AuthMiddleware] Token authentication";

    std::string token = auth_header.substr(strlen("Token "));
    auto session = PersistentData::session_store->login_session_by_token(token);
    return session;
  }

  const crow::PersistentData::UserSession* perform_xtoken_auth(
      const crow::request& req) const {
    CROW_LOG_DEBUG << "[AuthMiddleware] X-Auth-Token authentication";

    const std::string& token = req.get_header_value("X-Auth-Token");
    if (token.empty()) {
      return nullptr;
    }
    auto session = PersistentData::session_store->login_session_by_token(token);
    return session;
  }

  const crow::PersistentData::UserSession* perform_cookie_auth(
      const crow::request& req) const {
    CROW_LOG_DEBUG << "[AuthMiddleware] Cookie authentication";

    auto& cookie_value = req.get_header_value("Cookie");
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
    std::string auth_key =
        cookie_value.substr(start_index, end_index - start_index);

    const crow::PersistentData::UserSession* session =
        PersistentData::session_store->login_session_by_token(auth_key);
    if (session == nullptr) {
      return nullptr;
    }

    // RFC7231 defines methods that need csrf protection
    if (req.method != "GET"_method) {
      const std::string& csrf = req.get_header_value("X-XSRF-TOKEN");
      // Make sure both tokens are filled
      if (csrf.empty() || session->csrf_token.empty()) {
        return nullptr;
      }
      // Reject if csrf token not available
      if (csrf != session->csrf_token) {
        return nullptr;
      }
    }
    return session;
  }

  // checks if request can be forwarded without authentication
  bool is_on_whitelist(const crow::request& req) const {
    // it's allowed to GET root node without authentication
    if ("GET"_method == req.method) {
      if (req.url == "/redfish/v1") {
        return true;
      } else if (crow::webassets::routes.find(req.url) !=
                 crow::webassets::routes.end()) {
        return true;
      }
    }

    // it's allowed to POST on session collection & login without authentication
    if ("POST"_method == req.method) {
      if ((req.url == "/redfish/v1/SessionService/Sessions") ||
          (req.url == "/login") || (req.url == "/logout")) {
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
        std::string content_type;
        auto content_type_it = req.headers.find("content-type");
        if (content_type_it != req.headers.end()) {
          content_type = content_type_it->second;
          boost::algorithm::to_lower(content_type);
        }
        const std::string* username;
        const std::string* password;
        bool looks_like_ibm = false;

        // This object needs to be declared at this scope so the strings within
        // it are not destroyed before we can use them
        nlohmann::json login_credentials;
        // Check if auth was provided by a payload
        if (content_type == "application/json") {
          login_credentials = nlohmann::json::parse(req.body, nullptr, false);
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
            username = user_it->get_ptr<const std::string*>();
            password = pass_it->get_ptr<const std::string*>();
          } else {
            // Openbmc appears to push a data object that contains the same
            // keys (username and password), attempt to use that
            auto data_it = login_credentials.find("data");
            if (data_it != login_credentials.end()) {
              // Some apis produce an array of value ["username",
              // "password"]
              if (data_it->is_array()) {
                if (data_it->size() == 2) {
                  username = (*data_it)[0].get_ptr<const std::string*>();
                  password = (*data_it)[1].get_ptr<const std::string*>();
                  looks_like_ibm = true;
                }
              } else if (data_it->is_object()) {
                auto user_it = data_it->find("username");
                auto pass_it = data_it->find("password");
                if (user_it != data_it->end() && pass_it != data_it->end()) {
                  username = user_it->get_ptr<const std::string*>();
                  password = pass_it->get_ptr<const std::string*>();
                }
              }
            }
          }
        } else {
          // check if auth was provided as a query string
          auto user_it = req.headers.find("username");
          auto pass_it = req.headers.find("password");
          if (user_it != req.headers.end() && pass_it != req.headers.end()) {
            username = &user_it->second;
            password = &pass_it->second;
          }
        }

        if (username != nullptr && !username->empty() && password != nullptr &&
            !password->empty()) {
          if (!pam_authenticate_user(*username, *password)) {
            res.code = res.code = static_cast<int>(HttpRespCode::UNAUTHORIZED);
          } else {
            auto& session =
                PersistentData::session_store->generate_user_session(*username);

            if (looks_like_ibm) {
              // IBM requires a very specific login structure, and doesn't
              // actually look at the status code.  TODO(ed).... Fix that
              // upstream
              res.json_value = {{"data", "User '" + *username + "' logged in"},
                                {"message", "200 OK"},
                                {"status", "ok"}};
              res.add_header("Set-Cookie", "XSRF-TOKEN=" + session.csrf_token);
              res.add_header("Set-Cookie", "SESSION=" + session.session_token +
                                               "; Secure; HttpOnly");
            } else {
              // if content type is json, assume json token
              res.json_value = {{"token", session.session_token}};
            }
          }

        } else {
          res.code = static_cast<int>(HttpRespCode::BAD_REQUEST);
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
            res.code = static_cast<int>(HttpRespCode::OK);
            res.end();
            return;

          });
}
}  // namespace TokenAuthorization
}  // namespace crow
