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

enum class ConnState
{
    initialized,
    connectInProgress,
    connectFailed,
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
    boost::asio::io_context& ioc;
    boost::asio::ssl::context ctx{boost::asio::ssl::context::tlsv12_client};
    std::shared_ptr<boost::beast::ssl_stream<boost::beast::tcp_stream>> sslConn;
    std::shared_ptr<boost::beast::tcp_stream> conn;
    boost::asio::steady_timer timer;
    boost::beast::flat_buffer buffer;
    boost::beast::http::request<boost::beast::http::string_body> req;
    boost::beast::http::response<boost::beast::http::string_body> res;
    boost::asio::ip::tcp::resolver::results_type endpoint;
    std::vector<std::pair<std::string, std::string>> headers;
    std::queue<std::string> requestDataQueue;
    ConnState state;
    std::string subId;
    std::string host;
    std::string port;
    std::string uri;
    uint32_t retryCount;
    uint32_t maxRetryAttempts;
    uint32_t retryIntervalSecs;
    std::string retryPolicyAction;
    bool runningTimer;
    bool isSsl;

    inline boost::beast::tcp_stream& getConn()
    {
        if (isSsl)
        {
            return (boost::beast::get_lowest_layer(*sslConn));
        }
        else
        {
            return (*conn);
        }
    }

    void doConnect()
    {
        if (isSsl)
        {
            sslConn = std::make_shared<
                boost::beast::ssl_stream<boost::beast::tcp_stream>>(ioc, ctx);
        }
        else
        {
            conn = std::make_shared<boost::beast::tcp_stream>(ioc);
        }

        if (state == ConnState::connectInProgress)
        {
            return;
        }
        state = ConnState::connectInProgress;

        BMCWEB_LOG_DEBUG << "Trying to connect to: " << host << ":" << port;

        auto respHandler =
            [self(shared_from_this())](const boost::beast::error_code& ec,
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
                if (self->isSsl)
                {
                    self->performHandshake();
                }
                else
                {
                    self->state = ConnState::connected;
                    self->checkQueue();
                }
            };

        getConn().expires_after(std::chrono::seconds(30));
        getConn().async_connect(endpoint, std::move(respHandler));
    }

    void performHandshake()
    {
        sslConn->async_handshake(
            boost::asio::ssl::stream_base::client,
            [self(shared_from_this())](const boost::beast::error_code& ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "SSL handshake failed: "
                                     << ec.message();
                    self->state = ConnState::connectFailed;
                    self->doClose();
                    return;
                }
                self->state = ConnState::connected;
                BMCWEB_LOG_DEBUG << "SSL Handshake successfull \n";

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

        BMCWEB_LOG_DEBUG << __FUNCTION__ << "(): " << host << ":" << port;

        req = {};
        res = {};

        req.version(11); // HTTP 1.1
        req.target(uri);
        req.method(boost::beast::http::verb::post);

        // Set headers
        for (const auto& [key, value] : headers)
        {
            req.set(key, value);
        }
        req.set(boost::beast::http::field::host, host);
        req.keep_alive(true);

        req.body() = data;
        req.prepare_payload();

        auto respHandler = [self(shared_from_this())](
                               const boost::beast::error_code& ec,
                               const std::size_t& bytesTransferred) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "sendMessage() failed: " << ec.message();
                self->state = ConnState::sendFailed;
                self->doClose();
                return;
            }
            BMCWEB_LOG_DEBUG << "sendMessage() bytes transferred: "
                             << bytesTransferred;
            boost::ignore_unused(bytesTransferred);

            self->recvMessage();
        };

        getConn().expires_after(std::chrono::seconds(30));
        if (isSsl)
        {
            boost::beast::http::async_write(*sslConn, req,
                                            std::move(respHandler));
        }
        else
        {
            boost::beast::http::async_write(*conn, req, std::move(respHandler));
        }
    }

    void recvMessage()
    {
        auto respHandler = [self(shared_from_this())](
                               const boost::beast::error_code& ec,
                               const std::size_t& bytesTransferred) {
            if (ec && ec != boost::beast::http::error::partial_message)
            {
                BMCWEB_LOG_ERROR << "recvMessage() failed: " << ec.message();
                self->state = ConnState::recvFailed;
                self->doClose();
                return;
            }
            BMCWEB_LOG_DEBUG << "recvMessage() bytes transferred: "
                             << bytesTransferred;
            boost::ignore_unused(bytesTransferred);

            // Discard received data. We are not interested.
            BMCWEB_LOG_DEBUG << "recvMessage() data: " << self->res;

            // Send is successful, Lets remove data from queue
            // check for next request data in queue.
            self->requestDataQueue.pop();
            self->state = ConnState::idle;

            if (ec == boost::beast::http::error::partial_message)
            {
                // Least bothered about recv message. Partial
                // message means, already data is sent. Lets close
                // connection and let next request open connection
                // to avoid truncated stream.
                self->state = ConnState::closed;
                self->doClose();
                return;
            }

            self->checkQueue();
        };

        getConn().expires_after(std::chrono::seconds(30));
        if (isSsl)
        {
            boost::beast::http::async_read(*sslConn, buffer, res,
                                           std::move(respHandler));
        }
        else
        {
            boost::beast::http::async_read(*conn, buffer, res,
                                           std::move(respHandler));
        }
    }

    void doClose()
    {
        boost::beast::error_code ec;
        getConn().cancel();
        getConn().expires_after(std::chrono::seconds(30));
        getConn().socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both,
                                    ec);

        if (ec && ec != boost::asio::error::eof)
        {
            // Many https server closes connection abruptly
            // i.e witnout close_notify. More details are at
            // https://github.com/boostorg/beast/issues/824
            if (ec == boost::asio::ssl::error::stream_truncated)
            {
                BMCWEB_LOG_DEBUG << "doClose(): Connection closed by server.";
            }
            else
            {
                BMCWEB_LOG_ERROR << "doClose() failed: " << ec.message();
            }
        }

        getConn().close();
        BMCWEB_LOG_DEBUG << "Connection closed gracefully";
        checkQueue();
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
        else
        {
            if (state == ConnState::idle)
            {
                // State idle means, previous attempt is successful.
                retryCount = 0;
            }
        }
        connStateCheck();

        return;
    }

    void connStateCheck()
    {
        switch (state)
        {
            case ConnState::connectInProgress:
            case ConnState::sendInProgress:
            case ConnState::suspended:
            case ConnState::terminated:
                // do nothing
                break;
            case ConnState::initialized:
            case ConnState::closed:
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
            default:
                break;
        }
    }

  public:
    explicit HttpClient(boost::asio::io_context& ioc, const std::string& id,
                        const std::string& destIP, const std::string& destPort,
                        const std::string& destUri,
                        const bool isSecure = true) :
        ioc(ioc),
        timer(ioc), subId(id), host(destIP), port(destPort), uri(destUri),
        isSsl(isSecure)
    {
        boost::asio::ip::tcp::resolver resolver(ioc);
        endpoint = resolver.resolve(host, port);
        state = ConnState::initialized;
        retryCount = 0;
        maxRetryAttempts = 5;
        retryIntervalSecs = 30;
        runningTimer = false;
    }

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
        headers = httpHeaders;
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
