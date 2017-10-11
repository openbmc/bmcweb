#include <dbus/connection.hpp>
#include <dbus_monitor.hpp>
#include <dbus_singleton.hpp>
#include <intel_oem.hpp>
#include <openbmc_dbus_rest.hpp>
#include <persistent_data_middleware.hpp>
#include <redfish_v1.hpp>
#include <security_headers_middleware.hpp>
#include <ssl_key_handler.hpp>
#include <token_authorization_middleware.hpp>
#include <web_kvm.hpp>
#include <webassets.hpp>
#include <memory>
#include <string>
#include <crow/app.h>
#include <boost/asio.hpp>

int main(int argc, char** argv) {
  auto io = std::make_shared<boost::asio::io_service>();
  crow::App<crow::PersistentData::Middleware,
            crow::TokenAuthorization::Middleware,
            crow::SecurityHeadersMiddleware>
      app(io);

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
  crow::webassets::request_routes(app);
  crow::TokenAuthorization::request_routes(app);

  crow::kvm::request_routes(app);
  crow::redfish::request_routes(app);
  crow::dbus_monitor::request_routes(app);
  crow::intel_oem::request_routes(app);
  crow::openbmc_mapper::request_routes(app);

  crow::logger::setLogLevel(crow::LogLevel::INFO);
  int port = 18080;
  std::cout << "Starting webserver on port " << port << "\n";
  app.port(port);

  // Start dbus connection
  crow::connections::system_bus =
      std::make_shared<dbus::connection>(*io, dbus::bus::system);

  app.run();
}
