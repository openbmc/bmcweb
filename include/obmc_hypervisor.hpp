#pragma once

#include <app.hpp>
#include <include/obmc_websocket.hpp>

namespace crow
{
namespace obmc_hypervisor
{

static std::shared_ptr<obmc_websocket::Websocket> hyepWebSocket;

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/console1")
        .privileges({{"ConfigureComponents", "ConfigureManager"}})
        .websocket()
        .onopen([](crow::websocket::Connection& conn) {
            BMCWEB_LOG_DEBUG << "Connection " << &conn << " opened";

            const std::string consoleName("\0obmc-console.hypervisor", 24);
            if (hyepWebSocket)
            {
                hyepWebSocket->addConnection(conn);
            }
            else
            {
                hyepWebSocket = std::make_shared<obmc_websocket::Websocket>(
                    consoleName, conn);
                hyepWebSocket->connect();
            }
        })
        .onclose([](crow::websocket::Connection& conn,
                    [[maybe_unused]] const std::string& reason) {
            BMCWEB_LOG_INFO << "Closing websocket. Reason: " << reason;

            if (hyepWebSocket && hyepWebSocket->removeConnection(conn))
            {
                hyepWebSocket = nullptr;
            }
        })
        .onmessage([]([[maybe_unused]] crow::websocket::Connection& conn,
                      const std::string& data, [[maybe_unused]] bool isBinary) {
            hyepWebSocket->inputBuffer += data;
            hyepWebSocket->doWrite();
        });
}

} // namespace obmc_hypervisor
} // namespace crow
