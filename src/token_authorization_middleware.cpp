#include <unordered_map>

#include <boost/algorithm/string/predicate.hpp>

#include <token_authorization_middleware.hpp>

#include <base64.hpp>

namespace crow {

using random_bytes_engine = std::independent_bits_engine<std::default_random_engine, CHAR_BIT, unsigned char>;


void TokenAuthorizationMiddleware::before_handle(crow::request& req, response& res, context& ctx) {
  
  auto return_unauthorized = [&req, &res]() {
    res.code = 401;
    res.end();
  };
  if (req.url == "/" || boost::starts_with(req.url, "/static/")){
    //TODO this is total hackery to allow the login page to work before the user
    // is authenticated.  Also, it will be quite slow for all pages instead of
    // a one time hit for the whitelist entries.
    // Ideally, this should be done in the url router handler, with tagged routes
    // for the whitelist entries.
    return;
  }

  
  if (req.url == "/login") {
    if (req.method != HTTPMethod::POST){
      return_unauthorized();
      return;
    } else {
      auto login_credentials = crow::json::load(req.body);
      if (!login_credentials){
        return_unauthorized();
        return;
      }
      auto username = login_credentials["username"].s();
      auto password = login_credentials["password"].s();

      if (username == "dude" && password == "dude"){
        std::random_device rand;
        random_bytes_engine rbe;
        std::string token('a', 20);
        std::generate(begin(token), end(token), std::ref(rbe));
        std::string encoded_token;
        base64::base64_encode(token, encoded_token);
        ctx.auth_token = encoded_token;
        this->auth_token2 = encoded_token;

      } else {
        return_unauthorized();
        return;
      }
    }
    
  } else if (req.url == "/logout") {
    this->auth_token2 = "";
  } else { // Normal, non login, non static file request
    // Check to make sure we're logged in
    if (this->auth_token2.empty()){
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

    //todo, use span here instead of constructing a new string
    if (auth_header.substr(6) != this->auth_token2){
      return_unauthorized();
      return;
    }
  }
}

void TokenAuthorizationMiddleware::after_handle(request& /*req*/, response& res, context& ctx) {
  
}
}
