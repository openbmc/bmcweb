// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include "boost_formatters.hpp"
#include "http_body.hpp"
#include "http_request.hpp"
#include "io_context_singleton.hpp"
#include "logging.hpp"
#include "server_sent_event.hpp"

#include <boost/asio/buffer.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/serializer.hpp>
#include <boost/beast/http/write.hpp>

#include <array>
#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace crow
{

namespace sse_socket
{

template <typename Adaptor>
class ConnectionImpl : public Connection
{
  public:
    ConnectionImpl(
        Adaptor&& adaptorIn,
        std::function<void(Connection&, const Request&)> openHandlerIn,
        std::function<void(Connection&)> closeHandlerIn) :
        adaptor(std::move(adaptorIn)), timer(getIoContext()),
        openHandler(std::move(openHandlerIn)),
        closeHandler(std::move(closeHandlerIn))

    {
        BMCWEB_LOG_DEBUG("SseConnectionImpl: SSE constructor {}", logPtr(this));
    }

    ConnectionImpl(const ConnectionImpl&) = delete;
    ConnectionImpl(const ConnectionImpl&&) = delete;
    ConnectionImpl& operator=(const ConnectionImpl&) = delete;
    ConnectionImpl& operator=(const ConnectionImpl&&) = delete;

    ~ConnectionImpl() override
    {
        BMCWEB_LOG_DEBUG("SSE ConnectionImpl: SSE destructor {}", logPtr(this));
    }

    void start(const Request& req)
    {
        BMCWEB_LOG_DEBUG("Starting SSE connection");

        res.set(boost::beast::http::field::content_type, "text/event-stream");
        boost::beast::http::response_serializer<BodyType>& serial =
            serializer.emplace(res);

        boost::beast::http::async_write_header(
            adaptor, serial,
            std::bind_front(&ConnectionImpl::sendSSEHeaderCallback, this,
                            shared_from_this(), req));
    }

    void close(const std::string_view msg) override
    {
        BMCWEB_LOG_DEBUG("Closing connection with reason {}", msg);
        // send notification to handler for cleanup
        if (closeHandler)
        {
            closeHandler(*this);
        }
        BMCWEB_LOG_DEBUG("Closing SSE connection {} - {}", logPtr(this), msg);
        boost::beast::get_lowest_layer(adaptor).close();
    }

    void sendSSEHeaderCallback(const std::shared_ptr<Connection>& /*self*/,
                               const Request& req,
                               const boost::system::error_code& ec,
                               size_t /*bytesSent*/)
    {
        serializer.reset();
        if (ec)
        {
            BMCWEB_LOG_ERROR("Error sending header{}", ec);
            close("async_write_header failed");
            return;
        }
        BMCWEB_LOG_DEBUG("SSE header sent - Connection established");
        if (!openHandler)
        {
            BMCWEB_LOG_CRITICAL("No open handler???");
            return;
        }
        openHandler(*this, req);

        // SSE stream header sent, So let us setup monitor.
        // Any read data on this stream will be error in case of SSE.
        adaptor.async_read_some(boost::asio::buffer(buffer),
                                std::bind_front(&ConnectionImpl::afterReadError,
                                                this, shared_from_this()));
    }

    void afterReadError(const std::shared_ptr<Connection>& /*self*/,
                        const boost::system::error_code& ec, size_t bytesRead)
    {
        BMCWEB_LOG_DEBUG("Read {}", bytesRead);
        if (ec == boost::asio::error::operation_aborted)
        {
            return;
        }
        if (ec)
        {
            BMCWEB_LOG_ERROR("Read error: {}", ec);
        }

        close("Close SSE connection");
    }

    void doWrite()
    {
        if (doingWrite)
        {
            return;
        }
        if (inputBuffer.size() == 0)
        {
            BMCWEB_LOG_DEBUG("inputBuffer is empty... Bailing out");
            return;
        }
        startTimeout();
        doingWrite = true;

        adaptor.async_write_some(
            inputBuffer.data(),
            std::bind_front(&ConnectionImpl::doWriteCallback, this,
                            shared_from_this()));
    }

    void doWriteCallback(const std::shared_ptr<Connection>& /*self*/,
                         const boost::beast::error_code& ec,
                         size_t bytesTransferred)
    {
        timer.cancel();
        doingWrite = false;
        inputBuffer.consume(bytesTransferred);

        if (ec == boost::asio::error::eof)
        {
            BMCWEB_LOG_ERROR("async_write_some() SSE stream closed");
            close("SSE stream closed");
            return;
        }

        if (ec)
        {
            BMCWEB_LOG_ERROR("async_write_some() failed: {}", ec.message());
            close("async_write_some failed");
            return;
        }
        BMCWEB_LOG_DEBUG("async_write_some() bytes transferred: {}",
                         bytesTransferred);

        doWrite();
    }

    void sendSseEvent(std::string_view id, std::string_view msg) override
    {
        if (msg.empty())
        {
            BMCWEB_LOG_DEBUG("Empty data, bailing out.");
            return;
        }

        dataFormat(id, msg);

        doWrite();
    }

    void dataFormat(std::string_view id, std::string_view msg)
    {
        constexpr size_t bufferLimit = 10485760U; // 10MB
        if (id.size() + msg.size() + inputBuffer.size() >= bufferLimit)
        {
            BMCWEB_LOG_ERROR("SSE Buffer overflow while waiting for client");
            close("Buffer overflow");
            return;
        }
        std::string rawData;
        if (!id.empty())
        {
            rawData += "id: ";
            rawData.append(id);
            rawData += "\n";
        }

        rawData += "data: ";
        for (char character : msg)
        {
            rawData += character;
            if (character == '\n')
            {
                rawData += "data: ";
            }
        }
        rawData += "\n\n";

        size_t copied = boost::asio::buffer_copy(
            inputBuffer.prepare(rawData.size()), boost::asio::buffer(rawData));
        inputBuffer.commit(copied);
    }

    void startTimeout()
    {
        std::weak_ptr<Connection> weakSelf = weak_from_this();
        timer.expires_after(std::chrono::seconds(30));
        timer.async_wait(std::bind_front(&ConnectionImpl::onTimeoutCallback,
                                         this, weak_from_this()));
    }

    void onTimeoutCallback(const std::weak_ptr<Connection>& weakSelf,
                           const boost::system::error_code& ec)
    {
        std::shared_ptr<Connection> self = weakSelf.lock();
        if (!self)
        {
            BMCWEB_LOG_CRITICAL("{} Failed to capture connection",
                                logPtr(self.get()));
            return;
        }

        if (ec == boost::asio::error::operation_aborted)
        {
            BMCWEB_LOG_DEBUG("Timer operation aborted");
            // Canceled wait means the path succeeded.
            return;
        }
        if (ec)
        {
            BMCWEB_LOG_CRITICAL("{} timer failed {}", logPtr(self.get()), ec);
        }

        BMCWEB_LOG_WARNING("{} Connection timed out, closing",
                           logPtr(self.get()));

        self->close("closing connection");
    }

  private:
    std::array<char, 1> buffer{};
    boost::beast::multi_buffer inputBuffer;

    Adaptor adaptor;

    using BodyType = bmcweb::HttpBody;
    boost::beast::http::response<BodyType> res;
    std::optional<boost::beast::http::response_serializer<BodyType>> serializer;
    boost::asio::steady_timer timer;
    bool doingWrite = false;

    std::function<void(Connection&, const Request&)> openHandler;
    std::function<void(Connection&)> closeHandler;
};
} // namespace sse_socket
} // namespace crow
