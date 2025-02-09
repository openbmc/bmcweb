// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include "bmcweb_config.h"

#include "boost_formatters.hpp"
#include "http_body.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "ossl_random.hpp"
#include "sessions.hpp"
#include "websocket.hpp"

#include <boost/asio/buffer.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/core/role.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/websocket/error.hpp>
#include <boost/beast/websocket/rfc6455.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <boost/beast/websocket/stream_base.hpp>
#include <boost/url/url_view.hpp>

// NOLINTNEXTLINE(misc-include-cleaner)
#include <boost/beast/websocket/ssl.hpp>

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace crow
{
namespace websocket
{

template <typename Adaptor>
class ConnectionImpl : public Connection
{
    using self_t = ConnectionImpl<Adaptor>;

  public:
    ConnectionImpl(
        const boost::urls::url_view& urlViewIn,
        const std::shared_ptr<persistent_data::UserSession>& sessionIn,
        Adaptor adaptorIn, std::function<void(Connection&)> openHandlerIn,
        std::function<void(Connection&, const std::string&, bool)>
            messageHandlerIn,
        std::function<void(crow::websocket::Connection&, std::string_view,
                           crow::websocket::MessageType type,
                           std::function<void()>&& whenComplete)>
            messageExHandlerIn,
        std::function<void(Connection&, const std::string&)> closeHandlerIn,
        std::function<void(Connection&)> errorHandlerIn) :
        uri(urlViewIn), ws(std::move(adaptorIn)), inBuffer(inString, 131088),
        openHandler(std::move(openHandlerIn)),
        messageHandler(std::move(messageHandlerIn)),
        messageExHandler(std::move(messageExHandlerIn)),
        closeHandler(std::move(closeHandlerIn)),
        errorHandler(std::move(errorHandlerIn)), session(sessionIn)
    {
        /* Turn on the timeouts on websocket stream to server role */
        ws.set_option(boost::beast::websocket::stream_base::timeout::suggested(
            boost::beast::role_type::server));
        BMCWEB_LOG_DEBUG("Creating new connection {}", logPtr(this));
    }

    void start(const crow::Request& req)
    {
        BMCWEB_LOG_DEBUG("starting connection {}", logPtr(this));

        using bf = boost::beast::http::field;
        std::string protocolHeader{
            req.getHeaderValue(bf::sec_websocket_protocol)};

        ws.set_option(boost::beast::websocket::stream_base::decorator(
            [session{session},
             protocolHeader](boost::beast::websocket::response_type& m) {
                if constexpr (!BMCWEB_INSECURE_DISABLE_CSRF)
                {
                    if (session != nullptr)
                    {
                        // use protocol for csrf checking
                        if (session->cookieAuth &&
                            !bmcweb::constantTimeStringCompare(
                                protocolHeader, session->csrfToken))
                        {
                            BMCWEB_LOG_ERROR("Websocket CSRF error");
                            m.result(boost::beast::http::status::unauthorized);
                            return;
                        }
                    }
                }
                if (!protocolHeader.empty())
                {
                    m.insert(bf::sec_websocket_protocol, protocolHeader);
                }

                m.insert(bf::strict_transport_security,
                         "max-age=31536000; "
                         "includeSubdomains; "
                         "preload");
                m.insert(bf::pragma, "no-cache");
                m.insert(bf::cache_control, "no-Store,no-Cache");
                m.insert("Content-Security-Policy", "default-src 'self'");
                m.insert("X-XSS-Protection", "1; "
                                             "mode=block");
                m.insert("X-Content-Type-Options", "nosniff");
            }));

        // Make a pointer to keep the req alive while we accept it.
        using Body = boost::beast::http::request<bmcweb::HttpBody>;
        std::unique_ptr<Body> mobile = std::make_unique<Body>(req.req);
        Body* ptr = mobile.get();
        // Perform the websocket upgrade
        ws.async_accept(*ptr,
                        std::bind_front(&self_t::acceptDone, this,
                                        shared_from_this(), std::move(mobile)));
    }

    void sendBinary(std::string_view msg) override
    {
        ws.binary(true);
        outBuffer.commit(boost::asio::buffer_copy(outBuffer.prepare(msg.size()),
                                                  boost::asio::buffer(msg)));
        doWrite();
    }

    void sendEx(MessageType type, std::string_view msg,
                std::function<void()>&& onDone) override
    {
        if (doingWrite)
        {
            BMCWEB_LOG_CRITICAL(
                "Cannot mix sendEx usage with sendBinary or sendText");
            onDone();
            return;
        }
        ws.binary(type == MessageType::Binary);

        ws.async_write(boost::asio::buffer(msg),
                       [weak(weak_from_this()), onDone{std::move(onDone)}](
                           const boost::beast::error_code& ec, size_t) {
                           std::shared_ptr<Connection> self = weak.lock();
                           if (!self)
                           {
                               BMCWEB_LOG_ERROR("Connection went away");
                               return;
                           }

                           // Call the done handler regardless of whether we
                           // errored, but before we close things out
                           onDone();

                           if (ec)
                           {
                               BMCWEB_LOG_ERROR("Error in ws.async_write {}",
                                                ec);
                               self->close("write error");
                           }
                       });
    }

    void sendText(std::string_view msg) override
    {
        ws.text(true);
        outBuffer.commit(boost::asio::buffer_copy(outBuffer.prepare(msg.size()),
                                                  boost::asio::buffer(msg)));
        doWrite();
    }

    void close(std::string_view msg) override
    {
        ws.async_close(
            {boost::beast::websocket::close_code::normal, msg},
            [self(shared_from_this())](const boost::system::error_code& ec) {
                if (ec == boost::asio::error::operation_aborted)
                {
                    return;
                }
                if (ec)
                {
                    BMCWEB_LOG_ERROR("Error closing websocket {}", ec);
                    return;
                }
            });
    }

    boost::urls::url_view url() override
    {
        return uri;
    }

    void acceptDone(const std::shared_ptr<Connection>& /*self*/,
                    const std::unique_ptr<
                        boost::beast::http::request<bmcweb::HttpBody>>& /*req*/,
                    const boost::system::error_code& ec)
    {
        if (ec)
        {
            BMCWEB_LOG_ERROR("Error in ws.async_accept {}", ec);
            return;
        }
        BMCWEB_LOG_DEBUG("Websocket accepted connection");

        if (openHandler)
        {
            openHandler(*this);
        }
        doRead();
    }

    void deferRead() override
    {
        readingDefered = true;

        // If we're not actively reading, we need to take ownership of
        // ourselves for a small portion of time, do that, and clear when we
        // resume.
        selfOwned = shared_from_this();
    }

    void resumeRead() override
    {
        readingDefered = false;
        doRead();

        // No longer need to keep ourselves alive now that read is active.
        selfOwned.reset();
    }

    void doRead()
    {
        if (readingDefered)
        {
            return;
        }
        ws.async_read(inBuffer, [this, self(shared_from_this())](
                                    const boost::beast::error_code& ec,
                                    size_t bytesRead) {
            if (ec)
            {
                if (ec != boost::beast::websocket::error::closed &&
                    ec != boost::asio::error::eof &&
                    ec != boost::asio::ssl::error::stream_truncated)
                {
                    BMCWEB_LOG_ERROR("doRead error {}", ec);
                }
                if (closeHandler)
                {
                    std::string reason{ws.reason().reason.c_str()};
                    closeHandler(*this, reason);
                }
                return;
            }

            handleMessage(bytesRead);
        });
    }
    void doWrite()
    {
        // If we're already doing a write, ignore the request, it will be picked
        // up when the current write is complete
        if (doingWrite)
        {
            return;
        }

        if (outBuffer.size() == 0)
        {
            // Done for now
            return;
        }
        doingWrite = true;
        ws.async_write(outBuffer.data(), [this, self(shared_from_this())](
                                             const boost::beast::error_code& ec,
                                             size_t bytesSent) {
            doingWrite = false;
            outBuffer.consume(bytesSent);
            if (ec == boost::beast::websocket::error::closed)
            {
                // Do nothing here.  doRead handler will call the
                // closeHandler.
                close("Write error");
                return;
            }
            if (ec)
            {
                BMCWEB_LOG_ERROR("Error in ws.async_write {}", ec);
                return;
            }
            doWrite();
        });
    }

  private:
    void handleMessage(size_t bytesRead)
    {
        if (messageExHandler)
        {
            // Note, because of the interactions with the read buffers,
            // this message handler overrides the normal message handler
            messageExHandler(*this, inString, MessageType::Binary,
                             [this, self(shared_from_this()), bytesRead]() {
                                 if (self == nullptr)
                                 {
                                     return;
                                 }

                                 inBuffer.consume(bytesRead);
                                 inString.clear();

                                 doRead();
                             });
            return;
        }

        if (messageHandler)
        {
            messageHandler(*this, inString, ws.got_text());
        }
        inBuffer.consume(bytesRead);
        inString.clear();
        doRead();
    }

    boost::urls::url uri;

    boost::beast::websocket::stream<Adaptor, false> ws;

    bool readingDefered = false;
    std::string inString;
    boost::asio::dynamic_string_buffer<std::string::value_type,
                                       std::string::traits_type,
                                       std::string::allocator_type>
        inBuffer;

    boost::beast::multi_buffer outBuffer;
    bool doingWrite = false;

    std::function<void(Connection&)> openHandler;
    std::function<void(Connection&, const std::string&, bool)> messageHandler;
    std::function<void(crow::websocket::Connection&, std::string_view,
                       crow::websocket::MessageType type,
                       std::function<void()>&& whenComplete)>
        messageExHandler;
    std::function<void(Connection&, const std::string&)> closeHandler;
    std::function<void(Connection&)> errorHandler;
    std::shared_ptr<persistent_data::UserSession> session;

    std::shared_ptr<Connection> selfOwned;
};
} // namespace websocket
} // namespace crow
