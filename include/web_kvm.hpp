#include <string>
#include <crow/app.h>
#include <boost/endian/arithmetic.hpp>

#include <ast_jpeg_decoder.hpp>
#include <ast_video_puller.hpp>

namespace crow {
namespace kvm {

static const std::string rfb_3_3_version_string = "RFB 003.003\n";
static const std::string rfb_3_7_version_string = "RFB 003.007\n";
static const std::string rfb_3_8_version_string = "RFB 003.008\n";

enum class RfbAuthScheme : uint8_t {
  connection_failed = 0,
  no_authentication = 1,
  vnc_authentication = 2
};

struct pixel_format_struct {
  boost::endian::big_uint8_t bits_per_pixel;
  boost::endian::big_uint8_t depth;
  boost::endian::big_uint8_t is_big_endian;
  boost::endian::big_uint8_t is_true_color;
  boost::endian::big_uint16_t red_max;
  boost::endian::big_uint16_t green_max;
  boost::endian::big_uint16_t blue_max;
  boost::endian::big_uint8_t red_shift;
  boost::endian::big_uint8_t green_shift;
  boost::endian::big_uint8_t blue_shift;
  boost::endian::big_uint8_t pad1;
  boost::endian::big_uint8_t pad2;
  boost::endian::big_uint8_t pad3;
};

struct server_initialization_msg {
  boost::endian::big_uint16_t framebuffer_width;
  boost::endian::big_uint16_t framebuffer_height;
  pixel_format_struct pixel_format;
  boost::endian::big_uint32_t name_length;
};

enum class client_to_server_msg_type : uint8_t {
  set_pixel_format = 0,
  fix_color_map_entries = 1,
  set_encodings = 2,
  framebuffer_update_request = 3,
  key_event = 4,
  pointer_event = 5,
  client_cut_text = 6
};

enum class server_to_client_message_type : uint8_t {
  framebuffer_update = 0,
  set_color_map_entries = 1,
  bell_message = 2,
  server_cut_text = 3
};

struct set_pixel_format_msg {
  boost::endian::big_uint8_t pad1;
  boost::endian::big_uint8_t pad2;
  boost::endian::big_uint8_t pad3;
  pixel_format_struct pixel_format;
};

struct frame_buffer_update_req {
  boost::endian::big_uint8_t incremental;
  boost::endian::big_uint16_t x_position;
  boost::endian::big_uint16_t y_position;
  boost::endian::big_uint16_t width;
  boost::endian::big_uint16_t height;
};

struct key_event_msg {
  boost::endian::big_uint8_t down_flag;
  boost::endian::big_uint8_t pad1;
  boost::endian::big_uint8_t pad2;
  boost::endian::big_uint32_t key;
};

struct pointer_event_msg {
  boost::endian::big_uint8_t button_mask;
  boost::endian::big_uint16_t x_position;
  boost::endian::big_uint16_t y_position;
};

struct client_cut_text_msg {
  std::vector<uint8_t> data;
};

enum class encoding_type : uint32_t {
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
  enable_keep_alive = 0xFFFF8001,
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

struct framebuffer_rectangle {
  boost::endian::big_uint16_t x{};
  boost::endian::big_uint16_t y{};
  boost::endian::big_uint16_t width{};
  boost::endian::big_uint16_t height{};
  boost::endian::big_uint32_t encoding{};
  std::vector<uint8_t> data;
};

struct framebuffer_update_msg {
  boost::endian::big_uint8_t message_type{};
  std::vector<framebuffer_rectangle> rectangles;
};

inline std::string serialize(const framebuffer_update_msg& msg) {
  // calculate the size of the needed vector for serialization
  size_t vector_size = 4;
  for (const auto& rect : msg.rectangles) {
    vector_size += 12 + rect.data.size();
  }

  std::string serialized(vector_size, 0);

  size_t i = 0;
  serialized[i++] = static_cast<char>(
      server_to_client_message_type::framebuffer_update);  // Type
  serialized[i++] = 0;                                     // Pad byte
  boost::endian::big_uint16_t number_of_rectangles = msg.rectangles.size();
  std::memcpy(&serialized[i], &number_of_rectangles,
              sizeof(number_of_rectangles));
  i += sizeof(number_of_rectangles);

  for (const auto& rect : msg.rectangles) {
    // copy the first part of the struct
    size_t buffer_size =
        sizeof(framebuffer_rectangle) - sizeof(std::vector<uint8_t>);
    std::memcpy(&serialized[i], &rect, buffer_size);
    i += buffer_size;

    std::memcpy(&serialized[i], rect.data.data(), rect.data.size());
    i += rect.data.size();
  }

  return serialized;
}

enum class VncState {
  UNSTARTED,
  AWAITING_CLIENT_VERSION,
  AWAITING_CLIENT_AUTH_METHOD,
  AWAITING_CLIENT_INIT_msg,
  MAIN_LOOP
};

class connection_metadata {
 public:
  connection_metadata() {};

