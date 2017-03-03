#include <unordered_map>

#include <boost/algorithm/string/predicate.hpp>

#include <token_authorization_middleware.hpp>

namespace crow
{
    std::string TokenAuthorizationMiddleware::context::get_cookie(const std::string& key)
    {
        if (cookie_sessions.count(key))
            return cookie_sessions[key];
        return {};
    }

    void TokenAuthorizationMiddleware::context::set_cookie(const std::string& key, const std::string& value)
    {
        cookies_to_push_to_client.emplace(key, value);
    }
    

    void TokenAuthorizationMiddleware::before_handle(crow::request& req, response& res, context& ctx)
    {
        return;
        if (req.url == "/login"){
            return;
        }

        // Check for an authorization header, reject if not present
        if (req.headers.count("Authorization") != 1) {
            res.code = 400;
            res.end();
            return;
        }
        std::string auth_header = req.get_header_value("Authorization");
        // If the user is attempting any kind of auth other than token, reject
        if (!boost::starts_with(auth_header, "Token ")) {
            res.code = 400;
            res.end();
        }
    }

    void TokenAuthorizationMiddleware::after_handle(request& /*req*/, response& res, context& ctx)
    {
        for (auto& cookie : ctx.cookies_to_push_to_client) {
            res.add_header("Set-Cookie", cookie.first + "=" + cookie.second);
        }
    }

}