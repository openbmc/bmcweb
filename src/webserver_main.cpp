#include "crow/app.h"
#include "crow/ci_map.h"
#include "crow/common.h"
#include "crow/dumb_timer_queue.h"
#include "crow/http_connection.h"
#include "crow/http_parser_merged.h"
#include "crow/http_request.h"
#include "crow/http_response.h"
#include "crow/http_server.h"
#include "crow/json.h"
#include "crow/logging.h"
#include "crow/middleware.h"
#include "crow/middleware_context.h"
#include "crow/mustache.h"
#include "crow/parser.h"
#include "crow/query_string.h"
#include "crow/routing.h"
#include "crow/settings.h"
#include "crow/socket_adaptors.h"
#include "crow/utility.h"
#include "crow/websocket.h"

#include "app_type.hpp"

#include "color_cout_g3_sink.hpp"
#include "token_authorization_middleware.hpp"
#include "webassets.hpp"

#include <iostream>
#include <memory>
#include <string>
#include "ssl_key_handler.hpp"

#include <boost/endian/arithmetic.hpp>

#include <boost/asio.hpp>

#include <unordered_set>
#include <webassets.hpp>

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

struct server_initialization_message {
  boost::endian::big_uint16_t framebuffer_width;
  boost::endian::big_uint16_t framebuffer_height;
  pixel_format_struct pixel_format;
  boost::endian::big_uint32_t name_length;
};

enum class client_to_server_message_type : uint8_t {
  set_pixel_format = 0,
  fix_color_map_entries = 1,
  set_encodings = 2,
  framebuffer_update_request = 3,
  key_event = 4,
  pointer_event = 5,
  client_cut_text = 6
};

struct set_pixel_format_message {
  boost::endian::big_uint8_t pad1;
  boost::endian::big_uint8_t pad2;
  boost::endian::big_uint8_t pad3;
  pixel_format_struct pixel_format;
};

struct frame_buffer_update_request_message {
  boost::endian::big_uint8_t incremental;
  boost::endian::big_uint16_t x_position;
  boost::endian::big_uint16_t y_position;
  boost::endian::big_uint16_t width;
  boost::endian::big_uint16_t height;
};

struct key_event_message {
  boost::endian::big_uint8_t down_flag;
  boost::endian::big_uint8_t pad1;
  boost::endian::big_uint8_t pad2;
  boost::endian::big_uint32_t key;
};

struct pointer_event_message {
  boost::endian::big_uint8_t button_mask;
  boost::endian::big_uint16_t x_position;
  boost::endian::big_uint16_t y_position;
};

struct client_cut_text_message {
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
  boost::endian::big_uint16_t x;
  boost::endian::big_uint16_t y;
  boost::endian::big_uint16_t width;
  boost::endian::big_uint16_t height;
  boost::endian::big_uint32_t encoding;
  std::vector<uint8_t> data;
};

struct framebuffer_update_message {
  boost::endian::big_uint8_t message_type;

  std::vector<framebuffer_rectangle> rectangles;
};

std::string serialize(const framebuffer_update_message& msg) {
  // calculate the size of the needed vector for serialization
  size_t vector_size = 4;
  for (const auto& rect : msg.rectangles) {
    vector_size += 12 + rect.data.size();
  }

  std::string serialized(vector_size, 0);

  size_t i = 0;
  serialized[i++] = 0;  // Type
  serialized[i++] = 0;  // Pad byte
  boost::endian::big_uint16_t number_of_rectangles;
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
  AWAITING_CLIENT_INIT_MESSAGE,
  MAIN_LOOP
};

class connection_metadata {
 public:
  connection_metadata(void) : vnc_state(VncState::AWAITING_CLIENT_VERSION){};

  VncState vnc_state;
};

