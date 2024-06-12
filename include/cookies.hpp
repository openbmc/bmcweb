#pragma once

#include "http_response.hpp"
#include "sessions.hpp"

namespace bmcweb
{

inline void setSessionCookies(crow::Response& res,
                              const persistent_data::UserSession& session)
{
    res.addHeader(boost::beast::http::field::set_cookie,
                  "XSRF-TOKEN=" + session.csrfToken +
                      "; Path=/; SameSite=Strict; Secure");
    res.addHeader(boost::beast::http::field::set_cookie,
                  "SESSION=" + session.sessionToken +
                      "; Path=/; SameSite=Strict; Secure; HttpOnly");
}

inline void clearSessionCookies(crow::Response& res)
{
    res.addHeader(boost::beast::http::field::set_cookie,
                  "SESSION="
                  "; Path=/; SameSite=Strict; Secure; HttpOnly; "
                  "expires=Thu, 01 Jan 1970 00:00:00 GMT");
    res.addHeader("Clear-Site-Data", R"("cache","cookies","storage")");
}

} // namespace bmcweb
