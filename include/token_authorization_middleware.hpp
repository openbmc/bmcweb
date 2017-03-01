#pragma once

#include <crow/http_request.h>
#include <crow/http_response.h>

namespace crow
{
    struct TokenAuthorizationMiddleware {

        struct context {
            std::unordered_map<std::string, std::string> cookie_sessions;
            std::unordered_map<std::string, std::string> cookies_to_push_to_client;

            std::string get_cookie(const std::string& key);

            void set_cookie(const std::string& key, const std::string& value);
        };

        void before_handle(crow::request& req, response& res, context& ctx);

        void after_handle(request& req, response& res, context& ctx);
    };
}