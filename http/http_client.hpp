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
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>

#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <queue>
#include <string>

namespace crow
{

static constexpr uint8_t maxRequestQueueSize = 50;
static constexpr unsigned int httpReadBodyLimit = 1024;

enum class ConnState
{
    initialized,
    resolveInProgress,
    resolved,
    resolveFailed,
    connectInProgress,
    connectFailed,
    sslHandshakeInProgress,
    connected,
    sendInProgress,
    sendFailed,
    recvFailed,
    idle,
    suspended,
    closed,
    terminated
};

class HttpClient : public std::enable_shared_from_this<HttpClient>
{
  private:
    boost::asio::ip::tcp::resolver resolver;
    boost::asio::ssl::context ctx{boost::asio::ssl::context::tlsv12_client};
    boost::beast::tcp_stream conn;
    std::optional<boost::beast::ssl_stream<boost::beast::tcp_stream&>> sslConn;
    boost::asio::steady_timer timer;
    boost::beast::flat_static_buffer<httpReadBodyLimit> buffer;
    boost::beast::http::request<boost::beast::http::string_body> req;
    boost::beast::http::response<boost::beast::http::string_body> res;
    boost::asio::ip::tcp::resolver::results_type endpoint;
    boost::beast::http::fields fields;
    std::queue<std::string> requestDataQueue;
    std::string subId;
    std::string host;
    std::string port;
    std::string uri;
    bool useSsl;
    uint32_t retryCount;
    uint32_t maxRetryAttempts;
    uint32_t retryIntervalSecs;
    std::string retryPolicyAction;
    bool runningTimer;
    ConnState state;

    void doResolve()
    {
        state = ConnState::resolveInProgress;
        BMCWEB_LOG_DEBUG << "Trying to resolve: " << host << ":" << port;
        if (useSsl)
        {
            sslConn.emplace(conn, ctx);
        }
        conn.expires_after(std::chrono::seconds(30));
        // TODO: Use async_resolve once the boost crash is resolved
        endpoint = resolver.resolve(host, port);
        state = ConnState::resolved;
        checkQueue();
    }

    void doConnect()
    {
        if ((state == ConnState::connectInProgress) ||
            (state == ConnState::sslHandshakeInProgress))
        {
            return;
        }
        state = ConnState::connectInProgress;

        BMCWEB_LOG_DEBUG << "Trying to connect to: " << host << ":" << port;

        auto respHandler =
            [self(shared_from_this())](const boost::beast::error_code ec,
                                       const boost::asio::ip::tcp::resolver::
                                           results_type::endpoint_type& ep) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Connect " << ep
                                     << " failed: " << ec.message();
                    self->state = ConnState::connectFailed;
                    self->checkQueue();
                    return;
                }
                BMCWEB_LOG_DEBUG << "Connected to: " << ep;
                if (self->sslConn)
                {
                    self->performHandshake();
                }
                else
                {
                    self->state = ConnState::connected;
                    self->checkQueue();
                }
            };

