#pragma once
#include "async_resp.hpp"
#include "http_request.hpp"
#include "http_response.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/buffer.hpp>
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
static constexpr const std::array<const char*, 1> sseRoutes = {
    "/redfish/v1/EventService/SSE"};

struct Connection : std::enable_shared_from_this<Connection>
{
  public:
    explicit Connection(const crow::Request& reqIn) : req(reqIn)
    {}

    Connection(const Connection&) = delete;
    Connection(Connection&&) = delete;
    Connection& operator=(const Connection&) = delete;
    Connection& operator=(const Connection&&) = delete;
    virtual ~Connection() = default;

    virtual boost::asio::io_context& getIoContext() = 0;
    virtual void sendSSEHeader() = 0;
    virtual void completeRequest(crow::Response& thisRes) = 0;
    virtual void close(std::string_view msg = "quit") = 0;
    virtual void sendEvent(std::string_view id, std::string_view msg) = 0;

    crow::Request req;
};

template <typename Adaptor>
class ConnectionImpl : public Connection
{
  public:
    ConnectionImpl(
        const crow::Request& reqIn, Adaptor adaptorIn,
        std::function<void(std::shared_ptr<Connection>&, const crow::Request&,
                           const std::shared_ptr<bmcweb::AsyncResp>&)>
            openHandlerIn,
        std::function<void(std::shared_ptr<Connection>&)> closeHandlerIn) :
        Connection(reqIn),
        adaptor(std::move(adaptorIn)), openHandler(std::move(openHandlerIn)),
        closeHandler(std::move(closeHandlerIn))
    {
        BMCWEB_LOG_DEBUG << "SseConnectionImpl: SSE constructor " << this;
    }

    ConnectionImpl(const ConnectionImpl&) = delete;
    ConnectionImpl(const ConnectionImpl&&) = delete;
    ConnectionImpl& operator=(const ConnectionImpl&) = delete;
    ConnectionImpl& operator=(const ConnectionImpl&&) = delete;

    ~ConnectionImpl() override
    {
        BMCWEB_LOG_DEBUG << "SSE ConnectionImpl: SSE destructor " << this;
    }

    boost::asio::io_context& getIoContext() override
    {
        return static_cast<boost::asio::io_context&>(
            adaptor.get_executor().context());
    }

    void start()
    {

        if (openHandler)
        {
            auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
            std::shared_ptr<Connection> self = this->shared_from_this();

            asyncResp->res.setCompleteRequestHandler(
                [self(shared_from_this())](crow::Response& thisRes) {
                if (thisRes.resultInt() != 200)
                {
                    self->completeRequest(thisRes);
                }
            });

            openHandler(self, req, asyncResp);
        }
    }

    void close(const std::string_view msg) override
    {
        BMCWEB_LOG_DEBUG << "Closing SSE connection " << this << " - " << msg;
        boost::beast::get_lowest_layer(adaptor).close();

        // send notification to handler for cleanup
        if (closeHandler)
        {
            std::shared_ptr<Connection> self = this->shared_from_this();
            closeHandler(self);
        }
    }

