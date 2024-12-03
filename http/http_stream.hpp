#pragma once
#include "http/http_request.hpp"
#include "http/http_response.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core/ostream.hpp>
#include <boost/beast/http/basic_dynamic_body.hpp>
#include <boost/system/error_code.hpp>

#include <functional>
#include <memory>
#include <string>

namespace crow
{

namespace streaming_response
{

struct Connection : std::enable_shared_from_this<Connection>
{
  public:
    explicit Connection(const crow::Request& reqIn) : req(reqIn) {}
    virtual void sendMessage(const boost::asio::mutable_buffer& buffer,
                             std::function<void()> handler) = 0;
    virtual void close() = 0;
    virtual boost::asio::io_context* getIoContext() = 0;
    virtual void sendStreamHeaders(const std::string& streamDataSize,
                                   const std::string& contentType) = 0;
    virtual void sendStreamErrorStatus(boost::beast::http::status status) = 0;
    virtual void setStreamHeaders(const std::string& header,
                                  const std::string& headerValue) = 0;
    virtual ~Connection() = default;
    Connection(const Connection&) = delete;
    Connection(Connection&&) = delete;
    Connection& operator=(const Connection& c) = delete;
    Connection& operator=(Connection&& c) = delete;

    crow::Request req;
    crow::DynamicResponse streamres;
    bool completionStatus = false;
};

template <typename Adaptor>
class ConnectionImpl : public Connection
{
  public:
    ConnectionImpl(const crow::Request& reqIn, Adaptor&& adaptorIn,
                   std::function<void(Connection&)> openHandlerIn,
                   std::function<void(Connection&, const std::string&, bool)>
                       messageHandlerIn,
                   std::function<void(Connection&, bool&)> closeHandlerIn,
                   std::function<void(Connection&)> errorHandlerIn) :

        Connection(reqIn), adaptor(std::move(adaptorIn)),
        waitTimer(*reqIn.ioService), openHandler(std::move(openHandlerIn)),
        messageHandler(std::move(messageHandlerIn)),
        closeHandler(std::move(closeHandlerIn)),
        errorHandler(std::move(errorHandlerIn))
    {}

    boost::asio::io_context* getIoContext() override
    {
        return req.ioService;
    }

    void start()
    {
        streamres.completeRequestHandler = [this, self(shared_from_this())] {
            BMCWEB_LOG_DEBUG("running completeRequestHandler");
            this->close();
        };
        openHandler(*this);
    }

    void sendStreamErrorStatus(boost::beast::http::status status) override
    {
        streamres.result(status);
        boost::beast::http::async_write(
            adaptor, *streamres.bufferResponse,
            [this, self(shared_from_this())](
                const boost::system::error_code& ec2, std::size_t) {
                if (ec2)
                {
                    BMCWEB_LOG_DEBUG("Error while writing on socket {}", ec2);
                    close();
                    return;
                }
            });
    }

    void setStreamHeaders(const std::string& header,
                          const std::string& headerValue) override
    {
        streamres.addHeader(header, headerValue);
    }

    void sendStreamHeaders(const std::string& streamDataSize,
                           const std::string& contentType) override
    {
        streamres.addHeader("Content-Length", streamDataSize);
        streamres.addHeader("Content-Type", contentType);
        boost::beast::http::async_write(
            adaptor, *streamres.bufferResponse,
            [this, self(shared_from_this())](
                const boost::system::error_code& ec2, std::size_t) {
                if (ec2)
                {
                    BMCWEB_LOG_DEBUG("Error while writing on socket {}", ec2);
                    close();
                    return;
                }
            });
    }

    void sendMessage(const boost::asio::mutable_buffer& buffer,
                     std::function<void()> handler) override
    {
        if (buffer.size() != 0)
        {
            this->handlerFunc = handler;
            auto bytes = boost::asio::buffer_copy(
                streamres.bufferResponse->body().prepare(buffer.size()),
                buffer);
            streamres.bufferResponse->body().commit(bytes);
            doWrite();
        }
    }

    void close() override
    {
        streamres.end();
        boost::beast::get_lowest_layer(adaptor).close();
        closeHandler(*this, completionStatus);
    }

    void doWrite()
    {
        boost::asio::async_write(
            adaptor, streamres.bufferResponse->body().data(),
            [this, self(shared_from_this())](boost::beast::error_code ec,
                                             std::size_t bytesWritten) {
                streamres.bufferResponse->body().consume(bytesWritten);

                if (ec)
                {
                    BMCWEB_LOG_DEBUG("Error in async_write {}", ec);
                    close();
                    return;
                }
                (handlerFunc)();
            });
    }

  private:
    Adaptor adaptor;
    boost::asio::steady_timer waitTimer;
    bool doingWrite = false;
    std::function<void(Connection&)> openHandler;
    std::function<void(Connection&, const std::string&, bool)> messageHandler;
    std::function<void(Connection&, bool&)> closeHandler;
    std::function<void(Connection&)> errorHandler;
    std::function<void()> handlerFunc;
};
} // namespace streaming_response
} // namespace crow
