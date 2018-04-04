#include <systemd/sd-daemon.h>
#include <bmcweb/settings.hpp>
#include <dbus_monitor.hpp>
#include <dbus_singleton.hpp>
#include <image_upload.hpp>
#include <openbmc_dbus_rest.hpp>
#include <persistent_data_middleware.hpp>
#include <redfish.hpp>
#include <redfish_v1.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include <security_headers_middleware.hpp>
#include <ssl_key_handler.hpp>
#include <token_authorization_middleware.hpp>
#include <web_kvm.hpp>
#include <webassets.hpp>
#include <webserver_common.hpp>
#include <memory>
#include <string>
#include <crow/app.h>
#include <boost/asio.hpp>

constexpr int defaultPort = 18080;

template <typename... Middlewares>
void setup_socket(crow::Crow<Middlewares...>& app) {
  int listen_fd = sd_listen_fds(0);
  if (1 == listen_fd) {
    CROW_LOG_INFO << "attempting systemd socket activation";
    if (sd_is_socket_inet(SD_LISTEN_FDS_START, AF_UNSPEC, SOCK_STREAM, 1, 0)) {
      CROW_LOG_INFO << "Starting webserver on socket handle "
                    << SD_LISTEN_FDS_START;
      app.socket(SD_LISTEN_FDS_START);
    } else {
      CROW_LOG_INFO << "bad incoming socket, starting webserver on port "
                    << defaultPort;
      app.port(defaultPort);
    }
  } else {
    CROW_LOG_INFO << "Starting webserver on port " << defaultPort;
    app.port(defaultPort);
  }
}

int main(int argc, char** argv) {
  crow::logger::setLogLevel(crow::LogLevel::DEBUG);

  auto io = std::make_shared<boost::asio::io_service>();
  CrowApp app(io);

#ifdef CROW_ENABLE_SSL
  std::string ssl_pem_file("server.pem");
  std::cout << "Building SSL context\n";

  ensuressl::ensure_openssl_key_present_and_valid(ssl_pem_file);
  std::cout << "SSL Enabled\n";
  auto ssl_context = ensuressl::get_ssl_context(ssl_pem_file);
  app.ssl(std::move(ssl_context));
#endif
  // Static assets need to be initialized before Authorization, because auth
  // needs to build the whitelist from the static routes

#ifdef BMCWEB_ENABLE_PHOSPHOR_WEBUI
  crow::webassets::request_routes(app);
#endif

#ifdef BMCWEB_ENABLE_KVM
  crow::kvm::request_routes(app);
#endif

#ifdef BMCWEB_ENABLE_REDFISH
  crow::redfish::request_routes(app);
#endif

#ifdef BMCWEB_ENABLE_DBUS_REST
  crow::dbus_monitor::request_routes(app);
  crow::image_upload::requestRoutes(app);
  crow::openbmc_mapper::request_routes(app);
#endif

  crow::TokenAuthorization::request_routes(app);

  CROW_LOG_INFO << "bmcweb (" << __DATE__ << ": " << __TIME__ << ')';
  setup_socket(app);

  crow::connections::system_bus =
      std::make_shared<sdbusplus::asio::connection>(*io);
  redfish::RedfishService redfish(app);

  app.run();
  io->run();

  crow::connections::system_bus.reset();
}
