#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>
#include <boost/bind.hpp>
#include <fstream>
#include <iostream>
#include <utils/json_utils.hpp>

static constexpr int httpRequestTimeout = 30;

using boost::asio::deadline_timer;
using boost::asio::ip::tcp;

using HttpReqHeadersType = std::vector<std::pair<std::string, std::string>>;

enum class ConnState
{
    initializing,
    connected,
    closed
};

class HttpClient
{
  public:
    HttpClient(boost::asio::io_service& ioc, const std::string& _host,
               const std::string& _port) :
        clientSock(ioc),
        requestTimer(ioc), r(ioc), host(_host), port(_port)
    {
        state = ConnState::initializing;
        requestTimer.async_wait(
            boost::bind(&HttpClient::checkRequestTimeout, this));
    }

    void openConnection()
    {
        openConnection(r.resolve(tcp::resolver::query(host, port)));
    }

    void setHeaders(const HttpReqHeadersType& httpHeaders)
    {
        headers = httpHeaders;
    }

    ConnState getState()
    {
        return state;
    }

    void doWrite(const std::string& path, const std::string& msgData)
    {
        if (state != ConnState::connected)
            return;

        std::ostream request_stream(&request_);
        request_stream << "POST " << path << " HTTP/1.1\r\n";
        request_stream << "Accept: */*\r\n";
        for (auto& [key, value] : headers)
        {
            request_stream << key << ": " << value << "\r\n";
        }
        request_stream << "Content-Length: " << msgData.length() << "\r\n\r\n";
        request_stream << msgData;

        boost::asio::async_write(
            clientSock, request_,
            boost::bind(&HttpClient::handleWrite, this, _1));
    }

  private:
    tcp::socket clientSock;
    deadline_timer requestTimer;
    tcp::resolver r;
    std::string host;
    std::string port;
    ConnState state;
    HttpReqHeadersType headers;
    boost::asio::streambuf response_;
    boost::asio::streambuf request_;

    void closeConnection()
    {
        BMCWEB_LOG_DEBUG << "closing Connection..\n";
        state = ConnState::closed;
        clientSock.close();
        requestTimer.cancel();
    }

    void openConnection(tcp::resolver::iterator endpoint_iter)
    {
        if (endpoint_iter != tcp::resolver::iterator())
        {
            BMCWEB_LOG_DEBUG << "Trying " << endpoint_iter->endpoint()
                             << "...\n";
            requestTimer.expires_from_now(
                boost::posix_time::seconds(httpRequestTimeout));

            // Start the asynchronous connect operation.
            clientSock.async_connect(endpoint_iter->endpoint(),
                                     boost::bind(&HttpClient::handleConnect,
                                                 this, _1, endpoint_iter));
        }
        else
        {
            closeConnection();
        }
    }

    void doRead()
    {
        requestTimer.expires_from_now(
            boost::posix_time::seconds(httpRequestTimeout));

        boost::asio::async_read_until(
            clientSock, response_, "",
            boost::bind(&HttpClient::handleRead, this, _1));
    }

    void handleConnect(const boost::system::error_code& ec,
                       tcp::resolver::iterator endpoint_iter)
    {
        if (!clientSock.is_open())
        {
            BMCWEB_LOG_ERROR << "Connect timed out\n";
            // Try the next available endpoint.
            openConnection(++endpoint_iter);
        }
        else if (ec)
        {
            BMCWEB_LOG_ERROR << "Connect error: " << ec.message() << "\n";
            clientSock.close();
            // Try the next available endpoint.
            openConnection(++endpoint_iter);
        }
        else
        {
            BMCWEB_LOG_ERROR << "Connected to " << endpoint_iter->endpoint()
                             << "\n";
            state = ConnState::connected;
        }
    }

    void handleRead(const boost::system::error_code& ec)
    {
        if (state != ConnState::connected)
            return;

        if (!ec)
        {
            std::string line;
            std::istream is(&response_);
            std::getline(is, line);

            if (!line.empty())
            {
                BMCWEB_LOG_DEBUG << "Received: " << line << "\n";
            }
        }
        else
        {
            BMCWEB_LOG_ERROR << "Error on receive: " << ec.message() << "\n";
            closeConnection();
        }
    }

    void handleWrite(const boost::system::error_code& ec)
    {
        if (state != ConnState::connected)
            return;

        if (!ec)
        {
            doRead();
        }
        else
        {
            BMCWEB_LOG_ERROR << "Error on write message: " << ec.message()
                             << "\n";
            closeConnection();
        }
    }

    void checkRequestTimeout()
    {
        if (state != ConnState::connected)
            return;

        if (requestTimer.expires_at() <= deadline_timer::traits_type::now())
        {
            requestTimer.expires_at(boost::posix_time::pos_infin);
        }

        requestTimer.async_wait(
            boost::bind(&HttpClient::checkRequestTimeout, this));
    }
};
