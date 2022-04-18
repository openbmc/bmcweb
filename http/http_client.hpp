/*
// Copyright (c) 2020 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#pragma once
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/basic_endpoint.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/version.hpp>
#include <boost/container/devector.hpp>
#include <include/async_resolve.hpp>

#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <queue>
#include <string>

namespace crow
{

static constexpr uint8_t maxPoolSize = 4;
static constexpr uint8_t maxRequestQueueSize = 50;
static constexpr unsigned int httpReadBodyLimit = 8192;

enum class ConnState
{
    initialized,
    resolveInProgress,
    resolveFailed,
    connectInProgress,
    connectFailed,
    connected,
    sendInProgress,
    sendFailed,
    recvInProgress,
    recvFailed,
    idle,
    closeInProgress,
    closed,
    suspended,
    terminated,
    abortConnection,
    retry
};

// We need to allow retry information to be set before a message has been sent
// and a connection pool has been created
struct RetryPolicyData
{
    uint32_t maxRetryAttempts = 5;
    uint32_t retryIntervalSecs = 0;
    std::string retryPolicyAction = "TerminateAfterRetries";
};
static std::unordered_map<std::string, RetryPolicyData> unconnectedRetryInfo;

struct SatelliteConfig
{
    std::string name;
    std::string host;
    uint16_t port = 0;
    std::string authType;
};
static std::unordered_map<std::string, SatelliteConfig> satelliteInfo;

// Match signals for adding Satellite Config
static std::unique_ptr<sdbusplus::bus::match::match> matchSatelliteSignalMonitor;

class ConnectionInfo : public std::enable_shared_from_this<ConnectionInfo>
{
  private:
    boost::beast::tcp_stream conn;
    boost::asio::steady_timer timer;
    boost::beast::http::request<boost::beast::http::string_body> req;
    std::optional<
        boost::beast::http::response_parser<boost::beast::http::string_body>>
        parser;
    crow::async_resolve::Resolver resolver;
    boost::beast::flat_static_buffer<httpReadBodyLimit> buffer;

    ConnState state = ConnState::initialized;
    uint32_t retryCount = 0;
    bool runningTimer = false;
    std::string subId;
    std::string host;
    uint16_t port = 0;
    uint32_t connId;
    std::string data;
    std::function<void(bool, uint32_t, Response&)> callback;

    // Retry policy for the larger connection pool
    const uint32_t& maxRetryAttempts;
    const uint32_t& retryIntervalSecs;
    const std::string& retryPolicyAction;

    friend class ConnectionPool;

    void doResolve()
    {
        state = ConnState::resolveInProgress;
        BMCWEB_LOG_DEBUG << "Trying to resolve: " << host << ":"
                         << std::to_string(port)
                         << ", id: " << std::to_string(connId);

        auto respHandler =
            [self(shared_from_this())](
                const boost::beast::error_code ec,
                const std::vector<boost::asio::ip::tcp::endpoint>&
                    endpointList) {
                if (ec || (endpointList.empty()))
                {
                    BMCWEB_LOG_ERROR << "Resolve failed: " << ec.message();
                    self->state = ConnState::resolveFailed;
                    self->waitAndRetry();
                    return;
                }
                BMCWEB_LOG_DEBUG << "Resolved " << self->host << ":"
                                 << std::to_string(self->port)
                                 << ", id: " << std::to_string(self->connId);
                self->doConnect(endpointList);
            };

        resolver.asyncResolve(host, port, std::move(respHandler));
    }

    void doConnect(
        const std::vector<boost::asio::ip::tcp::endpoint>& endpointList)
    {
        state = ConnState::connectInProgress;

        BMCWEB_LOG_DEBUG << "Trying to connect to: " << host << ":"
                         << std::to_string(port)
                         << ", id: " << std::to_string(connId);

        conn.expires_after(std::chrono::seconds(30));
        conn.async_connect(
            endpointList, [self(shared_from_this())](
                              const boost::beast::error_code ec,
                              const boost::asio::ip::tcp::endpoint& endpoint) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Connect "
                                     << endpoint.address().to_string() << ":"
                                     << std::to_string(endpoint.port())
                                     << ", id: " << std::to_string(self->connId)
                                     << " failed: " << ec.message();
                    self->state = ConnState::connectFailed;
                    self->waitAndRetry();
                    return;
                }
                BMCWEB_LOG_DEBUG
                    << "Connected to: " << endpoint.address().to_string() << ":"
                    << std::to_string(endpoint.port())
                    << ", id: " << std::to_string(self->connId);
                self->state = ConnState::connected;
                self->sendMessage();
            });
    }

    void sendMessage()
    {
        state = ConnState::sendInProgress;

        req.body() = data;
        req.prepare_payload();

        // Set a timeout on the operation
        conn.expires_after(std::chrono::seconds(30));

        // Send the HTTP request to the remote host
        boost::beast::http::async_write(
            conn, req,
            [self(shared_from_this())](const boost::beast::error_code& ec,
                                       const std::size_t& bytesTransferred) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "sendMessage() failed: "
                                     << ec.message();
                    self->state = ConnState::sendFailed;
                    self->waitAndRetry();
                    return;
                }
                BMCWEB_LOG_DEBUG << "sendMessage() bytes transferred: "
                                 << bytesTransferred;
                boost::ignore_unused(bytesTransferred);

                self->recvMessage();
            });
    }

    void recvMessage()
    {
        parser.emplace(std::piecewise_construct, std::make_tuple());
        parser->body_limit(httpReadBodyLimit);

        // Receive the HTTP response
        boost::beast::http::async_read(
            conn, buffer, *parser,
            [self(shared_from_this())](const boost::beast::error_code& ec,
                                       const std::size_t& bytesTransferred) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "recvMessage() failed: "
                                     << ec.message();
                    self->state = ConnState::recvFailed;
                    self->waitAndRetry();
                    return;
                }
                BMCWEB_LOG_DEBUG << "recvMessage() bytes transferred: "
                                 << bytesTransferred;
                BMCWEB_LOG_DEBUG << "recvMessage() data: "
                                 << self->parser->get().body();

                unsigned int respCode = self->parser->get().result_int();
                BMCWEB_LOG_DEBUG << "recvMessage() Header Response Code: "
                                 << respCode;

                // 2XX response is considered to be successful
                if ((respCode < 200) || (respCode >= 300))
                {
                    // The listener failed to receive the Sent-Event
                    BMCWEB_LOG_ERROR
                        << "recvMessage() Listener Failed to "
                           "receive Sent-Event. Header Response Code: "
                        << respCode;
                    self->state = ConnState::recvFailed;
                    self->waitAndRetry();
                    return;
                }

                // Send is successful
                // Reset the counter just in case this was after retrying
                self->retryCount = 0;

                // Keep the connection alive if server supports it
                // Else close the connection
                BMCWEB_LOG_DEBUG << "recvMessage() keepalive : "
                                 << self->parser->keep_alive();

                // Copy the response into a Response object so that it can be
                // processed by the callback function.
                Response res;
                res.stringResponse = self->parser->get();
                if (!self->parser->get().body().empty())
                {
                    res.jsonValue =
                        nlohmann::json::parse(self->parser->get().body());
                }
                self->callback(self->parser->keep_alive(), self->connId, res);
            });
    }

    void waitAndRetry()
    {
        if (retryCount >= maxRetryAttempts)
        {
            BMCWEB_LOG_ERROR << "Maximum number of retries reached.";
            BMCWEB_LOG_DEBUG << "Retry policy: " << retryPolicyAction;
            Response res;
            redfish::messages::internalError(res);
            if (retryPolicyAction == "TerminateAfterRetries")
            {
                // TODO: delete subscription
                state = ConnState::terminated;
                callback(false, connId, res);
            }
            if (retryPolicyAction == "SuspendRetries")
            {
                state = ConnState::suspended;
                callback(false, connId, res);
            }
            // Reset the retrycount to zero so that client can try connecting
            // again if needed
            retryCount = 0;
            return;
        }

        if (runningTimer)
        {
            BMCWEB_LOG_DEBUG << "Retry timer is already running.";
            return;
        }
        runningTimer = true;

        retryCount++;

        BMCWEB_LOG_DEBUG << "Attempt retry after " << retryIntervalSecs
                         << " seconds. RetryCount = " << retryCount;
        timer.expires_after(std::chrono::seconds(retryIntervalSecs));
        timer.async_wait(
            [self(shared_from_this())](const boost::system::error_code ec) {
                if (ec == boost::asio::error::operation_aborted)
                {
                    BMCWEB_LOG_DEBUG
                        << "async_wait failed since the operation is aborted"
                        << ec.message();
                }
                else if (ec)
                {
                    BMCWEB_LOG_ERROR << "async_wait failed: " << ec.message();
                    // Ignore the error and continue the retry loop to attempt
                    // sending the event as per the retry policy
                }
                self->runningTimer = false;

                // Let's close the connection and restart from resolve.
                self->doCloseAndRetry();
            });
    }

    void doClose()
    {
        state = ConnState::closeInProgress;
        boost::beast::error_code ec;
        conn.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        conn.close();

        // not_connected happens sometimes so don't bother reporting it.
        if (ec && ec != boost::beast::errc::not_connected)
        {
            BMCWEB_LOG_ERROR << host << ":" << std::to_string(port)
                             << ", id: " << std::to_string(connId)
                             << "shutdown failed: " << ec.message();
            return;
        }
        BMCWEB_LOG_DEBUG << host << ":" << std::to_string(port)
                         << ", id: " << std::to_string(connId)
                         << " closed gracefully";
        if ((state != ConnState::suspended) && (state != ConnState::terminated))
        {
            state = ConnState::closed;
        }
    }

    void doCloseAndRetry()
    {
        state = ConnState::closeInProgress;
        boost::beast::error_code ec;
        conn.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        conn.close();

        // not_connected happens sometimes so don't bother reporting it.
        if (ec && ec != boost::beast::errc::not_connected)
        {
            BMCWEB_LOG_ERROR << host << ":" << std::to_string(port)
                             << ", id: " << std::to_string(connId)
                             << "shutdown failed: " << ec.message();
            return;
        }
        BMCWEB_LOG_DEBUG << host << ":" << std::to_string(port)
                         << ", id: " << std::to_string(connId)
                         << " closed gracefully";
        if ((state != ConnState::suspended) && (state != ConnState::terminated))
        {
            // Now let's try to resend the data
            state = ConnState::retry;
            this->doResolve();
        }
    }

  public:
    explicit ConnectionInfo(boost::asio::io_context& ioc, const std::string& id,
                            const std::string& destIP, const uint16_t destPort,
                            const std::string& destUri,
                            const boost::beast::http::fields& httpHeader,
                            const unsigned int connId,
                            const uint32_t& maxRetryAttempts,
                            const uint32_t& retryIntervalSecs,
                            const std::string& retryPolicyAction) :
        conn(ioc),
        timer(ioc),
        req(boost::beast::http::verb::post, destUri, 11, "", httpHeader),
        subId(id), host(destIP), port(destPort), connId(connId),
        maxRetryAttempts(maxRetryAttempts),
        retryIntervalSecs(retryIntervalSecs),
        retryPolicyAction(retryPolicyAction)
    {
        req.set(boost::beast::http::field::host, host);
        req.keep_alive(true);
    }
};

class ConnectionPool
{
  private:
    boost::asio::io_context& ioc;
    const std::string id;
    const std::string destIP;
    const uint16_t destPort;
    const std::string destUri;
    const boost::beast::http::fields& httpHeader;
    std::vector<std::shared_ptr<ConnectionInfo>> connections;
    boost::container::devector<std::string> requestDataQueue;
    boost::container::devector<std::function<void(bool, uint32_t, Response&)>>
        requestCallbackQueue;

    // Retry policy for all connections in the pool
    uint32_t maxRetryAttempts = 5;
    uint32_t retryIntervalSecs = 0;
    std::string retryPolicyAction = "TerminateAfterRetries";

    friend class HttpClient;

    // Gets called as part of callback after request is sent
    // Reuses the connection if there are any requests waiting to be sent
    // Otherwise closes the connection if it is not a keep-alive
    void sendNext(bool keepAlive, uint32_t connId)
    {
        auto conn = connections[connId];
        // Reuse the connection to send the next request in the queue
        if (!requestDataQueue.empty())
        {
            BMCWEB_LOG_DEBUG << std::to_string(requestDataQueue.size())
                             << " requests remaining in queue for " << destIP
                             << ":" << std::to_string(destPort)
                             << ", reusing connnection "
                             << std::to_string(connId);
            conn->data = requestDataQueue.front();
            requestDataQueue.pop_front();
            conn->callback = requestCallbackQueue.front();
            requestCallbackQueue.pop_front();
            if (keepAlive)
            {
                conn->sendMessage();
            }
            else
            {
                // Server is not keep-alive enabled so we need to close the
                // connection and then start over from resolve
                conn->doClose();
                conn->doResolve();
            }
            return;
        }

        // No more messages to send so close the connection if necessary
        if (keepAlive)
        {
            conn->state = ConnState::idle;
        }
        else
        {
            // Abort the connection since server is not keep-alive enabled
            conn->state = ConnState::abortConnection;
            conn->doClose();
        }
    }

    void sendData(const std::string& data,
                  const std::function<void(Response&)>& resHandler)
    {
        // Callback to be called once the request has been sent
        auto cb = [this, resHandler](bool keepAlive, uint32_t connId,
                                     Response& res) {
            // Allow provided callback to perform additional processing of the
            // request
            resHandler(res);

            // If requests remain in the queue then we want to reuse this
            // connection to send the next request
            this->sendNext(keepAlive, connId);
        };

        // Reuse an existing connection if one is available
        for (unsigned int i = 0; i < connections.size(); i++)
        {
            auto conn = connections[i];
            if (conn->state == ConnState::idle)
            {
                BMCWEB_LOG_DEBUG << "Grabbing idle connection "
                                 << std::to_string(i) << " from pool " << destIP
                                 << ":" << std::to_string(destPort);
                conn->data = data;
                conn->callback = std::bind_front(cb);
                conn->sendMessage();
                return;
            }
            if ((conn->state == ConnState::initialized) ||
                (conn->state == ConnState::closed))
            {
                BMCWEB_LOG_DEBUG << "Reusing existing connection "
                                 << std::to_string(i) << " from pool " << destIP
                                 << ":" << std::to_string(destPort);
                conn->data = data;
                conn->callback = std::bind_front(cb);
                conn->doResolve();
                return;
            }
        }

        // All connections in use so create a new connection or add request to
        // the queue
        if (connections.size() < maxPoolSize)
        {
            BMCWEB_LOG_DEBUG << "Adding new connection to pool " << destIP
                             << ":" << std::to_string(destPort);
            this->addConnection();
            connections.back()->data = data;
            connections.back()->callback = std::bind_front(cb);
            connections.back()->doResolve();
        }
        else if (requestDataQueue.size() < maxRequestQueueSize)
        {
            BMCWEB_LOG_ERROR << "Max pool size reached. Adding data to queue.";
            requestDataQueue.push_back(data);
            requestCallbackQueue.push_back(std::bind_front(cb));
        }
        else
        {
            BMCWEB_LOG_ERROR << destIP << ":" << std::to_string(destPort)
                             << " request queue full.  Dropping request.";
        }
    }

    void addConnection()
    {
        unsigned int newId = static_cast<unsigned int>(connections.size());

        connections.emplace_back(std::make_shared<ConnectionInfo>(
            ioc, id, destIP, destPort, destUri, httpHeader, newId,
            maxRetryAttempts, retryIntervalSecs, retryPolicyAction));

        BMCWEB_LOG_DEBUG << "Added connection "
                         << std::to_string(connections.size() - 1)
                         << " to pool " << destIP << ":"
                         << std::to_string(destPort);
    }

  public:
    explicit ConnectionPool(boost::asio::io_context& ioc, const std::string& id,
                            const std::string& destIP, const uint16_t destPort,
                            const std::string& destUri,
                            const boost::beast::http::fields& httpHeader) :
        ioc(ioc),
        id(id), destIP(destIP), destPort(destPort), destUri(destUri),
        httpHeader(httpHeader)
    {
        std::string clientKey = destIP + ":" + std::to_string(destPort);
        BMCWEB_LOG_DEBUG << "Initializing connection pool for " << destIP << ":"
                         << std::to_string(destPort);

        // Use existing retry info if it was set before connections were created
        auto rp = unconnectedRetryInfo.find(clientKey);
        if (rp != unconnectedRetryInfo.end())
        {
            BMCWEB_LOG_DEBUG << "Using previous retry info for " << clientKey;

            maxRetryAttempts = rp->second.maxRetryAttempts;
            retryIntervalSecs = rp->second.retryIntervalSecs;
            retryPolicyAction = rp->second.retryPolicyAction;

            // We can remove the info now that a connection has been created
            unconnectedRetryInfo.erase(clientKey);
        }

        // Initialize the pool with a single connection
        addConnection();
    }
};

class HttpClient
{
  private:
    std::unordered_map<std::string, ConnectionPool> connectionPools;
    boost::asio::io_context& ioc =
        crow::connections::systemBus->get_io_context();
    HttpClient()
    {
        // Setup signal matching first in case a satellite becomes available
        // before we complete manual searching
        registerSatelliteConfigSignal();

        // Search for satellite config information that's already available
        crow::connections::systemBus->async_method_call(
            [](const boost::system::error_code ec,
               const dbus::utility::ManagedObjectType& objects) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "DBUS response error " << ec.value()
                                     << ", " << ec.message();
                    return;
                }

                for (const auto& objectPath : objects)
                {
                    for (const auto& interface : objectPath.second)
                    {
                        if (interface.first ==
                            "xyz.openbmc_project.Configuration.SatelliteController")
                        {
                            BMCWEB_LOG_DEBUG << "Found Satellite Controller at "
                                             << objectPath.first.str;
                            SatelliteConfig satellite;
                            for (const auto& val : interface.second)
                            {
                                if (val.first == "Name")
                                {
                                    const std::string* valData =
                                        std::get_if<std::string>(&val.second);
                                    if (valData == nullptr)
                                    {
                                        BMCWEB_LOG_ERROR
                                            << "Invalid Name value";
                                        break;
                                    }
                                    if (satelliteInfo.find(*valData) !=
                                        satelliteInfo.end())
                                    {
                                        BMCWEB_LOG_ERROR
                                            << "Satellite config already exists for "
                                            << *valData;
                                        break;
                                    }
                                    satellite.name = *valData;
                                }

                                else if (val.first == "Hostname")
                                {
                                    const std::string* valData =
                                        std::get_if<std::string>(&val.second);
                                    if (valData == nullptr)
                                    {
                                        BMCWEB_LOG_ERROR
                                            << "Invalid Hostname value";
                                        break;
                                    }
                                    satellite.host = *valData;
                                }

                                else if (val.first == "Port")
                                {
                                    const uint64_t* valData =
                                        std::get_if<uint64_t>(&val.second);
                                    if (valData == nullptr)
                                    {
                                        BMCWEB_LOG_ERROR
                                            << "Invalid Port value";
                                        break;
                                    }

                                    if ((*valData > 65535) || (*valData == 0))
                                    {
                                        BMCWEB_LOG_ERROR
                                            << "Port value out of range";
                                    }
                                    satellite.port =
                                        static_cast<uint16_t>(*valData);
                                }

                                else if (val.first == "AuthType")
                                {
                                    const std::string* valData =
                                        std::get_if<std::string>(&val.second);
                                    if (valData == nullptr)
                                    {
                                        BMCWEB_LOG_ERROR
                                            << "Invalid AuthType value";
                                        break;
                                    }

                                    // For now assume authentication not
                                    // required to communicate with the
                                    // satellite BMC
                                    if (*valData != "none")
                                    {
                                        BMCWEB_LOG_ERROR
                                            << "Unsupported AuthType value: "
                                            << *valData;
                                        break;
                                    }
                                    satellite.authType = *valData;
                                }
                            }

                            // Make sure all required config information was
                            // available
                            if (satellite.name.empty())
                            {
                                BMCWEB_LOG_ERROR
                                    << "Satellite config missing Name";
                                continue;
                            }
                            if (satellite.host.empty())
                            {
                                BMCWEB_LOG_ERROR << "Satellite config "
                                                 << satellite.name
                                                 << " missing Hostname";
                                continue;
                            }
                            if (satellite.port == 0)
                            {
                                BMCWEB_LOG_ERROR << "Satellite config "
                                                 << satellite.name
                                                 << " missing Port";
                                continue;
                            }
                            if (satellite.authType.empty())
                            {
                                BMCWEB_LOG_ERROR << "Satellite config "
                                                 << satellite.name
                                                 << " missing AuthType";
                                continue;
                            }
                            satelliteInfo[satellite.name] = satellite;
                            BMCWEB_LOG_DEBUG
                                << "Added satellite config " << satellite.name
                                << " at " << satellite.host << ":"
                                << std::to_string(satellite.port)
                                << ", AuthType=" << satellite.authType;
                        }
                    }
                }

                if (!satelliteInfo.empty())
                {
                    BMCWEB_LOG_DEBUG << "Redfish Aggregation enabled with "
                                     << std::to_string(satelliteInfo.size())
                                     << " satellite BMCs";
                }
                else
                {
                    BMCWEB_LOG_DEBUG
                        << "No satellite BMCs detected.  Redfish Aggregation not enabled";
                }
            },
            "xyz.openbmc_project.EntityManager", "/",
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
    }

    // Setup a D-Bus match to add the config info for any new satellites
    // that added after bmcweb starts
    void registerSatelliteConfigSignal()
    {
/*
        // This causes getSatelliteConfig to get called 5 times (once per property)
        BMCWEB_LOG_DEBUG << "Satellite config signal - Register";
        std::string matchStr = "type='signal',member='PropertiesChanged',"
                               "path_namespace='/xyz/openbmc_project/inventory',"
                               "arg0='xyz.openbmc_project.Configuration.SatelliteController'";
        matchSatelliteConfigMonitor = std::make_shared<sdbusplus::bus::match::match>(
            *crow::connections::systemBus, matchStr, getSatelliteConfig);
*/

