#pragma once
#include "app.hpp"
#include "websocket.hpp"

#include <string>

namespace crow
{
namespace ipmi_https
{

inline void onMessage(crow::websocket::Connection& /*unused*/ /*unused*/,
                      const std::string& /*unused*/ /*unused*/,
                      bool /*unused*/ /*unused*/)
{
    // TODO
}

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/ipmi")
        .privileges({{"Login"}})
        .websocket()
        .onopen([&](crow::websocket::Connection&) {
            // TODO
        })
        .onclose([&](crow::websocket::Connection&, const std::string&) {
            // TODO
        })
        .onmessage(onMessage);
}

} // namespace ipmi_https
} // namespace crow