  VncState vnc_state{VncState::UNSTARTED};
};

using meta_list = std::vector<connection_metadata>;
meta_list connection_states(10);

connection_metadata meta;

template <typename... Middlewares>
void request_routes(Crow<Middlewares...>& app) {
  CROW_ROUTE(app, "/kvmws")
      .websocket()
      .onopen([&](crow::websocket::connection& conn) {
        if (meta.vnc_state == VncState::UNSTARTED) {
          meta.vnc_state = VncState::AWAITING_CLIENT_VERSION;
          conn.send_binary(rfb_3_8_version_string);
        } else {  // SHould never happen
          conn.close();
        }

      })
      .onclose(
          [&](crow::websocket::connection& conn, const std::string& reason) {
            meta.vnc_state = VncState::UNSTARTED;
          })
      .onmessage([&](crow::websocket::connection& conn, const std::string& data,
                     bool is_binary) {
        switch (meta.vnc_state) {
          case VncState::AWAITING_CLIENT_VERSION: {
            std::cout << "Client sent: " << data;
            if (data == rfb_3_8_version_string ||
                data == rfb_3_7_version_string) {
              std::string auth_types{1,
                                     (uint8_t)RfbAuthScheme::no_authentication};
              conn.send_binary(auth_types);
              meta.vnc_state = VncState::AWAITING_CLIENT_AUTH_METHOD;
            } else if (data == rfb_3_3_version_string) {
              // TODO(ed)  Support older protocols
              meta.vnc_state = VncState::UNSTARTED;
              conn.close();
            } else {
              // TODO(ed)  Support older protocols
              meta.vnc_state = VncState::UNSTARTED;
              conn.close();
            }
          } break;
          case VncState::AWAITING_CLIENT_AUTH_METHOD: {
            std::string security_result{{0, 0, 0, 0}};
            if (data[0] == (uint8_t)RfbAuthScheme::no_authentication) {
              meta.vnc_state = VncState::AWAITING_CLIENT_INIT_msg;
            } else {
              // Mark auth as failed
              security_result[3] = 1;
              meta.vnc_state = VncState::UNSTARTED;
            }
            conn.send_binary(security_result);
          } break;
          case VncState::AWAITING_CLIENT_INIT_msg: {
            // Now send the server initialization
            server_initialization_msg server_init_msg{};
            server_init_msg.framebuffer_width = 800;
            server_init_msg.framebuffer_height = 600;
            server_init_msg.pixel_format.bits_per_pixel = 32;
            server_init_msg.pixel_format.is_big_endian = 0;
            server_init_msg.pixel_format.is_true_color = 1;
            server_init_msg.pixel_format.red_max = 255;
            server_init_msg.pixel_format.green_max = 255;
            server_init_msg.pixel_format.blue_max = 255;
            server_init_msg.pixel_format.red_shift = 16;
            server_init_msg.pixel_format.green_shift = 8;
            server_init_msg.pixel_format.blue_shift = 0;
            server_init_msg.name_length = 0;
            std::cout << "size: " << sizeof(server_init_msg);
            // TODO(ed) this is ugly.  Crow should really have a span type
            // interface
            // to avoid the copy, but alas, today it does not.
            std::string s(reinterpret_cast<char*>(&server_init_msg),
                          sizeof(server_init_msg));
            std::cout << "s.size() " << s.size();
            conn.send_binary(s);
            meta.vnc_state = VncState::MAIN_LOOP;
          } break;
          case VncState::MAIN_LOOP: {
            if (data.size() >= sizeof(client_to_server_msg_type)) {
              auto type = static_cast<client_to_server_msg_type>(data[0]);
              std::cout << "Received client message type "
                        << static_cast<std::size_t>(type) << "\n";
              switch (type) {
                case client_to_server_msg_type::set_pixel_format: {
                } break;

                case client_to_server_msg_type::fix_color_map_entries: {
                } break;
                case client_to_server_msg_type::set_encodings: {
                } break;
                case client_to_server_msg_type::framebuffer_update_request: {
                  // Make sure the buffer is long enough to handle what we're
                  // about to do
                  if (data.size() >= sizeof(frame_buffer_update_req) +
                                         sizeof(client_to_server_msg_type)) {
                    auto msg = reinterpret_cast<const frame_buffer_update_req*>(
                        data.data() +   // NOLINT
                        sizeof(client_to_server_msg_type));
                    // TODO(ed) find a better way to do this deserialization

                    // Todo(ed) lifecycle of the video puller and decoder
                    // should be
                    // with the websocket, not recreated every time
                    AstVideo::SimpleVideoPuller p;
                    p.initialize();
                    auto out = p.read_video();
                    AstVideo::AstJpegDecoder d;
                    d.decode(out.buffer, out.width, out.height, out.mode,
                             out.y_selector, out.uv_selector);

                    framebuffer_update_msg buffer_update_msg;

                    // If the viewer is requesting a full update, force write
                    // of all pixels

                    framebuffer_rectangle this_rect;
                    this_rect.x = msg->x_position;
                    this_rect.y = msg->y_position;
                    this_rect.width = out.width;
                    this_rect.height = out.height;
                    this_rect.encoding =
                        static_cast<uint8_t>(encoding_type::raw);
                    std::cout << "Encoding is " << this_rect.encoding;
                    this_rect.data.reserve(
                        static_cast<std::size_t>(this_rect.width) *
                        static_cast<std::size_t>(this_rect.height) * 4);
                    std::cout << "Width " << out.width << " Height "
                              << out.height;

                    for (int i = 0; i < out.width * out.height; i++) {
                      auto& pixel = d.OutBuffer[i];
                      this_rect.data.push_back(pixel.B);
                      this_rect.data.push_back(pixel.G);
                      this_rect.data.push_back(pixel.R);
                      this_rect.data.push_back(0);
                    }

                    buffer_update_msg.rectangles.push_back(
                        std::move(this_rect));
                    auto serialized = serialize(buffer_update_msg);

                    conn.send_binary(serialized);

                  }  // TODO(Ed) handle error

                }

                break;

                case client_to_server_msg_type::key_event: {
                } break;

                case client_to_server_msg_type::pointer_event: {
                } break;

                case client_to_server_msg_type::client_cut_text: {
                } break;

                default:
                  break;
              }
            }

          } break;
          case VncState::UNSTARTED:
            // Error?  TODO
            break;
        }

      });
}
}  // namespace kvm
}  // namespace crow