int main(int argc, char** argv) {
  auto worker(g3::LogWorker::createLogWorker());
  std::string logger_name("bmcweb");
  std::string folder("/tmp/");
  auto handle = worker->addDefaultLogger(logger_name, folder);
  g3::initializeLogging(worker.get());
  auto sink_handle = worker->addSink(std::make_unique<crow::ColorCoutSink>(),
                                     &crow::ColorCoutSink::ReceiveLogMessage);

  std::string ssl_pem_file("server.pem");
  ensuressl::ensure_openssl_key_present_and_valid(ssl_pem_file);

  BmcAppType app;
  crow::webassets::request_routes(app);

  crow::logger::setLogLevel(crow::LogLevel::INFO);

  CROW_ROUTE(app, "/routes")
  ([&app]() {
    crow::json::wvalue routes;

    routes["routes"] = app.get_rules();
    return routes;
  });

  CROW_ROUTE(app, "/login")
      .methods("POST"_method)([&](const crow::request& req) {
        auto auth_token =
            app.get_context<crow::TokenAuthorizationMiddleware>(req).auth_token;
        crow::json::wvalue x;
        x["token"] = auth_token;

        return x;
      });

  CROW_ROUTE(app, "/logout")
      .methods("GET"_method, "POST"_method)([]() {
        // Do nothing.  Credentials have already been cleared by middleware.
        return 200;
      });

  CROW_ROUTE(app, "/systeminfo")
  ([]() {

    crow::json::wvalue j;
    j["device_id"] = 0x7B;
    j["device_provides_sdrs"] = true;
    j["device_revision"] = true;
    j["device_available"] = true;
    j["firmware_revision"] = "0.68";

    j["ipmi_revision"] = "2.0";
    j["supports_chassis_device"] = true;
    j["supports_bridge"] = true;
    j["supports_ipmb_event_generator"] = true;
    j["supports_ipmb_event_receiver"] = true;
    j["supports_fru_inventory_device"] = true;
    j["supports_sel_device"] = true;
    j["supports_sdr_repository_device"] = true;
    j["supports_sensor_device"] = true;

    j["firmware_aux_revision"] = "0.60.foobar";

    return j;
  });

  typedef std::vector<connection_metadata> meta_list;
  meta_list connection_states(10);

  connection_metadata meta;

  CROW_ROUTE(app, "/kvmws")
      .websocket()
      .onopen([&](crow::websocket::connection& conn) {
        meta.vnc_state = VncState::AWAITING_CLIENT_VERSION;
        conn.send_binary(rfb_3_8_version_string);
      })
      .onclose(
          [&](crow::websocket::connection& conn, const std::string& reason) {
            meta.vnc_state = VncState::UNSTARTED;
          })
      .onmessage([&](crow::websocket::connection& conn, const std::string& data,
                     bool is_binary) {
        switch (meta.vnc_state) {
          case VncState::AWAITING_CLIENT_VERSION: {
            LOG(DEBUG) << "Client sent: " << data;
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
              meta.vnc_state = VncState::AWAITING_CLIENT_INIT_MESSAGE;
            } else {
              // Mark auth as failed
              security_result[3] = 1;
              meta.vnc_state = VncState::UNSTARTED;
            }
            conn.send_binary(security_result);
          } break;
          case VncState::AWAITING_CLIENT_INIT_MESSAGE: {
            // Now send the server initialization
            server_initialization_message server_init_msg;
            server_init_msg.framebuffer_width = 640;
            server_init_msg.framebuffer_height = 480;
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
            LOG(DEBUG) << "size: " << sizeof(server_init_msg);
            // TODO(ed) this is ugly.  Crow should really have a span type
            // interface
            // to avoid the copy, but alas, today it does not.
            std::string s(reinterpret_cast<char*>(&server_init_msg),
                          sizeof(server_init_msg));
            LOG(DEBUG) << "s.size() " << s.size();
            conn.send_binary(s);
            meta.vnc_state = VncState::MAIN_LOOP;
          } break;
          case VncState::MAIN_LOOP: {
            if (data.size() >= sizeof(client_to_server_message_type)) {
              auto type = static_cast<client_to_server_message_type>(data[0]);
              LOG(DEBUG) << "Got type " << (uint32_t)type << "\n";
              switch (type) {
                case client_to_server_message_type::set_pixel_format: {
                } break;

                case client_to_server_message_type::fix_color_map_entries: {
                } break;
                case client_to_server_message_type::set_encodings: {
                } break;
                case client_to_server_message_type::
                    framebuffer_update_request: {
                  // Make sure the buffer is long enough to handle what we're
                  // about to do
                  if (data.size() >=
                      sizeof(frame_buffer_update_request_message) +
                          sizeof(client_to_server_message_type)) {
                    auto msg = reinterpret_cast<
                        const frame_buffer_update_request_message*>(
                        data.data() + sizeof(client_to_server_message_type));

                    LOG(DEBUG) << "framebuffer_update_request_message\n";
                    LOG(DEBUG) << "    incremental=" << msg->incremental
                               << "\n";
                    LOG(DEBUG) << "    x=" << msg->x_position;
                    LOG(DEBUG) << " y=" << msg->y_position << "\n";
                    LOG(DEBUG) << "    width=" << msg->width;
                    LOG(DEBUG) << " height=" << msg->height << "\n";

                    framebuffer_update_message buffer_update_message;

                    // If the viewer is requesting a full update, force write of
                    // all
                    // pixels

                    framebuffer_rectangle this_rect;
                    this_rect.x = msg->x_position;
                    this_rect.y = msg->y_position;
                    this_rect.width = msg->width;
                    this_rect.height = msg->height;
                    this_rect.encoding =
                        static_cast<uint8_t>(encoding_type::raw);

                    this_rect.data.reserve(this_rect.width * this_rect.height *
                                           4);

                    for (unsigned int x_index = 0; x_index < this_rect.width;
                         x_index++) {
                      for (unsigned int y_index = 0; y_index < this_rect.height;
                           y_index++) {
                        this_rect.data.push_back(
                            static_cast<uint8_t>(0));  // Blue
                        this_rect.data.push_back(
                            static_cast<uint8_t>(0));  // Green
                        this_rect.data.push_back(static_cast<uint8_t>(
                            x_index * 0xFF / msg->width));  // RED
                        this_rect.data.push_back(
                            static_cast<uint8_t>(0));  // UNUSED
                      }
                    }

                    buffer_update_message.rectangles.push_back(
                        std::move(this_rect));
                    auto serialized = serialize(buffer_update_message);

                    conn.send_binary(serialized);
                  }

                }

                break;

                case client_to_server_message_type::key_event: {
                } break;

                case client_to_server_message_type::pointer_event: {
                } break;

                case client_to_server_message_type::client_cut_text: {
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

  CROW_ROUTE(app, "/ipmiws")
      .websocket()
      .onopen([&](crow::websocket::connection& conn) {

      })
      .onclose(
          [&](crow::websocket::connection& conn, const std::string& reason) {

          })
      .onmessage([&](crow::websocket::connection& conn, const std::string& data,
                     bool is_binary) {
        boost::asio::io_service io_service;
        using boost::asio::ip::udp;
        udp::resolver resolver(io_service);
        udp::resolver::query query(udp::v4(), "10.243.48.31", "623");
        udp::endpoint receiver_endpoint = *resolver.resolve(query);

        udp::socket socket(io_service);
        socket.open(udp::v4());

        socket.send_to(boost::asio::buffer(data), receiver_endpoint);

        std::array<char, 255> recv_buf;

        udp::endpoint sender_endpoint;
        size_t len =
            socket.receive_from(boost::asio::buffer(recv_buf), sender_endpoint);
        // TODO(ed) THis is ugly.  Find a way to not make a copy (ie, use
        // std::string::data())
        std::string str(std::begin(recv_buf), std::end(recv_buf));
        LOG(DEBUG) << "Got " << str << "back \n";
        conn.send_binary(str);

      });

  app.port(18080)
      //.ssl(std::move(ensuressl::get_ssl_context(ssl_pem_file)))
      .run();
  // app.port(18080).run();
}
