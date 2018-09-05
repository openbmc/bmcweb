#include <crow/app.h>

#include <ast_jpeg_decoder.hpp>
#include <ast_video_puller.hpp>
#include <boost/endian/arithmetic.hpp>
#include <string>

namespace crow
{
namespace kvm
{

static const std::string rfb33VersionString = "RFB 003.003\n";
static const std::string rfb37VersionString = "RFB 003.007\n";
static const std::string rfb38VersionString = "RFB 003.008\n";

enum class RfbAuthScheme : uint8_t
{
    connection_failed = 0,
    no_authentication = 1,
    vnc_authentication = 2
};

struct PixelFormatStruct
{
    boost::endian::big_uint8_t bitsPerPixel;
    boost::endian::big_uint8_t depth;
    boost::endian::big_uint8_t isBigEndian;
    boost::endian::big_uint8_t isTrueColor;
    boost::endian::big_uint16_t redMax;
    boost::endian::big_uint16_t greenMax;
    boost::endian::big_uint16_t blueMax;
    boost::endian::big_uint8_t redShift;
    boost::endian::big_uint8_t greenShift;
    boost::endian::big_uint8_t blueShift;
    boost::endian::big_uint8_t pad1;
    boost::endian::big_uint8_t pad2;
    boost::endian::big_uint8_t pad3;
};

struct ServerInitializationMsg
{
    boost::endian::big_uint16_t framebufferWidth;
    boost::endian::big_uint16_t framebufferHeight;
    PixelFormatStruct pixelFormat;
    boost::endian::big_uint32_t nameLength;
};

enum class client_to_server_msg_type : uint8_t
{
    set_pixel_format = 0,
    fix_color_map_entries = 1,
    set_encodings = 2,
    framebuffer_update_request = 3,
    key_event = 4,
    pointer_event = 5,
    client_cut_text = 6
};

enum class server_to_client_message_type : uint8_t
{
    framebuffer_update = 0,
    set_color_map_entries = 1,
    bell_message = 2,
    server_cut_text = 3
};

struct SetPixelFormatMsg
{
    boost::endian::big_uint8_t pad1;
    boost::endian::big_uint8_t pad2;
    boost::endian::big_uint8_t pad3;
    PixelFormatStruct pixelFormat;
};

struct FrameBufferUpdateReq
{
    boost::endian::big_uint8_t incremental;
    boost::endian::big_uint16_t xPosition;
    boost::endian::big_uint16_t yPosition;
    boost::endian::big_uint16_t width;
    boost::endian::big_uint16_t height;
};

struct KeyEventMsg
{
    boost::endian::big_uint8_t downFlag;
    boost::endian::big_uint8_t pad1;
    boost::endian::big_uint8_t pad2;
    boost::endian::big_uint32_t key;
};

struct PointerEventMsg
{
    boost::endian::big_uint8_t buttonMask;
    boost::endian::big_uint16_t xPosition;
    boost::endian::big_uint16_t yPosition;
};

struct ClientCutTextMsg
{
    std::vector<uint8_t> data;
};

enum class encoding_type : uint32_t
{
    raw = 0x00,
    copy_rectangle = 0x01,
    rising_rectangle = 0x02,
    corre = 0x04,
    hextile = 0x05,
    zlib = 0x06,
    tight = 0x07,
    zlibhex = 0x08,
    ultra = 0x09,
    zrle = 0x10,
    zywrle = 0x011,
    cache_enable = 0xFFFF0001,
    xor_enable = 0xFFFF0006,
    server_state_ultranvc = 0xFFFF8000,
    enable_keepAlive = 0xFFFF8001,
    enableftp_protocol_version = 0xFFFF8002,
    tight_compress_level_0 = 0xFFFFFF00,
    tight_compress_level_9 = 0xFFFFFF09,
    x_cursor = 0xFFFFFF10,
    rich_cursor = 0xFFFFFF11,
    pointer_pos = 0xFFFFFF18,
    last_rect = 0xFFFFFF20,
    new_framebuffer_size = 0xFFFFFF21,
    tight_quality_level_0 = 0xFFFFFFE0,
    tight_quality_level_9 = 0xFFFFFFE9
};

struct FramebufferRectangle
{
    boost::endian::big_uint16_t x{};
    boost::endian::big_uint16_t y{};
    boost::endian::big_uint16_t width{};
    boost::endian::big_uint16_t height{};
    boost::endian::big_uint32_t encoding{};
    std::vector<uint8_t> data;
};

struct FramebufferUpdateMsg
{
    boost::endian::big_uint8_t messageType{};
    std::vector<FramebufferRectangle> rectangles;
};

inline std::string serialize(const FramebufferUpdateMsg& msg)
{
    // calculate the size of the needed vector for serialization
    size_t vectorSize = 4;
    for (const auto& rect : msg.rectangles)
    {
        vectorSize += 12 + rect.data.size();
    }

    std::string serialized(vectorSize, 0);

    size_t i = 0;
    serialized[i++] = static_cast<char>(
        server_to_client_message_type::framebuffer_update); // Type
    serialized[i++] = 0;                                    // Pad byte
    boost::endian::big_uint16_t numberOfRectangles = msg.rectangles.size();
    std::memcpy(&serialized[i], &numberOfRectangles,
                sizeof(numberOfRectangles));
    i += sizeof(numberOfRectangles);

    for (const auto& rect : msg.rectangles)
    {
        // copy the first part of the struct
        size_t bufferSize =
            sizeof(FramebufferRectangle) - sizeof(std::vector<uint8_t>);
        std::memcpy(&serialized[i], &rect, bufferSize);
        i += bufferSize;

        std::memcpy(&serialized[i], rect.data.data(), rect.data.size());
        i += rect.data.size();
    }

    return serialized;
}

enum class VncState
{
    UNSTARTED,
    AWAITING_CLIENT_VERSION,
    AWAITING_CLIENT_AUTH_METHOD,
    AWAITING_CLIENT_INIT_msg,
    MAIN_LOOP
};

class ConnectionMetadata
{
  public:
    ConnectionMetadata(){};

