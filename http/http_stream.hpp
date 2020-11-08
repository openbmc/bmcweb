#pragma once
#include "http/http_request.hpp"
#include "http/http_response.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core/ostream.hpp>

namespace crow
{

namespace streaming_response
{

struct Connection : std::enable_shared_from_this<Connection>
{
  public:
    explicit Connection(const crow::Request& reqIn) : req(reqIn.req)
    {}
    virtual void sendMessage(char* data, std::function<void()> handler,
                             size_t size) = 0;
    virtual void close() = 0;
    virtual boost::asio::io_context* getIoContext() = 0;
    virtual void sendStreamHeaders(const std::string& streamDataSize,
                                   const std::string& contentType) = 0;
    virtual void sendStreamErrorStatus(boost::beast::http::status status) = 0;
    virtual ~Connection() = default;

    boost::beast::http::request<boost::beast::http::string_body> req;

    crow::DynamicResponse streamres;
};

template <typename Adaptor>
class ConnectionImpl : public Connection
{
  public:
    ConnectionImpl(const crow::Request& reqIn, Adaptor&& adaptorIn,
                   std::function<void(Connection&)> openHandler,
                   std::function<void(Connection&, const std::string&, bool)>
                       messageHandler,
                   std::function<void(Connection&)> closeHandler,
                   std::function<void(Connection&)> errorHandler) :

        Connection(reqIn),
        adaptor(std::move(adaptorIn)), waitTimer(*reqIn.ioService),
        openHandler(std::move(openHandler)),
        messageHandler(std::move(messageHandler)),
        closeHandler(std::move(closeHandler)),
        errorHandler(std::move(errorHandler)), req(reqIn)
    {}

    boost::asio::io_context* getIoContext() override
    {
        return req.ioService;
    }

    void start()
    {
        streamres.completeRequestHandler = [this, self(shared_from_this())] {
            BMCWEB_LOG_DEBUG << "running completeRequestHandler";
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
                    BMCWEB_LOG_DEBUG << "Error while writing on socket" << ec2;
                    close();
                    return;
                }
            });
    }

    void sendStreamHeaders(const std::string& streamDataSize,
                           const std::string& contentType) override
    {
        streamres.addHeader("Content-Length", streamDataSize);
        streamres.addHeader("Content-Type", contentType);
        streamres.bufferResponse->chunked(false);
    }

    void sendMessage(char* data, std::function<void()> handler,
                     size_t size) override
    {
        if (size)
        {
            this->handlerFunc = handler;
            streamres.bufferResponse->body().more = true;
            streamres.bufferResponse->body().data = data;
            streamres.bufferResponse->body().size = size;
            doWrite();
        }
    }

    void close() override
    {
        streamres.end();
        boost::beast::get_lowest_layer(adaptor).close();
        closeHandler(*this);
    }

    void doWrite()
    {
        boost::beast::http::async_write(
            adaptor, *streamres.bufferResponse,
            [this, self(shared_from_this())](boost::beast::error_code ec,
                                             std::size_t bytesWritten) {
                BMCWEB_LOG_DEBUG << "async_write wrote " << bytesWritten << ec;

                if (ec)
                {
                    BMCWEB_LOG_DEBUG << "Error in async_write " << ec;
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
    std::function<void(Connection&)> closeHandler;
    std::function<void(Connection&)> errorHandler;
    std::function<void()> handlerFunc;
    crow::Request req;
};
} // namespace streaming_response
} // namespace crow