    void sendSSEHeader() override
    {
        BMCWEB_LOG_DEBUG << "Starting SSE connection";
        using BodyType = boost::beast::http::buffer_body;
        auto response =
            std::make_shared<boost::beast::http::response<BodyType>>(
                boost::beast::http::status::ok, 11);
        auto serializer =
            std::make_shared<boost::beast::http::response_serializer<BodyType>>(
                *response);

        response->set(boost::beast::http::field::server, "bmcweb");
        response->set(boost::beast::http::field::content_type,
                      "text/event-stream");
        response->body().data = nullptr;
        response->body().size = 0;
        response->body().more = true;

        boost::beast::http::async_write_header(
            adaptor, *serializer,
            [this, self(shared_from_this()), response, serializer](
                const boost::beast::error_code& ec, const std::size_t&) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "Error sending header" << ec;
                close("async_write_header failed");
                return;
            }
            BMCWEB_LOG_DEBUG << "SSE header sent - Connection established";

            // SSE stream header sent, So lets setup monitor.
            // Any read data on this stream will be error in case of SSE.
            setupRead();
            });
    }

    void setupRead()
    {
        boost::beast::http::async_read_some(
            adaptor, outputBuffer, *parser,
            [this](const boost::system::error_code& ec, std::size_t bytesRead) {
            BMCWEB_LOG_DEBUG << "async_read_some: Read " << bytesRead
                             << " bytes";
            if (ec)
            {
                BMCWEB_LOG_ERROR << "Read error: " << ec;
            }
            outputBuffer.commit(bytesRead);
            outputBuffer.consume(bytesRead);

            // After establishing SSE stream, Reading data on this
            // stream means client is disobeys the SSE protocol.
            // Read the data to avoid buffer attacks and close connection.

            close("Close SSE connection");
            return;
            });
    }

    void doWrite()
    {
        if (doingWrite)
        {
            return;
        }
        if (inputBuffer.size() == 0)
        {
            BMCWEB_LOG_DEBUG << "inputBuffer is empty... Bailing out";
            return;
        }
        doingWrite = true;

        adaptor.async_write_some(inputBuffer.data(),
                                 [this, self(shared_from_this())](
                                     boost::beast::error_code ec,
                                     const std::size_t& bytesTransferred) {
            doingWrite = false;
            inputBuffer.consume(bytesTransferred);

            if (ec == boost::asio::error::eof)
            {
                BMCWEB_LOG_ERROR << "async_write_some() SSE stream closed";
                close("SSE stream closed");
                return;
            }

            if (ec)
            {
                BMCWEB_LOG_ERROR << "async_write_some() failed: "
                                 << ec.message();
                close("async_write_some failed");
                return;
            }
            BMCWEB_LOG_DEBUG << "async_write_some() bytes transferred: "
                             << bytesTransferred;

            doWrite();
        });
    }

    void completeRequest(crow::Response& thisRes) override
    {
        auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
        asyncResp->res = std::move(thisRes);

        if (asyncResp->res.body().empty() && !asyncResp->res.jsonValue.empty())
        {
            asyncResp->res.addHeader("Content-Type", "application/json");
            asyncResp->res.body() = asyncResp->res.jsonValue.dump(
                2, ' ', true, nlohmann::json::error_handler_t::replace);
        }

        asyncResp->res.preparePayload();
        auto serializer =
            std::make_shared<boost::beast::http::response_serializer<
                boost::beast::http::string_body>>(
                *asyncResp->res.stringResponse);

        boost::beast::http::async_write(
            adaptor, *serializer,
            [this, asyncResp, self(shared_from_this()),
             serializer](const boost::system::error_code& ec,
                         std::size_t bytesTransferred) {
            BMCWEB_LOG_DEBUG << this << " async_write " << bytesTransferred
                             << " bytes";
            if (ec)
            {
                BMCWEB_LOG_DEBUG << this << " from async_write failed";
                return;
            }

            BMCWEB_LOG_DEBUG << this
                             << " Closing SSE connection - Request invalid";
            close("Request invalid");
            asyncResp->res.releaseCompleteRequestHandler();
            });

        // delete lambda with self shared_ptr
        // to enable connection destruction
    }

    void sendEvent(const std::string_view id,
                   const std::string_view msg) override
    {
        if (msg.empty())
        {
            BMCWEB_LOG_DEBUG << "Empty data, bailing out.";
            return;
        }

        std::string rawData;
        if (!id.empty())
        {
            rawData += "id: ";
            rawData.append(id.begin(), id.end());
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

        doWrite();
    }

  private:
    Adaptor adaptor;

    boost::beast::flat_static_buffer<1024UL * 8U> outputBuffer;
    boost::beast::flat_static_buffer<1024UL * 64U> inputBuffer;
    bool doingWrite = false;
    std::optional<
        boost::beast::http::request_parser<boost::beast::http::string_body>>
        parser;

    std::function<void(std::shared_ptr<Connection>&, const crow::Request&,
                       const std::shared_ptr<bmcweb::AsyncResp>&)>
        openHandler;
    std::function<void(std::shared_ptr<Connection>&)> closeHandler;
};
} // namespace sse_socket
} // namespace crow