/*
        // This gets individual properties
        BMCWEB_LOG_DEBUG << "Satellite config signal - Register";
        std::string matchStr = "type='signal',member='PropertiesChanged',"
                               "interface='org.freedesktop.DBus.Properties',"
                               "arg0namespace='xyz.openbmc_project.Configuration.SatelliteController'";
        matchSatelliteSignalMonitor = std::make_unique<sdbusplus::bus::match::match>(
            *crow::connections::systemBus, matchStr, getSatelliteConfig);
*/

/*
        // This will get called multiple times for every interface including
        // xyz.openbmc_project.Configuration.SatelliteController
        BMCWEB_LOG_DEBUG << "Satellite config signal - Register";
        std::string matchStr =
            ("type='signal',"
             "interface='org.freedesktop.DBus.ObjectManager',"
             "member='InterfacesAdded'");
             //"path_namespace='/xyz/openbmc_project/inventory',"
             //"member='InterfacesAdded'");
        matchSatelliteSignalMonitor = std::make_unique<sdbusplus::bus::match::match>(
            *crow::connections::systemBus, matchStr, getSatelliteConfig);
*/



        // This will get called multiple times for every interface including
        // xyz.openbmc_project.Configuration.SatelliteController
        BMCWEB_LOG_DEBUG << "Satellite config signal - Register";
        std::string matchStr =
            ("type='signal',"
             "interface='org.freedesktop.DBus.ObjectManager',"
             "member='InterfacesAdded'");
             //"path='/xyz/openbmc_project/inventory'");
             //"arg3='xyz.openbmc_project.Configuration.SatelliteController'");
             //"path_namespace='/xyz/openbmc_project/inventory',"
             //"member='InterfacesAdded'");
        matchSatelliteSignalMonitor = std::make_unique<sdbusplus::bus::match::match>(
            *crow::connections::systemBus, matchStr, getSatelliteConfig);





