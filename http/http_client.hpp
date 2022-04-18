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

constexpr uint8_t maxPoolSize = 4;
constexpr uint8_t maxRequestQueueSize = 50;
constexpr unsigned int httpReadBodyLimit = 8192;

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
static std::unordered_map<std::string, RetryPolicyData> retryInfo;

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
    uint16_t port;
    uint32_t connId;
    std::string data;
    std::function<void(bool, uint32_t, Response&)> callback;

    // Retry policy information
    // This should be updated before each message is sent
    uint32_t& maxRetryAttempts = retryInfo["default"].maxRetryAttempts;
    uint32_t& retryIntervalSecs = retryInfo["default"].retryIntervalSecs;
    std::string& retryPolicyAction = retryInfo["default"].retryPolicyAction;

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
                            const unsigned int connId) :
        conn(ioc),
        timer(ioc),
        req(boost::beast::http::verb::post, destUri, 11, "", httpHeader),
        subId(id), host(destIP), port(destPort), connId(connId)
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
    boost::container::devector<std::string> retryPolicyNameQueue;

    friend class HttpClient;

    // Configure a connections's data, callback, and retry info in preparation
    // to begin sending a request
    void setConnProps(const std::shared_ptr<ConnectionInfo>& conn)
    {
        std::string& retryPolicyName = retryPolicyNameQueue.front();
        setConnRetryPolicy(conn, retryPolicyName);
        conn->data = requestDataQueue.front();
        conn->callback = requestCallbackQueue.front();

        // Clean up the queues
        retryPolicyNameQueue.pop_front();
        requestDataQueue.pop_front();
        requestCallbackQueue.pop_front();
    }

    // Configures a connection to use the specific retry policy.  Creates a new
    // policy with default values if the policy does not already exist
    inline void setConnRetryPolicy(const std::shared_ptr<ConnectionInfo>& conn,
                                   const std::string& retryPolicyName)
    {
        auto result = retryInfo.try_emplace(retryPolicyName);
        if (result.second)
        {
            BMCWEB_LOG_DEBUG << "Creating retry policy \"" << retryPolicyName
                             << "\" with default values";
        }

        BMCWEB_LOG_DEBUG << destIP << ":" << std::to_string(destPort)
                         << ", id: " << std::to_string(conn->connId)
                         << " using retry policy \"" << retryPolicyName << "\"";

        conn->maxRetryAttempts = result.first->second.maxRetryAttempts;
        conn->retryIntervalSecs = result.first->second.retryIntervalSecs;
        conn->retryPolicyAction = result.first->second.retryPolicyAction;
    }

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

            setConnProps(conn);

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

    void sendData(const std::string& data, const std::string& retryPolicyName,
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
                setConnRetryPolicy(conn, retryPolicyName);
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
                setConnRetryPolicy(conn, retryPolicyName);
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
            setConnRetryPolicy(connections.back(), retryPolicyName);
            connections.back()->doResolve();
        }
        else if (requestDataQueue.size() < maxRequestQueueSize)
        {
            BMCWEB_LOG_ERROR << "Max pool size reached. Adding data to queue.";
            requestDataQueue.push_back(data);
            requestCallbackQueue.push_back(std::bind_front(cb));
            retryPolicyNameQueue.push_back(retryPolicyName);
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
            ioc, id, destIP, destPort, destUri, httpHeader, newId));

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
    std::unique_ptr<boost::asio::deadline_timer> filterTimer;

    // Match signals for adding Satellite Config
    std::unique_ptr<sdbusplus::bus::match::match> matchSatelliteSignalMonitor;
    std::unordered_map<std::string, boost::urls::url> satelliteInfo;

    HttpClient()
    {
        filterTimer = std::make_unique<boost::asio::deadline_timer>(ioc);

        // Setup signal matching first in case a satellite becomes available
        // before we complete manual searching
        registerSatelliteConfigSignal();

        // Search for satellite config information that's available before
        // HttpClient is initialized
        getSatelliteConfigs();
    }

    // Setup a D-Bus match to add the config info for any satellites
    // that are added or changed after bmcweb starts
    void registerSatelliteConfigSignal()
    {
        // This handler will get called per each property created or updated.
        // We want to wait until its final call and then query the D-Bus to get
        // all of the satellite config information
        std::function<void(sdbusplus::message::message&)> eventHandler =
            [this](sdbusplus::message::message& message) {
                if (message.is_method_error())
                {
                    BMCWEB_LOG_ERROR
                        << "registerSatelliteConfigSignal callback method error";
                    return;
                }

                // This implicitly cancels the timer
                filterTimer->expires_from_now(boost::posix_time::seconds(1));

                filterTimer->async_wait([this](const boost::system::error_code&
                                                   ec) {
                    if (ec == boost::asio::error::operation_aborted)
                    {
                        // We were cancelled
                        return;
                    }
                    if (ec)
                    {
                        BMCWEB_LOG_ERROR
                            << "registerSatelliteConfigSignal timer error";
                        return;
                    }
                    // Now manually scan to get all of the new satellite config
                    // information
                    BMCWEB_LOG_DEBUG
                        << "Match received for SatelliteController.  Updating satellite configs";
                    this->getSatelliteConfigs();
                });
            };

        BMCWEB_LOG_DEBUG << "Satellite config signal - Register";
        std::string matchStr =
            "type='signal',member='PropertiesChanged',"
            "interface='org.freedesktop.DBus.Properties',"
            "arg0namespace='xyz.openbmc_project.Configuration.SatelliteController'";
        matchSatelliteSignalMonitor =
            std::make_unique<sdbusplus::bus::match::match>(
                *crow::connections::systemBus, matchStr, eventHandler);
    }

    // Polls D-Bus to get all available satellite config information
    void getSatelliteConfigs()
    {
        BMCWEB_LOG_DEBUG << "Gathering satellite configs";
        crow::connections::systemBus->async_method_call(
            [this](const boost::system::error_code ec,
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

                            this->parseSatelliteConfig(interface.second);
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

    // Parse the properties of a satellite config object and add the
    // configuration if the properties are valid
    void
        parseSatelliteConfig(const dbus::utility::DBusPropertiesMap& properties)
    {
        boost::urls::url url;
        std::string name;

        for (const auto& prop : properties)
        {
            if (prop.first == "Name")
            {
                const std::string* propVal =
                    std::get_if<std::string>(&prop.second);
                if (propVal == nullptr)
                {
                    BMCWEB_LOG_ERROR << "Invalid Name value";
                    return;
                }
                name = *propVal;
            }

            else if (prop.first == "Hostname")
            {
                const std::string* propVal =
                    std::get_if<std::string>(&prop.second);
                if (propVal == nullptr)
                {
                    BMCWEB_LOG_ERROR << "Invalid Hostname value";
                    return;
                }
                url.set_host(*propVal);
            }

            else if (prop.first == "Port")
            {
                const uint64_t* propVal = std::get_if<uint64_t>(&prop.second);
                if (propVal == nullptr)
                {
                    BMCWEB_LOG_ERROR << "Invalid Port value";
                    return;
                }

                if ((*propVal > 65535) || (*propVal == 0))
                {
                    BMCWEB_LOG_ERROR << "Port value out of range";
                }
                url.set_port(static_cast<uint16_t>(*propVal));
            }

            else if (prop.first == "AuthType")
            {
                const std::string* propVal =
                    std::get_if<std::string>(&prop.second);
                if (propVal == nullptr)
                {
                    BMCWEB_LOG_ERROR << "Invalid AuthType value";
                    return;
                }

                // For now assume authentication not required to communicate
                // with the satellite BMC
                if (*propVal != "none")
                {
                    BMCWEB_LOG_ERROR
                        << "Unsupported AuthType value: " << *propVal
                        << ", only \"none\" is supported";
                    return;
                }
                url.set_scheme("http");
            }
        } // Finished reading properties

        // Make sure all required config information was made available
        if (name.empty())
        {
            BMCWEB_LOG_ERROR << "Satellite config missing Name";
            return;
        }

        if (url.host().empty())
        {
            BMCWEB_LOG_ERROR << "Satellite config " << name << " missing Host";
            return;
        }

        if (!url.has_port())
        {
            BMCWEB_LOG_ERROR << "Satellite config " << name << " missing Port";
            return;
        }

        if (!url.has_scheme())
        {
            BMCWEB_LOG_ERROR << "Satellite config " << name
                             << " missing AuthType";
            return;
        }

        BMCWEB_LOG_DEBUG << "Added satellite config " << name << " at "
                         << url.scheme() << "://"
                         << url.encoded_host_and_port();
        satelliteInfo[name] = url;
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
                  const boost::beast::http::fields& httpHeader,
                  const std::string& retryPolicyName)
    {
        // Create a dummy callback for handling the response
        auto resHandler = [](Response& res) {
            BMCWEB_LOG_DEBUG << "Response handled with return code: "
                             << std::to_string(res.resultInt());
        };

        sendData(data, id, destIP, destPort, destUri, httpHeader,
                 retryPolicyName, std::bind_front(resHandler));
    }

    // Send request to destIP:destPort and use the provided callback to
    // handle the response
    void sendData(const std::string& data, const std::string& id,
                  const std::string& destIP, const uint16_t destPort,
                  const std::string& destUri,
                  const boost::beast::http::fields& httpHeader,
                  const std::string& retryPolicyName,
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
        result.first->second.sendData(data, retryPolicyName,
                                      std::bind_front(resHandler));
    }
};

void setRetryConfig(const uint32_t retryAttempts,
                    const uint32_t retryTimeoutInterval,
                    const std::string& retryPolicyName)
{
    // We need to create the retry policy if one does not already exist for
    // the given retryPolicyName
    auto result = retryInfo.try_emplace(retryPolicyName);
    if (result.second)
    {
        BMCWEB_LOG_DEBUG << "setRetryConfig(): Creating new retry policy \""
                         << retryPolicyName << "\"";
    }
    else
    {
        BMCWEB_LOG_DEBUG << "setRetryConfig(): Updating retry info for \""
                         << retryPolicyName << "\"";
    }

    result.first->second.maxRetryAttempts = retryAttempts;
    result.first->second.retryIntervalSecs = retryTimeoutInterval;
}

void setRetryPolicy(const std::string& retryPolicy,
                    const std::string& retryPolicyName)
{
    // We need to create the retry policy if one does not already exist for
    // the given retryPolicyName
    auto result = retryInfo.try_emplace(retryPolicyName);
    if (result.second)
    {
        BMCWEB_LOG_DEBUG << "setRetryPolicy(): Creating new retry policy \""
                         << retryPolicyName << "\"";
    }
    else
    {
        BMCWEB_LOG_DEBUG << "setRetryPolicy(): Updating retry policy for \""
                         << retryPolicyName << "\"";
    }

    result.first->second.retryPolicyAction = retryPolicy;
}

} // namespace crow
