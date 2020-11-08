#pragma once
#include "http/http_request.hpp"
#include "http/http_response.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core/ostream.hpp>
#include <boost/beast/http/basic_dynamic_body.hpp>

namespace crow
{

namespace streamsocket
{

struct Connection : std::enable_shared_from_this<Connection>
{
  public:
    explicit Connection(const crow::Request& reqIn) : req(reqIn.req)
    {}
    virtual void sendMessage(const std::string_view msg,
                             std::function<void()> handler) = 0;
    virtual void close() = 0;
    virtual boost::asio::io_context* getIoContext() = 0;
    virtual void sendStreamHeaders(const std::string streamDataSize) = 0;
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
                   std::function<void(Connection&)> open_handler,
                   std::function<void(Connection&, const std::string&, bool)>
                       message_handler,
                   std::function<void(Connection&)> close_handler,
                   std::function<void(Connection&)> error_handler) :

        Connection(reqIn),
        adaptor(std::move(adaptorIn)), waitTimer(*reqIn.ioService),
        openHandler(std::move(open_handler)),
        messageHandler(std::move(message_handler)),
        closeHandler(std::move(close_handler)),
        errorHandler(std::move(error_handler)), req(reqIn)
    {}

    boost::asio::io_context* getIoContext() override
    {
        return req.ioService;
    }

    void keepConnection()
    {
        waitTimer.expires_after(std::chrono::seconds(1));
        waitTimer.async_wait(
            [this, self(shared_from_this())](const boost::system::error_code&) {
                if (!streamres.isCompleted())
                {
                    keepConnection();
                }
            });
    }
    void start()
    {
        keepConnection();
        openHandler(*this);
    }

    void sendStreamErrorStatus(boost::beast::http::status status) override
    {
        streamres.result(status);
        doingWrite = true;
        boost::beast::http::async_write(
            adaptor, *streamres.bufferResponse,
            [this, self(shared_from_this())](
                const boost::system::error_code& ec2, std::size_t) {
                doingWrite = false;
                if (ec2)
                {
                    BMCWEB_LOG_DEBUG << "Error while writing on socket" << ec2;
                    close();
                    return;
                }
            });
    }

    void sendStreamHeaders(const std::string streamDataSize) override
    {

        streamres.addHeader("Content-Type", "application/octet-stream");
        streamres.addHeader("Content-Length", streamDataSize);

        doingWrite = true;
        boost::beast::http::async_write(
            adaptor, *streamres.bufferResponse,
            [this, self(shared_from_this())](
                const boost::system::error_code& ec2, std::size_t) {
                doingWrite = false;
                if (ec2)
                {
                    BMCWEB_LOG_DEBUG << "Error while writing on socket" << ec2;
                    close();
                    return;
                }
            });
    }

    void sendMessage(const std::string_view msg,
                     std::function<void()> handler) override
    {
        if (msg.size())
        {

            this->handlerFunc = handler;
            boost::beast::ostream(streamres.bufferResponse->body()) << msg;
            doWrite();
        }
        else
        {
            return;
        }
    }

    void close() override
    {
        streamres.end();
        if constexpr (std::is_same_v<Adaptor,
                                     boost::beast::ssl_stream<
                                         boost::asio::ip::tcp::socket>>)
        {
            adaptor.next_layer().close();
        }
        else
        {
            adaptor.close();
        }
        closeHandler(*this);
    }

    void doWrite()
    {
        doingWrite = true;

        BMCWEB_LOG_DEBUG << streamres.bufferResponse->body().size();
        boost::asio::async_write(
            adaptor, streamres.bufferResponse->body().data(),
            [this, self(shared_from_this())](boost::beast::error_code ec,
                                             std::size_t bytes_written) {
                streamres.bufferResponse->body().consume(bytes_written);
                doingWrite = false;

                if (ec == boost::asio::error::eof)
                {
                    close();
                    return;
                }
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Error in async_write " << ec;
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
} // namespace streamsocket
} // namespace crow
