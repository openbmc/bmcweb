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
    boost::asio::ssl::context ctx{boost::asio::ssl::context::tlsv12_client};
    boost::beast::tcp_stream conn;
    std::optional<boost::beast::ssl_stream<boost::beast::tcp_stream&>> sslConn;
    boost::asio::steady_timer timer;
    boost::beast::flat_static_buffer<httpReadBodyLimit> buffer;
    std::optional<
        boost::beast::http::response_parser<boost::beast::http::string_body>>
        parser;
    boost::beast::http::request<boost::beast::http::string_body> req;
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

    void doConnect()
    {
        if (useSsl)
        {
            sslConn.emplace(conn, ctx);
        }

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
        for (const auto& field : fields)
        {
            req.set(field.name_string(), field.value());
        }
        req.set(boost::beast::http::field::host, host);
        req.set(boost::beast::http::field::content_type, "text/plain");

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
                self->doCloseAndCheckQueue();
                return;
            }
            BMCWEB_LOG_DEBUG << "recvMessage() bytes transferred: "
                             << bytesTransferred;
            boost::ignore_unused(bytesTransferred);

            // TODO: check for return status code and perform
            // retry if fails(Ex: 40x). Take action depending on
            // retry policy.
            BMCWEB_LOG_DEBUG << "recvMessage() data: "
                             << self->parser->get().body();

            // Send is successful, Lets remove data from queue
            // check for next request data in queue.
            self->requestDataQueue.pop();
            self->state = ConnState::idle;

            // Transfer ownership of the response
            self->parser->release();

            // TODO: Implement the keep-alive connections.
            // Most of the web servers close connection abruptly
            // and might be reason due to which its observed that
            // stream_truncated(Next read) or  partial_message
            // errors. So for now, closing connection and re-open
            // for all cases.
            self->state = ConnState::closed;
            self->doCloseAndCheckQueue();
        };

        parser.emplace(std::piecewise_construct, std::make_tuple());
        parser->body_limit(httpReadBodyLimit);
        buffer.consume(buffer.size());

        conn.expires_after(std::chrono::seconds(30));
        if (sslConn)
        {
            boost::beast::http::async_read(*sslConn, buffer, *parser,
                                           std::move(respHandler));
        }
        else
        {
            boost::beast::http::async_read(conn, buffer, *parser,
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
            case ConnState::connectInProgress:
            case ConnState::sslHandshakeInProgress:
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
                        const bool inUseSsl = true) :
        conn(ioc),
        timer(ioc), subId(id), host(destIP), port(destPort), uri(destUri),
        useSsl(inUseSsl), retryCount(0), maxRetryAttempts(5),
        retryPolicyAction("TerminateAfterRetries"), runningTimer(false),
        state(ConnState::initialized)
    {
        boost::asio::ip::tcp::resolver resolver(ioc);
        // TODO: Use async_resolver. boost asio example
        // code as is crashing with async_resolve().
        // It needs debug.
        endpoint = resolver.resolve(host, port);
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
        // Set headers
        for (const auto& [key, value] : httpHeaders)
        {
            // TODO: Validate the header fileds before assign.
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
