#pragma once
#include "dbus_singleton.hpp"
#include "http_request.hpp"
#include "http_response.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/http/buffer_body.hpp>
#include <boost/beast/websocket.hpp>

#include <array>
#include <functional>

#ifdef BMCWEB_ENABLE_SSL
#include <boost/beast/websocket/ssl.hpp>
#endif

namespace crow
{

namespace sse_socket
{
struct Connection : std::enable_shared_from_this<Connection>
{
  public:
    Connection() = default;

    Connection(const Connection&) = delete;
    Connection(Connection&&) = delete;
    Connection& operator=(const Connection&) = delete;
    Connection& operator=(const Connection&&) = delete;
    virtual ~Connection() = default;

    virtual boost::asio::io_context& getIoContext() = 0;
    virtual void close(std::string_view msg = "quit") = 0;
    virtual void sendEvent(std::string_view id, std::string_view msg) = 0;
};

template <typename Adaptor>
class ConnectionImpl : public Connection
{
  public:
    ConnectionImpl(Adaptor&& adaptorIn,
                   std::function<void(Connection&)> openHandlerIn,
                   std::function<void(Connection&)> closeHandlerIn) :
        adaptor(std::move(adaptorIn)),
        timer(ioc), openHandler(std::move(openHandlerIn)),
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

    boost::asio::io_context& getIoContext() override
    {
        return static_cast<boost::asio::io_context&>(
            adaptor.get_executor().context());
    }

    void start()
    {
        if (!openHandler)
        {
            BMCWEB_LOG_CRITICAL("No open handler???");
            return;
        }
        openHandler(*this);
    }

    void close(const std::string_view msg) override
    {
        // send notification to handler for cleanup
        if (closeHandler)
        {
            closeHandler(*this);
        }
        BMCWEB_LOG_DEBUG("Closing SSE connection {} - {}", logPtr(this), msg);
        boost::beast::get_lowest_layer(adaptor).close();
    }

    void sendSSEHeader()
    {
        BMCWEB_LOG_DEBUG("Starting SSE connection");
        using BodyType = boost::beast::http::buffer_body;
        boost::beast::http::response<BodyType> res(
            boost::beast::http::status::ok, 11, BodyType{});
        res.set(boost::beast::http::field::content_type, "text/event-stream");
        res.body().more = true;
        boost::beast::http::response_serializer<BodyType>& ser =
            serializer.emplace(std::move(res));

        boost::beast::http::async_write_header(
            adaptor, ser,
            std::bind_front(&ConnectionImpl::sendSSEHeaderCallback, this,
                            shared_from_this()));
    }

    void sendSSEHeaderCallback(const std::shared_ptr<Connection>& /*self*/,
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

        serializer.reset();

        // SSE stream header sent, So let us setup monitor.
        // Any read data on this stream will be error in case of SSE.

        adaptor.async_wait(boost::asio::ip::tcp::socket::wait_error,
                           std::bind_front(&ConnectionImpl::afterReadError,
                                           this, shared_from_this()));
    }

    void afterReadError(const std::shared_ptr<Connection>& /*self*/,
                        const boost::system::error_code& ec)
    {
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
                            weak_from_this()));
    }

    void doWriteCallback(const std::weak_ptr<Connection>& weak,
                         const boost::beast::error_code& ec,
                         size_t bytesTransferred)
    {
        auto self = weak.lock();
        if (self == nullptr)
        {
            return;
        }
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

    void sendEvent(std::string_view id, std::string_view msg) override
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

        boost::asio::buffer_copy(inputBuffer.prepare(rawData.size()),
                                 boost::asio::buffer(rawData));
        inputBuffer.commit(rawData.size());
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
            BMCWEB_LOG_DEBUG("operation aborted");
            // Canceled wait means the path succeeeded.
            return;
        }
        if (ec)
        {
            BMCWEB_LOG_CRITICAL("{} timer failed {}", logPtr(self.get()), ec);
        }

        BMCWEB_LOG_WARNING("{}Connection timed out, closing",
                           logPtr(self.get()));

        self->close("closing connection");
    }

  private:
    Adaptor adaptor;

    boost::beast::multi_buffer inputBuffer;

    std::optional<boost::beast::http::response_serializer<
        boost::beast::http::buffer_body>>
        serializer;
    boost::asio::io_context& ioc =
        crow::connections::systemBus->get_io_context();
    boost::asio::steady_timer timer;
    bool doingWrite = false;

    std::function<void(Connection&)> openHandler;
    std::function<void(Connection&)> closeHandler;
};
} // namespace sse_socket
} // namespace crow
