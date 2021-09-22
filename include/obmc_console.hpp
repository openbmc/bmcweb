#pragma once

#include <app.hpp>
#include <include/obmc_websocket.hpp>

namespace crow
{
namespace obmc_console
{
std::shared_ptr<obmc_websocket::Websocket> consoleWebSocket;

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/console0")
        .privileges({{"ConfigureComponents", "ConfigureManager"}})
        .websocket()
        .onopen([](crow::websocket::Connection& conn) {
            BMCWEB_LOG_DEBUG << "Connection " << &conn << " opened";

            const std::string consoleName("\0obmc-console", 13);
            if (consoleWebSocket)
            {
                consoleWebSocket->addConnection(conn);
            }
            else
            {
                consoleWebSocket = std::make_shared<obmc_websocket::Websocket>(
                    consoleName, conn);
                consoleWebSocket->addConnection(conn);
                consoleWebSocket->connect();
            }
        })
        .onclose(
            [](crow::websocket::Connection& conn, const std::string& reason) {
        BMCWEB_LOG_INFO << "Closing websocket. Reason: " << reason;
        // removeConnection return true if this is last connection.
        if (consoleWebSocket == nullptr &&
            consoleWebSocket->removeConnection(conn))
        {
            consoleWebSocket = nullptr;
        }
        })
        .onmessage([]([[maybe_unused]] crow::websocket::Connection& conn,
                      const std::string& data, [[maybe_unused]] bool isBinary) {
            consoleWebSocket->inputBuffer += data;
            consoleWebSocket->doWrite();
        });
}
} // namespace obmc_console
} // namespace crow
