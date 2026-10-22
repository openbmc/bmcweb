// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace persistent_data
{
struct UserSession;
} // namespace persistent_data

namespace crow
{
namespace websocket
{
struct Connection;
} // namespace websocket

namespace sse_socket
{
struct Connection;
} // namespace sse_socket

// Websockets and server-sent event streams are authorized once, when they
// open.  They are kept here with the user and session that opened them, so
// that they can be closed when that user loses access.
class ConnectionManager
{
  public:
    static ConnectionManager& getInstance();

    ConnectionManager(const ConnectionManager&) = delete;
    ConnectionManager(ConnectionManager&&) = delete;
    ConnectionManager& operator=(const ConnectionManager&) = delete;
    ConnectionManager& operator=(ConnectionManager&&) = delete;
    ~ConnectionManager() = default;

    // A connection opened without a session (authentication disabled) is not
    // kept.
    void add(const std::shared_ptr<persistent_data::UserSession>& session,
             const std::shared_ptr<websocket::Connection>& conn);
    void add(const std::shared_ptr<persistent_data::UserSession>& session,
             const std::shared_ptr<sse_socket::Connection>& conn);

    // Closes the connections of the user, except those opened by the session
    // keepSessionId.
    void closeUserConnections(std::string_view username,
                              std::string_view keepSessionId);

  private:
    ConnectionManager() = default;

    template <typename Conn>
    struct UserConnection
    {
        std::string username;
        std::string sessionId;
        std::weak_ptr<Conn> conn;
    };

    template <typename Conn>
    static void addTo(
        std::vector<UserConnection<Conn>>& connections,
        const std::shared_ptr<persistent_data::UserSession>& session,
        const std::shared_ptr<Conn>& conn);

    template <typename Conn>
    static void closeIn(std::vector<UserConnection<Conn>>& connections,
                        std::string_view username,
                        std::string_view keepSessionId);

    std::vector<UserConnection<websocket::Connection>> websockets;
    std::vector<UserConnection<sse_socket::Connection>> sseConnections;
};

} // namespace crow
