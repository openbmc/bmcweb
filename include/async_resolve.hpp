#pragma once
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/basic_endpoint.hpp>
#include <sdbusplus/message.hpp>

#include <charconv>
#include <iostream>
#include <memory>

namespace crow
{

namespace async_resolve
{

class Resolver
{
  public:
    Resolver() = default;

    ~Resolver() = default;

    template <typename ResolveHandler>
    void asyncResolve(const std::string& host, const std::string& port,
                      ResolveHandler&& handler)
    {
        BMCWEB_LOG_DEBUG << "Trying to resolve: " << host << ":" << port;
        uint64_t flag = 0;
        crow::connections::systemBus->async_method_call(
            [host, port, handler{std::move(handler)}](
                const boost::system::error_code ec,
                const std::vector<
                    std::tuple<int32_t, int32_t, std::vector<uint8_t>>>& resp,
                const std::string& hostName, const uint64_t flagNum) {
                std::vector<boost::asio::ip::tcp::endpoint> endpointList;
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Resolve failed: " << ec.message();
                    handler(ec, endpointList);
                    return;
                }
                BMCWEB_LOG_DEBUG << "ResolveHostname returned: " << hostName
                                 << ":" << flagNum;
                // Extract the IP address from the response
                for (auto resolveList : resp)
                {
                    std::vector<uint8_t> ipAddress = std::get<2>(resolveList);
                    boost::asio::ip::tcp::endpoint endpoint;
                    if (ipAddress.size() == 4) // ipv4 address
                    {
                        BMCWEB_LOG_DEBUG << "ipv4 address";
                        boost::asio::ip::address_v4 ipv4Addr(
                            {ipAddress[0], ipAddress[1], ipAddress[2],
                             ipAddress[3]});
                        endpoint.address(ipv4Addr);
                    }
                    else if (ipAddress.size() == 16) // ipv6 address
                    {
                        BMCWEB_LOG_DEBUG << "ipv6 address";
                        boost::asio::ip::address_v6 ipv6Addr(
                            {ipAddress[0], ipAddress[1], ipAddress[2],
                             ipAddress[3], ipAddress[4], ipAddress[5],
                             ipAddress[6], ipAddress[7], ipAddress[8],
                             ipAddress[9], ipAddress[10], ipAddress[11],
                             ipAddress[12], ipAddress[13], ipAddress[14],
                             ipAddress[15]});
                        endpoint.address(ipv6Addr);
                    }
                    else
                    {
                        BMCWEB_LOG_ERROR
                            << "Resolve failed to fetch the IP address";
                        handler(ec, endpointList);
                        return;
                    }
                    uint16_t portNum;
                    auto it = std::from_chars(
                        port.data(), port.data() + port.size(), portNum);
                    if (it.ec != std::errc())
                    {
                        BMCWEB_LOG_ERROR << "Failed to get the Port";
                        handler(ec, endpointList);
                        return;
                    }
                    endpoint.port(portNum);
                    BMCWEB_LOG_DEBUG << "resolved endpoint is : " << endpoint;
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
} // namespace crow