        conn.expires_after(std::chrono::seconds(30));
        conn.async_connect(endpoint, std::move(respHandler));
    }

    void performHandshake()
    {
        if (state == ConnState::sslHandshakeInProgress)
        {
            return;
        }
        state = ConnState::sslHandshakeInProgress;

        sslConn->async_handshake(
            boost::asio::ssl::stream_base::client,
            [self(shared_from_this())](const boost::beast::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "SSL handshake failed: "
                                     << ec.message();
                    self->state = ConnState::connectFailed;
                    self->doCloseAndCheckQueue();
                    return;
                }
                self->state = ConnState::connected;
                BMCWEB_LOG_DEBUG << "SSL Handshake successfull";

                self->checkQueue();
            });
    }

    void sendMessage(const std::string& data)
    {
        if (state == ConnState::sendInProgress)
        {
            return;
        }
        state = ConnState::sendInProgress;

        BMCWEB_LOG_DEBUG << host << ":" << port;

        req = {};
        req.set(boost::beast::http::field::host, host);
        req.set(boost::beast::http::field::content_type, "text/plain");

        for (const auto& field : fields)
        {
            req.set(field.name_string(), field.value());
        }

        req.version(static_cast<int>(11)); // HTTP 1.1
        req.target(uri);
        req.method(boost::beast::http::verb::post);
        req.keep_alive(true);

        req.body() = data;
        req.prepare_payload();

        auto respHandler = [self(shared_from_this())](
                               const boost::beast::error_code ec,
                               const std::size_t& bytesTransferred) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "sendMessage() failed: " << ec.message();
                self->state = ConnState::sendFailed;
                self->doCloseAndCheckQueue();
                return;
            }
            BMCWEB_LOG_DEBUG << "sendMessage() bytes transferred: "
                             << bytesTransferred;
            boost::ignore_unused(bytesTransferred);

            self->recvMessage();
        };

        conn.expires_after(std::chrono::seconds(30));
        if (sslConn)
        {
            boost::beast::http::async_write(*sslConn, req,
                                            std::move(respHandler));
        }
        else
        {
            boost::beast::http::async_write(conn, req, std::move(respHandler));
        }
    }

    void recvMessage()
    {
        auto respHandler = [self(shared_from_this())](
                               const boost::beast::error_code ec,
                               const std::size_t& bytesTransferred) {
            if (ec && ec != boost::beast::http::error::partial_message)
            {
                BMCWEB_LOG_ERROR << "recvMessage() failed: " << ec.message();
                self->state = ConnState::recvFailed;
                // retry sending as per the retry policy
                self->checkQueue();
                return;
            }
            BMCWEB_LOG_DEBUG << "recvMessage() bytes transferred: "
                             << bytesTransferred;
            boost::ignore_unused(bytesTransferred);

            BMCWEB_LOG_DEBUG << "recvMessage() data: " << self->res;

            // Send is successful, Lets remove data from queue
            // check for next request data in queue.
            self->requestDataQueue.pop();
            self->state = ConnState::idle;

            // Set the keep_alive
            self->res.keep_alive(self->req.keep_alive());

            self->checkQueue();
        };

        conn.expires_after(std::chrono::seconds(30));
        if (sslConn)
        {
            boost::beast::http::async_read(*sslConn, buffer, res,
                                           std::move(respHandler));
        }
        else
        {
            boost::beast::http::async_read(conn, buffer, res,
                                           std::move(respHandler));
        }
    }

    void doCloseAndCheckQueue()
    {
        if (sslConn)
        {
            conn.expires_after(std::chrono::seconds(30));
            sslConn->async_shutdown([self = shared_from_this()](
                                        const boost::system::error_code ec) {
                if (ec)
                {
                    // Many https server closes connection abruptly
                    // i.e witnout close_notify. More details are at
                    // https://github.com/boostorg/beast/issues/824
                    if (ec == boost::asio::ssl::error::stream_truncated)
                    {
                        BMCWEB_LOG_ERROR
                            << "doCloseAndCheckQueue(): Connection "
                               "closed by server. ";
                    }
                    else
                    {
                        BMCWEB_LOG_ERROR << "doCloseAndCheckQueue() failed: "
                                         << ec.message();
                    }
                }
                else
                {
                    BMCWEB_LOG_DEBUG << "Connection closed gracefully...";
                }
                self->state = ConnState::closed;
                self->conn.cancel();
                self->checkQueue();
            });
        }
        else
        {
            boost::beast::error_code ec;
            conn.expires_after(std::chrono::seconds(30));
            conn.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both,
                                   ec);
            if (ec)
            {
                BMCWEB_LOG_ERROR << "doCloseAndCheckQueue() failed: "
                                 << ec.message();
            }
            else
            {
                BMCWEB_LOG_DEBUG << "Connection closed gracefully...";
            }
            state = ConnState::closed;
            conn.close();
            checkQueue();
        }
        return;
    }

    void checkQueue(const bool newRecord = false)
    {
        if (requestDataQueue.empty())
        {
            BMCWEB_LOG_DEBUG << "requestDataQueue is empty\n";
            return;
        }

        if (retryCount >= maxRetryAttempts)
        {
            BMCWEB_LOG_ERROR << "Maximum number of retries is reached.";

            // Clear queue.
            while (!requestDataQueue.empty())
            {
                requestDataQueue.pop();
            }

            BMCWEB_LOG_DEBUG << "Retry policy is set to " << retryPolicyAction;
            if (retryPolicyAction == "TerminateAfterRetries")
            {
                // TODO: delete subscription
                state = ConnState::terminated;
                return;
            }
            else if (retryPolicyAction == "SuspendRetries")
            {
                state = ConnState::suspended;
                return;
            }
            else
            {
                // keep retrying, reset count and continue.
                retryCount = 0;
            }
        }

        if ((state == ConnState::connectFailed) ||
            (state == ConnState::sendFailed) ||
            (state == ConnState::recvFailed))
        {
            if (newRecord)
            {
                // We are already running async wait and retry.
                // Since record is added to queue, it gets the
                // turn in FIFO.
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
                [self = shared_from_this()](boost::system::error_code) {
                    self->runningTimer = false;
                    self->connStateCheck();
                });
            return;
        }

        if (state == ConnState::idle)
        {
            // State idle means, previous attempt is successful.
            retryCount = 0;
        }
        connStateCheck();

        return;
    }

    void connStateCheck()
    {
        switch (state)
        {
            case ConnState::initialized:
            case ConnState::resolveFailed:
            {
                doResolve();
                break;
            }
            case ConnState::resolveInProgress:
            case ConnState::connectInProgress:
            case ConnState::sslHandshakeInProgress:
            case ConnState::sendInProgress:
            case ConnState::suspended:
            case ConnState::terminated:
            case ConnState::closed:
                // do nothing
                break;
            case ConnState::resolved:
            case ConnState::connectFailed:
            case ConnState::sendFailed:
            case ConnState::recvFailed:
            {
                // After establishing the connection, checkQueue() will
                // get called and it will attempt to send data.
                doConnect();
                break;
            }
            case ConnState::connected:
            case ConnState::idle:
            {
                std::string data = requestDataQueue.front();
                sendMessage(data);
                break;
            }
        }
    }

  public:
    explicit HttpClient(boost::asio::io_context& ioc, const std::string& id,
                        const std::string& destIP, const std::string& destPort,
                        const std::string& destUri,
                        const bool inUseSsl = true) :
        resolver(ioc),
        conn(ioc), timer(ioc), subId(id), host(destIP), port(destPort),
        uri(destUri), useSsl(inUseSsl), retryCount(0), maxRetryAttempts(5),
        retryPolicyAction("TerminateAfterRetries"), runningTimer(false),
        state(ConnState::initialized)
    {}

    void sendData(const std::string& data)
    {
        if (state == ConnState::suspended)
        {
            return;
        }

        if (requestDataQueue.size() <= maxRequestQueueSize)
        {
            requestDataQueue.push(data);
            checkQueue(true);
        }
        else
        {
            BMCWEB_LOG_ERROR << "Request queue is full. So ignoring data.";
        }

        return;
    }

    void setHeaders(
        const std::vector<std::pair<std::string, std::string>>& httpHeaders)
    {
        // Set headers
        for (const auto& [key, value] : httpHeaders)
        {
            fields.set(key, value);
        }
    }

    void setRetryConfig(const uint32_t retryAttempts,
                        const uint32_t retryTimeoutInterval)
    {
        maxRetryAttempts = retryAttempts;
        retryIntervalSecs = retryTimeoutInterval;
    }

    void setRetryPolicy(const std::string& retryPolicy)
    {
        retryPolicyAction = retryPolicy;
    }
};

} // namespace crow
