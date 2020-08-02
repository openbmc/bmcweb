#include <systemd/sd-daemon.h>

#include <app.hpp>
#include <boost/asio/io_context.hpp>
#include <cors_preflight.hpp>
#include <dbus_monitor.hpp>
#include <dbus_singleton.hpp>
#include <hostname_monitor.hpp>
#include <ibm/management_console_rest.hpp>
#include <image_upload.hpp>
#include <kvm_websocket.hpp>
#include <login_routes.hpp>
#include <obmc_console.hpp>
#include <openbmc_dbus_rest.hpp>
#include <redfish.hpp>
#include <redfish_v1.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include <security_headers.hpp>
#include <ssl_key_handler.hpp>
#include <vm_websocket.hpp>
#include <webassets.hpp>

#include <memory>
#include <string>

#ifdef BMCWEB_ENABLE_VM_NBDPROXY
#include <nbd_proxy.hpp>
#endif

inline void setupSocket(crow::App& app)
{
    char** names = nullptr;
    int listenFdCount = sd_listen_fds_with_names(0, &names);
    BMCWEB_LOG_DEBUG << "Got " << listenFdCount << " sockets to open";
    for (int socketIndex = 0; socketIndex < listenFdCount; socketIndex++)
    {
        // Assume HTTPS as default
        HttpType httpType = HttpType::HTTPS;
        if (names != nullptr)
        {
            if (names[socketIndex] != nullptr)
            {
                std::string socketName(names[socketIndex]);
                size_t nameStart = socketName.rfind('_');
                if (nameStart != std::string::npos)
                {
                    std::string_view name = socketName.substr(nameStart);
                    if (name == "_http")
                    {
                        BMCWEB_LOG_DEBUG << "Got http socket";
                        httpType = HttpType::HTTP;
                    }
                    else if (name == "_https")
                    {
                        BMCWEB_LOG_DEBUG << "Got https socket";
                        httpType = HttpType::HTTPS;
                    }
                    else if (name == "_both")
                    {
                        BMCWEB_LOG_DEBUG << "Got both socket";
                        httpType = HttpType::BOTH;
                    }
                    else
                    {
                        // all other types https
                        BMCWEB_LOG_ERROR << "Unknown socket type " << socketName
                                         << " assuming HTTPS only";
                    }
                }
            }
        }

        int listenFd = socketIndex + SD_LISTEN_FDS_START;
        if (sd_is_socket_inet(listenFd, AF_UNSPEC, SOCK_STREAM, 1, 0))
        {
            BMCWEB_LOG_INFO << "Starting webserver on socket handle "
                            << listenFd;
            app.addSocket(listenFd, httpType);
        }
    }
}

int main(int /*argc*/, char** /*argv*/)
{
    crow::Logger::setLogLevel(crow::LogLevel::Debug);

    auto io = std::make_shared<boost::asio::io_context>();
    App app(io);

    // Static assets need to be initialized before Authorization, because auth
    // needs to build the whitelist from the static routes

#ifdef BMCWEB_ENABLE_STATIC_HOSTING
    crow::webassets::requestRoutes(app);
#endif

#ifdef BMCWEB_ENABLE_KVM
    crow::obmc_kvm::requestRoutes(app);
#endif

#ifdef BMCWEB_ENABLE_REDFISH
    crow::redfish::requestRoutes(app);
    redfish::RedfishService redfish(app);

    // Create EventServiceManager instance and initialize Config
    redfish::EventServiceManager::getInstance();
#endif

#ifdef BMCWEB_ENABLE_DBUS_REST
    crow::dbus_monitor::requestRoutes(app);
    crow::image_upload::requestRoutes(app);
    crow::openbmc_mapper::requestRoutes(app);
#endif

#ifdef BMCWEB_ENABLE_HOST_SERIAL_WEBSOCKET
    crow::obmc_console::requestRoutes(app);
#endif

#ifdef BMCWEB_ENABLE_VM_WEBSOCKET
    crow::obmc_vm::requestRoutes(app);
#endif

#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
    crow::ibm_mc::requestRoutes(app);
    crow::ibm_mc_lock::Lock::getInstance();
#endif

    if (bmcwebInsecureDisableXssPrevention)
    {
        cors_preflight::requestRoutes(app);
    }

    crow::login_routes::requestRoutes(app);

    setupSocket(app);

    crow::connections::systemBus =
        std::make_shared<sdbusplus::asio::connection>(*io);

#ifdef BMCWEB_ENABLE_VM_NBDPROXY
    crow::nbd_proxy::requestRoutes(app);
#endif

#ifndef BMCWEB_ENABLE_REDFISH_DBUS_LOG_ENTRIES
    int rc = redfish::EventServiceManager::startEventLogMonitor(*io);
    if (rc)
    {
        BMCWEB_LOG_ERROR << "Redfish event handler setup failed...";
        return rc;
    }
#endif

#ifdef BMCWEB_ENABLE_SSL
    BMCWEB_LOG_INFO << "Start Hostname Monitor Service...";
    crow::hostname_monitor::registerHostnameSignal();
#endif

    app.run();
    io->run();

    crow::connections::systemBus.reset();
}
