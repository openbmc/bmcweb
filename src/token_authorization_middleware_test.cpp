#include "token_authorization_middleware.hpp"
#include <crow/app.h>
#include "gtest/gtest.h"


// Tests that Base64 basic strings work
TEST(Authentication, TestBasicReject)
{
    crow::App<crow::TokenAuthorizationMiddleware> app;
    crow::request req;
    crow::response res;
    app.handle(req, res);
    ASSERT_EQ(res.code, 400);


    crow::App<crow::TokenAuthorizationMiddleware> app;
    decltype(app)::server_t server(&app, "127.0.0.1", 45451);
    CROW_ROUTE(app, "/")([&](const crow::request& req)
    {
        app.get_context<NullMiddleware>(req);
        app.get_context<NullSimpleMiddleware>(req);
        return "";
    });
}


