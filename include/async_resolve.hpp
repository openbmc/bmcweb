#pragma once
#ifdef BMCWEB_DBUS_DNS_RESOLVER
#include "dbus_singleton.hpp"
#include "logging.hpp"

#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/basic_endpoint.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <sdbusplus/message.hpp>

#include <charconv>
#include <iostream>
#include <memory>

namespace async_resolve
{

inline bool endpointFromResolveTuple(const std::vector<uint8_t>& ipAddress,
                                     boost::asio::ip::tcp::endpoint& endpoint)
{
    if (ipAddress.size() == 4) // ipv4 address
    {
        BMCWEB_LOG_DEBUG("ipv4 address");
        boost::asio::ip::address_v4 ipv4Addr(
            {ipAddress[0], ipAddress[1], ipAddress[2], ipAddress[3]});
        endpoint.address(ipv4Addr);
    }
    else if (ipAddress.size() == 16) // ipv6 address
    {
        BMCWEB_LOG_DEBUG("ipv6 address");
        boost::asio::ip::address_v6 ipv6Addr(
            {ipAddress[0], ipAddress[1], ipAddress[2], ipAddress[3],
             ipAddress[4], ipAddress[5], ipAddress[6], ipAddress[7],
             ipAddress[8], ipAddress[9], ipAddress[10], ipAddress[11],
             ipAddress[12], ipAddress[13], ipAddress[14], ipAddress[15]});
        endpoint.address(ipv6Addr);
    }
    else
    {
        BMCWEB_LOG_ERROR("Resolve failed to fetch the IP address");
        return false;
    }
    return true;
}

class Resolver
{
  public:
    // unused io param used to keep interface identical to
    // boost::asio::tcp:::resolver
    explicit Resolver(boost::asio::io_context& /*io*/) {}

    ~Resolver() = default;

    Resolver(const Resolver&) = delete;
    Resolver(Resolver&&) = delete;
    Resolver& operator=(const Resolver&) = delete;
    Resolver& operator=(Resolver&&) = delete;

    using results_type = std::vector<boost::asio::ip::tcp::endpoint>;

    template <typename ResolveHandler>
    // This function is kept using snake case so that it is interoperable with
    // boost::asio::ip::tcp::resolver
    // NOLINTNEXTLINE(readability-identifier-naming)
    void async_resolve(std::string_view host, std::string_view port,
                       ResolveHandler&& handler)
    {
        BMCWEB_LOG_DEBUG("Trying to resolve: {}:{}", host, port);

        uint16_t portNum = 0;

        auto it = std::from_chars(&*port.begin(), &*port.end(), portNum);
        if (it.ec != std::errc())
        {
            BMCWEB_LOG_ERROR("Failed to get the Port");
            handler(std::make_error_code(std::errc::invalid_argument),
                    results_type{});

            return;
        }

        uint64_t flag = 0;
        crow::connections::systemBus->async_method_call(
            [host{std::string(host)}, portNum,
             handler = std::forward<ResolveHandler>(handler)](
                const boost::system::error_code& ec,
                const std::vector<
                    std::tuple<int32_t, int32_t, std::vector<uint8_t>>>& resp,
                const std::string& hostName, const uint64_t flagNum) {
            results_type endpointList;
            if (ec)
            {
                BMCWEB_LOG_ERROR("Resolve failed: {}", ec.message());
                handler(ec, endpointList);
                return;
            }
            BMCWEB_LOG_DEBUG("ResolveHostname returned: {}:{}", hostName,
                             flagNum);
            // Extract the IP address from the response
            for (const std::tuple<int32_t, int32_t, std::vector<uint8_t>>&
                     resolveList : resp)
            {
                boost::asio::ip::tcp::endpoint endpoint;
                endpoint.port(portNum);
                if (!endpointFromResolveTuple(std::get<2>(resolveList),
                                              endpoint))
                {
                    boost::system::error_code ecErr = make_error_code(
                        boost::system::errc::address_not_available);
                    handler(ecErr, endpointList);
                }
                BMCWEB_LOG_DEBUG("resolved endpoint is : {}",
                                 endpoint.address().to_string());
                endpointList.push_back(endpoint);
            }
            // All the resolved data is filled in the endpointList
            handler(ec, endpointList);
        },
            "org.freedesktop.resolve1", "/org/freedesktop/resolve1",
            "org.freedesktop.resolve1.Manager", "ResolveHostname", 0, host,
            AF_UNSPEC, flag);
    }
};

} // namespace async_resolve
#endif
