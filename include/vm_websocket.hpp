#pragma once

#include <app.hpp>
#include <boost/beast/core/flat_static_buffer.hpp>
#include <boost/process.hpp>
#include <websocket.hpp>

#include <csignal>

namespace crow
{
namespace obmc_vm
{

// The max network block device buffer size is 128kb plus 16bytes
// for the message header:
// https://github.com/NetworkBlockDevice/nbd/blob/master/doc/proto.md#simple-reply-message
static constexpr auto nbdBufferSize = 131088;

class Handler : public std::enable_shared_from_this<Handler>
{
  public:
    Handler(const std::string& mediaIn, boost::asio::io_context& ios) :
        pipeOut(ios), pipeIn(ios), media(mediaIn), doingWrite(false),
        session(nullptr),
        outputBuffer(new boost::beast::flat_static_buffer<nbdBufferSize>),
        inputBuffer(new boost::beast::flat_static_buffer<nbdBufferSize>)
    {}

    ~Handler() = default;

    void update_mediasession(int user_count)
    {
        std::ofstream fileWriter;
        fileWriter.open("/etc/nbd-proxy/mediasession",
                        std::ios::out | std::ios::trunc);
        if (fileWriter)
        {
            fileWriter << user_count;
            fileWriter.close();
        }
    }

    void doClose()
    {
        // boost::process::child::terminate uses SIGKILL, need to send SIGTERM
        // to allow the proxy to stop nbd-client and the USB device gadget.
        int rc = kill(proxy.id(), SIGTERM);
        if (rc)
        {
            BMCWEB_LOG_ERROR
                << "Couldn't found Child PID to send SIGTERM signal";
            return;
        }
        proxy.wait();
    }

    void connect()
    {
        std::error_code ec;
        proxy = boost::process::child("/usr/sbin/nbd-proxy", media,
                                      boost::process::std_out > pipeOut,
                                      boost::process::std_in < pipeIn, ec);
        if (ec)
        {
            BMCWEB_LOG_ERROR << "Couldn't connect to nbd-proxy: "
                             << ec.message();
            if (session != nullptr)
            {
                session->close("Error connecting to nbd-proxy");
            }
            return;
        }
        doWrite();
        doRead();
    }

    void doWrite()
    {
        if (doingWrite)
        {
            BMCWEB_LOG_DEBUG << "Already writing.  Bailing out";
            return;
        }

        if (inputBuffer->size() == 0)
        {
            BMCWEB_LOG_DEBUG << "inputBuffer empty.  Bailing out";
            return;
        }

        doingWrite = true;
        pipeIn.async_write_some(
            inputBuffer->data(),
            [this, self(shared_from_this())](boost::beast::error_code ec,
                                             std::size_t bytesWritten) {
                BMCWEB_LOG_DEBUG << "Wrote " << bytesWritten << "bytes,session"
                                 << session;
                doingWrite = false;
                inputBuffer->consume(bytesWritten);

                if (session == nullptr)
                {
                    BMCWEB_LOG_ERROR << "Session closed it is set to null "
                                     << ec;
                    return;
                }
                if (ec == boost::asio::error::eof)
                {
                    session->close("VM socket port closed");
                    return;
                }
                if (ec)
                {
                    session->close("Error in writing to proxy port");
                    BMCWEB_LOG_ERROR << "Error in VM socket write " << ec;
                    return;
                }
                doWrite();
            });
    }

    void doRead()
    {
        std::size_t bytes = outputBuffer->capacity() - outputBuffer->size();

        pipeOut.async_read_some(
            outputBuffer->prepare(bytes),
            [this, self(shared_from_this())](
                const boost::system::error_code& ec, std::size_t bytesRead) {
                BMCWEB_LOG_DEBUG << "Read done.  Read " << bytesRead
                                 << " bytes, session-" << session;
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Couldn't read from VM port: " << ec;
                    if (session != nullptr)
                    {
                        session->close("Error in connecting to VM port");
                    }
                    return;
                }
                if (session == nullptr)
                {
                    return;
                }

                outputBuffer->commit(bytesRead);
                std::string_view payload(
                    static_cast<const char*>(outputBuffer->data().data()),
                    bytesRead);
                session->sendBinary(payload);
                outputBuffer->consume(bytesRead);

                doRead();
            });
    }

    boost::process::async_pipe pipeOut;
    boost::process::async_pipe pipeIn;
    boost::process::child proxy;
    std::string media;
    bool doingWrite;
    crow::websocket::Connection* session;