    VncState vncState{VncState::UNSTARTED};
};

using meta_list = std::vector<ConnectionMetadata>;
meta_list connectionStates(10);

ConnectionMetadata meta;

template <typename... Middlewares> void requestRoutes(Crow<Middlewares...>& app)
{
    BMCWEB_ROUTE(app, "/kvmws")
        .websocket()
        .onopen([&](crow::websocket::Connection& conn) {
            if (meta.vncState == VncState::UNSTARTED)
            {
                meta.vncState = VncState::AWAITING_CLIENT_VERSION;
                conn.sendBinary(rfb38VersionString);
            }
            else
            { // SHould never happen
                conn.close();
            }
        })
        .onclose(
            [&](crow::websocket::Connection& conn, const std::string& reason) {
                meta.vncState = VncState::UNSTARTED;
            })
        .onmessage([&](crow::websocket::Connection& conn,
                       const std::string& data, bool is_binary) {
            switch (meta.vncState)
            {
                case VncState::AWAITING_CLIENT_VERSION:
                {
                    std::cout << "Client sent: " << data;
                    if (data == rfb38VersionString ||
                        data == rfb37VersionString)
                    {
                        std::string authTypes{
                            1, (uint8_t)RfbAuthScheme::no_authentication};
                        conn.sendBinary(authTypes);
                        meta.vncState = VncState::AWAITING_CLIENT_AUTH_METHOD;
                    }
                    else if (data == rfb33VersionString)
                    {
                        // TODO(ed)  Support older protocols
                        meta.vncState = VncState::UNSTARTED;
                        conn.close();
                    }
                    else
                    {
                        // TODO(ed)  Support older protocols
                        meta.vncState = VncState::UNSTARTED;
                        conn.close();
                    }
                }
                break;
                case VncState::AWAITING_CLIENT_AUTH_METHOD:
                {
                    std::string securityResult{{0, 0, 0, 0}};
                    if (data[0] == (uint8_t)RfbAuthScheme::no_authentication)
                    {
                        meta.vncState = VncState::AWAITING_CLIENT_INIT_msg;
                    }
                    else
                    {
                        // Mark auth as failed
                        securityResult[3] = 1;
                        meta.vncState = VncState::UNSTARTED;
                    }
                    conn.sendBinary(securityResult);
                }
                break;
                case VncState::AWAITING_CLIENT_INIT_msg:
                {
                    // Now send the server initialization
                    ServerInitializationMsg serverInitMsg{};
                    serverInitMsg.framebufferWidth = 800;
                    serverInitMsg.framebufferHeight = 600;
                    serverInitMsg.pixelFormat.bitsPerPixel = 32;
                    serverInitMsg.pixelFormat.isBigEndian = 0;
                    serverInitMsg.pixelFormat.isTrueColor = 1;
                    serverInitMsg.pixelFormat.redMax = 255;
                    serverInitMsg.pixelFormat.greenMax = 255;
                    serverInitMsg.pixelFormat.blueMax = 255;
                    serverInitMsg.pixelFormat.redShift = 16;
                    serverInitMsg.pixelFormat.greenShift = 8;
                    serverInitMsg.pixelFormat.blueShift = 0;
                    serverInitMsg.nameLength = 0;
                    std::cout << "size: " << sizeof(serverInitMsg);
                    // TODO(ed) this is ugly.  Crow should really have a span
                    // type interface to avoid the copy, but alas, today it does
                    // not.
                    std::string s(reinterpret_cast<char*>(&serverInitMsg),
                                  sizeof(serverInitMsg));
                    std::cout << "s.size() " << s.size();
                    conn.sendBinary(s);
                    meta.vncState = VncState::MAIN_LOOP;
                }
                break;
                case VncState::MAIN_LOOP:
                {
                    if (data.size() >= sizeof(client_to_server_msg_type))
                    {
                        auto type =
                            static_cast<client_to_server_msg_type>(data[0]);
                        std::cout << "Received client message type "
                                  << static_cast<std::size_t>(type) << "\n";
                        switch (type)
                        {
                            case client_to_server_msg_type::set_pixel_format:
                            {
                            }
                            break;

                            case client_to_server_msg_type::
                                fix_color_map_entries:
                            {
                            }
                            break;
                            case client_to_server_msg_type::set_encodings:
                            {
                            }
                            break;
                            case client_to_server_msg_type::
                                framebuffer_update_request:
                            {
                                // Make sure the buffer is long enough to handle
                                // what we're about to do
                                if (data.size() >=
                                    sizeof(FrameBufferUpdateReq) +
                                        sizeof(client_to_server_msg_type))
                                {
                                    auto msg = reinterpret_cast<
                                        const FrameBufferUpdateReq*>(
                                        data.data() + // NOLINT
                                        sizeof(client_to_server_msg_type));
                                    // TODO(ed) find a better way to do this
                                    // deserialization

                                    // Todo(ed) lifecycle of the video puller
                                    // and decoder should be with the websocket,
                                    // not recreated every time
                                    ast_video::SimpleVideoPuller p;
                                    p.initialize();
                                    auto out = p.readVideo();
                                    ast_video::AstJpegDecoder d;
                                    d.decode(out.buffer, out.width, out.height,
                                             out.mode, out.ySelector,
                                             out.uvSelector);

                                    FramebufferUpdateMsg bufferUpdateMsg;

                                    // If the viewer is requesting a full
                                    // update, force write of all pixels

                                    FramebufferRectangle thisRect;
                                    thisRect.x = msg->xPosition;
                                    thisRect.y = msg->yPosition;
                                    thisRect.width = out.width;
                                    thisRect.height = out.height;
                                    thisRect.encoding = static_cast<uint8_t>(
                                        encoding_type::raw);
                                    std::cout << "Encoding is "
                                              << thisRect.encoding;
                                    thisRect.data.reserve(
                                        static_cast<std::size_t>(
                                            thisRect.width) *
                                        static_cast<std::size_t>(
                                            thisRect.height) *
                                        4);
                                    std::cout << "Width " << out.width
                                              << " Height " << out.height;

                                    for (int i = 0; i < out.width * out.height;
                                         i++)
                                    {
                                        auto& pixel = d.outBuffer[i];
                                        thisRect.data.push_back(pixel.b);
                                        thisRect.data.push_back(pixel.g);
                                        thisRect.data.push_back(pixel.r);
                                        thisRect.data.push_back(0);
                                    }

                                    bufferUpdateMsg.rectangles.push_back(
                                        std::move(thisRect));
                                    auto serialized =
                                        serialize(bufferUpdateMsg);

                                    conn.sendBinary(serialized);

                                } // TODO(Ed) handle error
                            }

                            break;

                            case client_to_server_msg_type::key_event:
                            {
                            }
                            break;

                            case client_to_server_msg_type::pointer_event:
                            {
                            }
                            break;

                            case client_to_server_msg_type::client_cut_text:
                            {
                            }
                            break;

                            default:
                                break;
                        }
                    }
                }
                break;
                case VncState::UNSTARTED:
                    // Error?  TODO
                    break;
            }
        });
}
} // namespace kvm
} // namespace crow