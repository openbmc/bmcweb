#pragma once
#include "app.hpp"
#include "dbus_utility.hpp"
#include "logging.hpp"
#include "websocket.hpp"

#include <boost/container/flat_set.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace crow
{
namespace ipmi_https
{

using SessionMap = boost::container::flat_set<crow::websocket::Connection*>;

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static SessionMap sessions;

struct IpmiRqHeader
{
    uint8_t netfn;
    uint8_t lun;
    uint8_t cmd;
    uint8_t target_cmd;
    uint16_t data_len;
};

inline void doIPMIResponse(crow::websocket::Connection& conn, uint8_t cc,
                           const std::vector<uint8_t>& data)
{
    std::string payload;

    payload.push_back(static_cast<char>(cc));
    payload.insert(payload.end(), data.begin(), data.end());

    conn.sendBinary(payload);
}

inline void onMessage(crow::websocket::Connection& conn,
                      const std::string& data, bool /*isBinary*/)
{
    const auto session = sessions.find(&conn);
    if (session == sessions.end())
    {
        BMCWEB_LOG_DEBUG("Ipmi https: Can not find ipmi https session!");
        return;
    }

    if (data.empty() || data.size() < sizeof(struct IpmiRqHeader))
    {
        // Invalid command
        const uint8_t cc = 0xc1;
        doIPMIResponse(conn, cc, {});
        return;
    }

    bool ipmiHttps = true;

    std::string userRole = conn.getUserRole();
    int privilege = 0;
    if (userRole == "priv-admin")
    {
        privilege = 4;
    }
    else if (userRole == "priv-operator")
    {
        privilege = 3;
    }
    else if (userRole == "priv-user")
    {
        privilege = 2;
    }

    uint32_t sessionId =
        static_cast<uint32_t>(std::hash<crow::websocket::Connection*>{}(&conn));

    std::map<std::string, dbus::utility::IPMIValue> options = {
        {"privilege", dbus::utility::IPMIValue(privilege)},
        {"currentSessionId", dbus::utility::IPMIValue(sessionId)},
        {"ipmiHttps", dbus::utility::IPMIValue(ipmiHttps)},
    };

    struct IpmiRqHeader header{};
    header.netfn = data[0] & 0x3F;
    header.lun = static_cast<uint8_t>(data[0] >> 6);
    header.cmd = static_cast<uint8_t>(data[1]);
    header.target_cmd = static_cast<uint8_t>(data[2]);
    header.data_len =
        static_cast<uint16_t>((static_cast<uint16_t>(data[3]) << 8) | data[4]);

    std::vector<uint8_t> req;
    if (header.data_len > 0)
    {
        req.resize(header.data_len);
        for (size_t i = 0; i < header.data_len; i++)
        {
            req[i] = static_cast<uint8_t>(data[6 + i]);
        }
    }

    dbus::utility::callIPMI(
        header.netfn, header.lun, header.cmd, req, options,
        [&conn](const boost::system::error_code& ec,
                const dbus::utility::IpmiDbusRspType& res) {
            const auto ipmiSession = sessions.find(&conn);
            if (ipmiSession == sessions.end())
            {
                BMCWEB_LOG_DEBUG(
                    "Ipmi https: Can not find ipmi https session!");
                return;
            }

            if (ec)
            {
                const uint8_t cc = 0xff;
                doIPMIResponse(conn, cc, {});
                BMCWEB_LOG_ERROR("Error in IPMI call: {}", ec.message());
                return;
            }

            const uint8_t& cc = std::get<3>(res);
            const std::vector<uint8_t>& responseData = std::get<4>(res);

            doIPMIResponse(conn, cc, responseData);
        });
}

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/ipmi")
        .privileges({{"Login"}})
        .websocket()
        .onopen([&](crow::websocket::Connection& conn) {
            BMCWEB_LOG_DEBUG("Connection {} opened", logPtr(&conn));
            sessions.insert(&conn);
        })
        .onclose([&](crow::websocket::Connection& conn, const std::string&) {
            sessions.erase(&conn);
        })
        .onmessage(onMessage);
}
} // namespace ipmi_https
} // namespace crow