    std::unique_ptr<boost::beast::flat_static_buffer<nbdBufferSize>>
        outputBuffer;
    std::unique_ptr<boost::beast::flat_static_buffer<nbdBufferSize>>
        inputBuffer;
};

static std::shared_ptr<Handler> g_handler[4][2] = {0};

inline void getmediaClient(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    std::string file_path = "/etc/nbd-proxy/mediasession";
    std::ifstream fileReader;
    fileReader.open(file_path);
    if (fileReader)
    {
        std::string temp_buf;
        std::getline(fileReader, temp_buf);

        if (temp_buf == "0" || temp_buf == "1" || temp_buf == "2")
        {
            asyncResp->res.jsonValue["Media_session_id"] = temp_buf;
            asyncResp->res.end();
        }

        else
        {
            BMCWEB_LOG_ERROR << "Invalid Client Id";
            asyncResp->res.jsonValue["Media_session_id"] = "Invalid Client Id";
        }
        fileReader.close();
    }

    else
    {
        BMCWEB_LOG_ERROR << "File Not found";
        asyncResp->res.jsonValue["Media_session_id"] = "File Not Found";
        return;
    }
}

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/vm/<str>/<str>")
        .privileges({{"ConfigureComponents", "ConfigureManager"}})
        .websocket()
        .onopen([](crow::websocket::Connection& conn,
                   const std::shared_ptr<bmcweb::AsyncResp>&) {
            BMCWEB_LOG_DEBUG << "Connection " << &conn << " opened";
            std::string target_name;
            target_name = conn.req.target();

            int user_session, endpoint;
            if (strstr(target_name.c_str(), "/vm/"))
            {
                std::stringstream session_name;
                session_name << target_name.at(4);
                session_name >> user_session;

                std::stringstream media_endpoint;
                media_endpoint << target_name.at(6);
                media_endpoint >> endpoint;
            }

            else
            {
                conn.close("target did not matched...!");
                return;
            }

            std::string media = std::to_string((2 * user_session) + endpoint);
            BMCWEB_LOG_ERROR << "media- " << media << "user_session-"
                             << user_session << "endpoint" << endpoint;
            std::shared_ptr<Handler> handler;

            handler = std::make_shared<Handler>(media, conn.getIoContext());
            g_handler[user_session][endpoint] = handler;
            handler->session = &conn;
            handler->connect();
            int client_id;
            for (client_id = 0; client_id <= 3; client_id++)
            {
                if (g_handler[client_id][0] == 0 &&
                    g_handler[client_id][1] == 0)
                {
                    handler->update_mediasession(client_id);
                    if (client_id == 3)
                        BMCWEB_LOG_ERROR << "Media session reached Max number "
                                            "of users ...!!! ";

                    break;
                }
            }
        })
        .onclose([](crow::websocket::Connection& conn,
                    const std::string& /*reason*/) {
            /*if (&conn != session)
              {
              return;
              } */

            std::shared_ptr<Handler> handler;
            int client_id, endpoint_id;
            for (client_id = 0; client_id < 3; client_id++)
            {
                for (endpoint_id = 0; endpoint_id < 2; endpoint_id++)
                {
                    if (g_handler[client_id][endpoint_id] != 0 &&
                        g_handler[client_id][endpoint_id]->session == &conn)
                    {
                        handler = g_handler[client_id][endpoint_id];
                        g_handler[client_id][endpoint_id] = 0;
                        goto handler_found;
                    }
                }
            }
        handler_found:
            if (client_id == 3)
            {

                BMCWEB_LOG_ERROR << "Handler not found for connection - "
                                 << &conn;
                return;
            }

            if (endpoint_id == 0)
                sleep(2);
            else
                sleep(4);

            // session = nullptr;
            handler->doClose();
            handler->session->close("VM socket port closed");
            handler->session = nullptr;
            BMCWEB_LOG_DEBUG << "Connection " << &conn << " closed";
            g_handler[client_id][endpoint_id] = 0;
            for (client_id = 0; client_id <= 3; client_id++)
            {
                if (g_handler[client_id][0] == 0 &&
                    g_handler[client_id][1] == 0)
                {
                    handler->update_mediasession(client_id);
                    if (client_id == 3)
                        BMCWEB_LOG_ERROR << "Media session reached Max number "
                                            "of users ...!!! ";

                    break;
                }
            }
        })
        .onmessage([](crow::websocket::Connection& conn,
                      const std::string& data, bool) {
            std::shared_ptr<Handler> handler;
            int client_id, endpoint_id;
            for (client_id = 0; client_id < 3; client_id++)
            {
                for (endpoint_id = 0; endpoint_id < 2; endpoint_id++)
                {
                    if (g_handler[client_id][endpoint_id] != 0 &&
                        g_handler[client_id][endpoint_id]->session == &conn)
                    {
                        handler = g_handler[client_id][endpoint_id];
                        BMCWEB_LOG_DEBUG << "handler_onmessgae is ==>  "
                                         << handler << " \n";
                        goto handler_found;
                    }
                }
            }
        handler_found:
            if (client_id == 3)
            {
                BMCWEB_LOG_ERROR << "No handler found with connection - "
                                 << &conn;
                return;
            }

            if (data.length() > handler->inputBuffer->capacity())
            {
                BMCWEB_LOG_ERROR << "Buffer overrun when writing "
                                 << data.length() << " bytes";
                conn.close("Buffer overrun");
                return;
            }

            boost::asio::buffer_copy(handler->inputBuffer->prepare(data.size()),
                                     boost::asio::buffer(data));
            handler->inputBuffer->commit(data.size());
            handler->doWrite();
        });

    BMCWEB_ROUTE(app, "/vmedia/client_Id")
        .privileges({{"ConfigureComponents", "ConfigureManager"}})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                getmediaClient(asyncResp);
            });
}

} // namespace obmc_vm
} // namespace crow
