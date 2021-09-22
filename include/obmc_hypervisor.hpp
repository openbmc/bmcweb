#pragma once

#include <app.hpp>
#include <include/obmc_websocket.hpp>

namespace crow
{
namespace obmc_hypervisor
{

static std::shared_ptr<obmc_websocket::Websocket> hypervisorWebSocket;

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/console1")
        .privileges({{"ConfigureComponents", "ConfigureManager"}})
        .websocket()
        .onopen([](crow::websocket::Connection& conn) {
            BMCWEB_LOG_DEBUG << "Connection " << &conn << " opened";

            const std::string consoleName("\0obmc-console.hypervisor", 24);
            if (hypervisorWebSocket)
            {
                hypervisorWebSocket->addConnection(conn);
            }
            else
            {
                hypervisorWebSocket =
                    std::make_shared<obmc_websocket::Websocket>(consoleName,
                                                                conn);
                hypervisorWebSocket->addConnection(conn);
                hypervisorWebSocket->connect();
            }
        })
        .onclose(
            [](crow::websocket::Connection& conn, const std::string& reason) {
        BMCWEB_LOG_INFO << "Closing websocket. Reason: " << reason;

        if (hypervisorWebSocket == nullptr &&
            hypervisorWebSocket->removeConnection(conn))
        {
            hypervisorWebSocket = nullptr;
        }
        })
        .onmessage([]([[maybe_unused]] crow::websocket::Connection& conn,
                      const std::string& data, [[maybe_unused]] bool isBinary) {
            hypervisorWebSocket->inputBuffer += data;
            hypervisorWebSocket->doWrite();
        });
}

} // namespace obmc_hypervisor
} // namespace crow
