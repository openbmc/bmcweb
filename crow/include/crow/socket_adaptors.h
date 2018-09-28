#pragma once
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>

#include "crow/logging.h"

#ifdef BMCWEB_ENABLE_SSL
#include <boost/asio/ssl.hpp>
#endif
namespace crow
{
using namespace boost;
using tcp = asio::ip::tcp;

struct SocketAdaptor
{
    using streamType = tcp::socket;
    using secure = std::false_type;
    using context = void;
    SocketAdaptor(boost::asio::io_service& ioService, context* /*unused*/) :
        socketCls(ioService)
    {
    }

    boost::asio::io_service& getIoService()
    {
        return socketCls.get_io_service();
    }

    tcp::socket& rawSocket()
    {
        return socketCls;
    }

    tcp::socket& socket()
    {
        return socketCls;
    }

    std::string remoteEndpoint()
    {
        boost::system::error_code ec;
        tcp::endpoint ep = socketCls.remote_endpoint(ec);
        if (ec)
        {
            return "";
        }
        return boost::lexical_cast<std::string>(ep);
    }

    bool isOpen()
    {
        return socketCls.is_open();
    }

    void close()
    {
        socketCls.close();
    }

    template <typename F> void start(F f)
    {
        boost::system::error_code ec;
        f(ec);
    }

    tcp::socket socketCls;
    std::string authenticatedUsername;
};

struct TestSocketAdaptor
{
    using secure = std::false_type;
    using context = void;
    TestSocketAdaptor(boost::asio::io_service& ioService, context* /*unused*/) :
        socketCls(ioService)
    {
    }

    boost::asio::io_service& getIoService()
    {
        return socketCls.get_io_service();
    }

    tcp::socket& rawSocket()
    {
        return socketCls;
    }

    tcp::socket& socket()
    {
        return socketCls;
    }

    std::string remoteEndpoint()
    {
        return "Testhost";
    }

    bool isOpen()
    {
        return socketCls.is_open();
    }

    void close()
    {
        socketCls.close();
    }

    template <typename F> void start(F f)
    {
        f(boost::system::error_code());
    }

    tcp::socket socketCls;
};

#ifdef BMCWEB_ENABLE_SSL
struct SSLAdaptor
{
    using streamType = boost::asio::ssl::stream<tcp::socket>;
    using secure = std::true_type;
    using context = boost::asio::ssl::context;
    using ssl_socket_t = boost::asio::ssl::stream<tcp::socket>;
    SSLAdaptor(boost::asio::io_service& ioService, context* ctx) :
        sslSocket(new ssl_socket_t(ioService, *ctx))
    {
        sslSocket->set_verify_callback(
            [this](bool preverified, boost::asio::ssl::verify_context& ctx) {
                X509_STORE_CTX* cts = ctx.native_handle();
                X509* cert =
                    X509_STORE_CTX_get_current_cert(ctx.native_handle());
                if (cert == nullptr)
                {
                    return preverified;
                }

                int error = X509_STORE_CTX_get_error(cts);
                int32_t depth = X509_STORE_CTX_get_error_depth(cts);
                BMCWEB_LOG_DEBUG << "CTX DEPTH : " << depth;

                switch (error)
                {
                    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
                        BMCWEB_LOG_DEBUG
                            << "X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT";
                        break;
                    case X509_V_ERR_CERT_NOT_YET_VALID:
                    case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
                        BMCWEB_LOG_DEBUG << "Certificate not yet valid!!";
                        break;
                    case X509_V_ERR_CERT_HAS_EXPIRED:
                    case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
                        BMCWEB_LOG_DEBUG << "Certificate expired..";
                        break;
                    case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
                        BMCWEB_LOG_DEBUG
                            << "Self signed certificate in chain!!!";
                        break;
                    default:
                        break;
                }
                if (error == X509_V_OK)
                {
                    authenticatedUsername.resize(256, '\0');
                    X509_NAME_get_text_by_NID(X509_get_subject_name(cert),
                                              NID_commonName,
                                              &authenticatedUsername[0],
                                              authenticatedUsername.size());

                    BMCWEB_LOG_DEBUG << "Subject name="
                                     << &authenticatedUsername[0];
                }
                else
                {
                    // If anything in the cert chain is invalid, clear the
                    // username field
                    authenticatedUsername.clear();
                }
                return preverified;
            });
    }

    boost::asio::ssl::stream<tcp::socket>& socket()
    {
        return *sslSocket;
    }

    tcp::socket::lowest_layer_type& rawSocket()
    {
        return sslSocket->lowest_layer();
    }

    std::string remoteEndpoint()
    {
        boost::system::error_code ec;
        tcp::endpoint ep = rawSocket().remote_endpoint(ec);
        if (ec)
        {
            return "";
        }
        return boost::lexical_cast<std::string>(ep);
    }

    bool isOpen()
    {
        /*TODO(ed) this is a bit of a cheat.
         There are cases  when running a websocket where sslSocket might have
        std::move() called on it (to transfer ownership to
        websocket::Connection) and be empty.  This (and the check on close()) is
        a cheat to do something sane in this scenario. the correct fix would
        likely involve changing the http parser to return a specific code
        meaning "has been upgraded" so that the doRead function knows not to try
        to close the Connection which would fail, because the adapter is gone.
        As is, doRead believes the parse failed, because isOpen now returns
        False (which could also mean the client disconnected during parse)
        UPdate: The parser does in fact have an "isUpgrade" method that is
        intended for exactly this purpose.  Todo is now to make doRead obey the
        flag appropriately so this code can be changed back.
        */
        if (sslSocket != nullptr)
        {
            return sslSocket->lowest_layer().is_open();
        }
        return false;
    }

    void close()
    {
        if (sslSocket == nullptr)
        {
            return;
        }
        boost::system::error_code ec;

        // Shut it down
        this->sslSocket->lowest_layer().close();
    }

    boost::asio::io_service& getIoService()
    {
        return rawSocket().get_io_service();
    }

    template <typename F> void start(F f)
    {
        sslSocket->async_handshake(
            boost::asio::ssl::stream_base::server,
            [f](const boost::system::error_code& ec) { f(ec); });
    }

    std::unique_ptr<boost::asio::ssl::stream<tcp::socket>> sslSocket;

    std::string authenticatedUsername;
};
#endif
} // namespace crow