/*
        // Not working as-is
        BMCWEB_LOG_DEBUG << "Satellite config signal - Register";
        std::string matchStr = "interface='org.freedesktop.DBus.ObjectMapper',"
                               "type='signal',member='InterfacesAdded'";
                               //"path_namespace='/xyz/openbmc_project/inventory'";
                               //"arg0='xyz.openbmc_project.Configuration.SatelliteController'";
        matchSatelliteSignalMonitor = std::make_unique<sdbusplus::bus::match::match>(
            *crow::connections::systemBus, matchStr, getSatelliteConfig);
*/
    }

    static void getSatelliteConfig(sdbusplus::message::message& msg)
    {
      BMCWEB_LOG_DEBUG << "";
      BMCWEB_LOG_DEBUG << "MYDEBUG: Here we go again...";
      const std::string& pathName = msg.get_path();
      std::string interfaceName;

//using BasicVariantType =
//    std::variant<std::vector<std::string>, std::string, int64_t, uint64_t,
//                 double, int32_t, uint32_t, int16_t, uint16_t, uint8_t, bool>;
//      boost::container::flat_map<std::string, BasicVariantType> properties;

      //boost::container::flat_map<std::string, dbus::utility::DbusVariantType> properties;

//      dbus::utility::ManagedObjectType properties;
//      dbus::utility::DBusInteracesMap properties;
//      dbus::utility::DBusPropertiesMap properties;
      BMCWEB_LOG_DEBUG << "MYDEBUG: pathName = " << pathName;

//      BMCWEB_LOG_DEBUG << "MYDEBUG: About to call read()";

      BMCWEB_LOG_DEBUG << "MYDEBUG: msg.get_member() = " << msg.get_member();

//      msg.read(interfaceName, properties);
//      BMCWEB_LOG_DEBUG << "MYDEBUG: interfaceName = " << interfaceName;

      nlohmann::json data;
      int r = openbmc_mapper::convertDBusToJSON("oa{sa{sv}}", msg, data);
      if (r < 0)
      {
          BMCWEB_LOG_ERROR << "convertDBUSToJSON failed with " << r;
          return;
      }

      if (!data.is_array())
      {
          BMCWEB_LOG_ERROR << "No data in InterfacesAdded signal";
      }

      for (auto& entry : data[1].items())
      {
          BMCWEB_LOG_ERROR << "MYDEBUG: entry.key()=" << entry.key();
          BMCWEB_LOG_ERROR << "MYDEBUG: entry.value()=" << entry.value();
      }


//      for (const auto& prop : properties)
//      {
          //BMCWEB_LOG_DEBUG << "MYDEBUG: prop.first = " << prop.first;
//          BMCWEB_LOG_DEBUG << "MYDEBUG: prop.first = " << prop.first.str;
//      }
      BMCWEB_LOG_DEBUG << "MYDEBUG: Past the new stuff";


/*
        //boost::asio::deadline_timer filterTimer(ioc);
        boost::asio::deadline_timer filterTimer(
            crow::connections::systemBus->get_io_context());




        BMCWEB_LOG_DEBUG << "MYDEBUG: Start of getSatelliteConfig()";
        if (msg.is_method_error())
        {
            BMCWEB_LOG_ERROR << "SatelliteConfig Signal error";
            return;
        }

        sdbusplus::message::object_path path(msg.get_path());
        std::string id = path.filename();
        if (id.empty())
        {
            BMCWEB_LOG_ERROR << "Failed to get Id from path";
        }

        BMCWEB_LOG_DEBUG << "MYDEBUG: Id is " << id;


        filterTimer.expires_from_now(boost::posix_time::seconds(1));
        filterTimer.async_wait([&msg](const boost::system::error_code& ec) {
            if (ec == boost::asio::error::operation_aborted)
            {
                // We were canceled
                return;
            }
            if (ec)
            {
                BMCWEB_LOG_ERROR << "timer error";
                return;
            }
*/

/*
        std::string interface;
        dbus::utility::DBusPropertiesMap props;
        std::vector<std::string> invalidProps;
        msg.read(interface, props, invalidProps);

        BMCWEB_LOG_DEBUG << "MYDEBUG: interface = " << interface;
        BMCWEB_LOG_DEBUG << "MYDEBUG: About to print properties";
        for (const auto& prop : props)
        {
            BMCWEB_LOG_DEBUG << "MYDEBUG: prop.first = " << prop.first;
        //                     << ", prop.second = " << prop.second;
        }
        
        BMCWEB_LOG_DEBUG << "MYDEBUG: Past the for loop";
        }); // End of async_wait
*/


      /*
        BMCWEB_LOG_DEBUG << "MYDEBUG: About to create interfacesProperties";

        dbus::utility::DBusInteracesMap interfacesProperties;
        sdbusplus::message::object_path objPath;
        
        BMCWEB_LOG_DEBUG << "MYDEBUG: About to call msg.read()";
        
        msg.read(objPath, interfacesProperties);
        
        BMCWEB_LOG_DEBUG << "objPath = " << objPath.str;
        
        for (auto& interface : interfacesProperties)
        {
            BMCWEB_LOG_DEBUG << "interface = " << interface.first;
        }
*/


/*
        std::vector<
            std::pair<std::string, dbus::utility::DBusPropertiesMap>>
            interfacesProperties;
        sdbusplus::message::object_path objPath;
        msg.read(objPath, interfacesProperties);

        BMCWEB_LOG_DEBUG << "MYDEBUG: objPath = " << objPath.str;
        for (const auto& : props)
        {
            BMCWEB_LOG_DEBUG << "MYDEBUG: propts.first = " props.first;
        }
*/

        BMCWEB_LOG_DEBUG << "MYDEBUG: End of getSatelliteConfig";
    }

  public:
    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;
    HttpClient(HttpClient&&) = delete;
    HttpClient& operator=(HttpClient&&) = delete;
    ~HttpClient() = default;

    static HttpClient& getInstance()
    {
        static HttpClient handler;
        return handler;
    }

    // Send a request to destIP:destPort where additional processing of the
    // result is not required
    void sendData(const std::string& data, const std::string& id,
                  const std::string& destIP, const uint16_t destPort,
                  const std::string& destUri,
                  const boost::beast::http::fields& httpHeader)
    {
        // Create a dummy callback for handling the response
        auto resHandler = [](Response& res) {
            BMCWEB_LOG_DEBUG << "Response handled with return code: "
                             << std::to_string(res.resultInt());
        };

        sendData(data, id, destIP, destPort, destUri, httpHeader,
                 std::bind_front(resHandler));
    }

    // Send request to destIP:destPort and use the provided callback to
    // handle the response
    void sendData(const std::string& data, const std::string& id,
                  const std::string& destIP, const uint16_t destPort,
                  const std::string& destUri,
                  const boost::beast::http::fields& httpHeader,
                  const std::function<void(Response&)>& resHandler)
    {
        std::string clientKey = destIP + ":" + std::to_string(destPort);
        auto result = connectionPools.try_emplace(
            clientKey, ioc, id, destIP, destPort, destUri, httpHeader);

        if (result.second)
        {
            BMCWEB_LOG_DEBUG << "Created connection pool for " << clientKey;
        }
        else
        {
            BMCWEB_LOG_DEBUG << "Using existing connection pool for "
                             << clientKey;
        }

        // Send the data using either the existing connection pool or the newly
        // created connection pool
        result.first->second.sendData(data, std::bind_front(resHandler));
    }

    void setRetryConfig(const uint32_t retryAttempts,
                        const uint32_t retryTimeoutInterval,
                        const std::string& destIP, const uint16_t destPort)
    {
        std::string clientKey = destIP + ":" + std::to_string(destPort);
        auto conn = connectionPools.find(clientKey);
        if (conn == connectionPools.end())
        {
            auto rp = unconnectedRetryInfo.find(clientKey);
            if (rp != unconnectedRetryInfo.end())
            {
                BMCWEB_LOG_DEBUG
                    << "setRetryConfig(): updating existing retry info for unconnected "
                    << clientKey;
                rp->second.maxRetryAttempts = retryAttempts;
                rp->second.retryIntervalSecs = retryTimeoutInterval;
            }
            else
            {
                BMCWEB_LOG_DEBUG
                    << "setRetryConfig(): creating retry info for unconnected "
                    << clientKey;
                RetryPolicyData obj;
                obj.maxRetryAttempts = retryAttempts;
                obj.retryIntervalSecs = retryTimeoutInterval;
                unconnectedRetryInfo[clientKey] = obj;
            }
        }
        else
        {
            BMCWEB_LOG_DEBUG
                << "setRetryConfig(): updating retry info for existing connection "
                << clientKey;
            conn->second.maxRetryAttempts = retryAttempts;
            conn->second.retryIntervalSecs = retryTimeoutInterval;
        }
    }

    void setRetryPolicy(const std::string& retryPolicy,
                        const std::string& destIP, const uint16_t destPort)
    {
        std::string clientKey = destIP + ":" + std::to_string(destPort);
        auto conn = connectionPools.find(clientKey);
        if (conn == connectionPools.end())
        {
            auto rp = unconnectedRetryInfo.find(clientKey);
            if (rp != unconnectedRetryInfo.end())
            {
                BMCWEB_LOG_DEBUG
                    << "setRetryPolicy(): updating existing retry info for unconnected "
                    << clientKey;
                rp->second.retryPolicyAction = retryPolicy;
            }
            else
            {
                BMCWEB_LOG_DEBUG
                    << "setRetryPolicy(): creating retry info for unconnected "
                    << clientKey;
                RetryPolicyData obj;
                obj.retryPolicyAction = retryPolicy;
                unconnectedRetryInfo[clientKey] = obj;
            }
        }
        else
        {
            BMCWEB_LOG_DEBUG
                << "setRetryPolicy(): updating retry info for existing connection "
                << clientKey;
            conn->second.retryPolicyAction = retryPolicy;
        }
    }
};

} // namespace crow
