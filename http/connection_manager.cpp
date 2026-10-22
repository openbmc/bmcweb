// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "connection_manager.hpp"

#include "server_sent_event.hpp"
#include "sessions.hpp"
#include "websocket.hpp"

#include <memory>
#include <string_view>
#include <vector>

namespace crow
{

ConnectionManager& ConnectionManager::getInstance()
{
    static ConnectionManager manager;
    return manager;
}

template <typename Conn>
void ConnectionManager::addTo(
    std::vector<UserConnection<Conn>>& connections,
    const std::shared_ptr<persistent_data::UserSession>& session,
    const std::shared_ptr<Conn>& conn)
{
    if (session == nullptr)
    {
        return;
    }
    std::erase_if(connections, [](const UserConnection<Conn>& entry) {
        return entry.conn.expired();
    });
    connections.emplace_back(session->username, session->uniqueId, conn);
}

template <typename Conn>
void ConnectionManager::closeIn(std::vector<UserConnection<Conn>>& connections,
                                std::string_view username,
                                std::string_view keepSessionId)
{
    auto it = connections.begin();
    while (it != connections.end())
    {
        if (it->username != username || it->sessionId == keepSessionId)
        {
            it++;
            continue;
        }
        std::shared_ptr<Conn> conn = it->conn.lock();
        it = connections.erase(it);
        if (conn != nullptr)
        {
            conn->close("User lost access");
        }
    }
}

void ConnectionManager::add(
    const std::shared_ptr<persistent_data::UserSession>& session,
    const std::shared_ptr<websocket::Connection>& conn)
{
    addTo(websockets, session, conn);
}

void ConnectionManager::add(
    const std::shared_ptr<persistent_data::UserSession>& session,
    const std::shared_ptr<sse_socket::Connection>& conn)
{
    addTo(sseConnections, session, conn);
}

void ConnectionManager::closeUserConnections(std::string_view username,
                                             std::string_view keepSessionId)
{
    closeIn(websockets, username, keepSessionId);
    closeIn(sseConnections, username, keepSessionId);
}

} // namespace crow
