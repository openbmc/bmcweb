
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
#include "node.hpp"

#include <boost/asio/strand.hpp>
#include <boost/beast/core/span.hpp>
#include <boost/beast/http/buffer_body.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/version.hpp>

#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <queue>
#include <string>

namespace crow
{

static constexpr uint8_t maxReqQueueSize = 50;

enum class SseConnState
{
    startInit,
    initInProgress,
    initialized,
    initFailed,
    sendInProgress,
    sendFailed,
    idle,
    suspended,
    closed
};

class ServerSentEvents : public std::enable_shared_from_this<ServerSentEvents>
{
  private:
    std::shared_ptr<boost::beast::tcp_stream> sseConn;
    std::queue<std::pair<uint64_t, std::string>> requestDataQueue;
    std::string outBuffer;
    SseConnState state;
    int retryCount;
    int maxRetryAttempts;

    void sendEvent(const std::string& id, const std::string& msg)
    {
        if (msg.empty())
        {
            BMCWEB_LOG_DEBUG << "Empty data, bailing out.";
            return;
        }

        if (state == SseConnState::sendInProgress)
        {
            return;
        }
        state = SseConnState::sendInProgress;

        if (!id.empty())
        {
            outBuffer += "id: ";
            outBuffer.append(id.begin(), id.end());
            outBuffer += "\n";
        }

        outBuffer += "data: ";
        for (char character : msg)
        {
            outBuffer += character;
            if (character == '\n')
            {
                outBuffer += "data: ";
            }
        }
        outBuffer += "\n\n";

        doWrite();
    }

    void doWrite()
    {
        if (outBuffer.empty())
        {
            BMCWEB_LOG_DEBUG << "All data sent successfully.";
            // Send is successful, Lets remove data from queue
            // check for next request data in queue.
            requestDataQueue.pop();
            state = SseConnState::idle;
            checkQueue();
            return;
        }

        sseConn->async_write_some(
            boost::asio::buffer(outBuffer.data(), outBuffer.size()),
            [self(shared_from_this())](
                boost::beast::error_code ec,
                [[maybe_unused]] const std::size_t& bytesTransferred) {
                self->outBuffer.erase(0, bytesTransferred);

                if (ec == boost::asio::error::eof)
                {
                    // Send is successful, Lets remove data from queue
                    // check for next request data in queue.
                    self->requestDataQueue.pop();
                    self->state = SseConnState::idle;
                    self->checkQueue();
                    return;
                }

                if (ec)
                {
                    BMCWEB_LOG_ERROR << "async_write_some() failed: "
                                     << ec.message();
                    self->state = SseConnState::sendFailed;
                    self->checkQueue();
                    return;
                }
                BMCWEB_LOG_DEBUG << "async_write_some() bytes transferred: "
                                 << bytesTransferred;

                self->doWrite();
            });
    }

    void startSSE()
    {
        if (state == SseConnState::initInProgress)
        {
            return;
        }
        state = SseConnState::initInProgress;

        BMCWEB_LOG_DEBUG << "starting SSE connection ";
        using BodyType = boost::beast::http::buffer_body;
        auto response =
            std::make_shared<boost::beast::http::response<BodyType>>(
                boost::beast::http::status::ok, 11);
        auto serializer =
            std::make_shared<boost::beast::http::response_serializer<BodyType>>(
                *response);

        // TODO: Add hostname in http header.
        response->set(boost::beast::http::field::server, "iBMC");
        response->set(boost::beast::http::field::content_type,
                      "text/event-stream");
        response->body().data = nullptr;
        response->body().size = 0;
        response->body().more = true;

        boost::beast::http::async_write_header(
            *sseConn, *serializer,
            [this, response,
             serializer](const boost::beast::error_code& ec,
                         [[maybe_unused]] const std::size_t& bytesTransferred) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Error sending header" << ec;
                    state = SseConnState::initFailed;
                    checkQueue();
                    return;
                }

                BMCWEB_LOG_DEBUG << "startSSE  Header sent.";
                state = SseConnState::initialized;
                checkQueue();
            });
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

            // TODO: Take 'DeliveryRetryPolicy' action.
            // For now, doing 'SuspendRetries' action.
            state = SseConnState::suspended;
            return;
        }

        if ((state == SseConnState::initFailed) ||
            (state == SseConnState::sendFailed))
        {
            if (newRecord)
            {
                // We are already running async wait and retry.
                // Since record is added to queue, it gets the
                // turn in FIFO.
                return;
            }

            retryCount++;
            // TODO: Perform async wait for retryTimeoutInterval before proceed.
        }
        else
        {
            // reset retry count.
            retryCount = 0;
        }

        switch (state)
        {
            case SseConnState::initInProgress:
            case SseConnState::sendInProgress:
            case SseConnState::suspended:
            case SseConnState::startInit:
            case SseConnState::closed:
                // do nothing
                break;
            case SseConnState::initFailed:
            {
                startSSE();
                break;
            }
            case SseConnState::initialized:
            case SseConnState::idle:
            case SseConnState::sendFailed:
            {
                std::pair<uint64_t, std::string> reqData =
                    requestDataQueue.front();
                sendEvent(std::to_string(reqData.first), reqData.second);
                break;
            }
        }

        return;
    }

  public:
    ServerSentEvents(const ServerSentEvents&) = delete;
    ServerSentEvents& operator=(const ServerSentEvents&) = delete;
    ServerSentEvents(ServerSentEvents&&) = delete;
    ServerSentEvents& operator=(ServerSentEvents&&) = delete;

    ServerSentEvents(const std::shared_ptr<boost::beast::tcp_stream>& adaptor) :
        sseConn(adaptor), state(SseConnState::startInit), retryCount(0),
        maxRetryAttempts(5)
    {
        startSSE();
    }

    ~ServerSentEvents() = default;

    void sendData(const uint64_t& id, const std::string& data)
    {
        if (state == SseConnState::suspended)
        {
            return;
        }

        if (requestDataQueue.size() <= maxReqQueueSize)
        {
            requestDataQueue.push(std::pair(id, data));
            checkQueue(true);
        }
        else
        {
            BMCWEB_LOG_ERROR << "Request queue is full. So ignoring data.";
        }
    }
};

} // namespace crow
