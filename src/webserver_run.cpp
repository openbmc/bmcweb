// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "webserver_run.hpp"

#include "bmcweb_config.h"

#include "app.hpp"
#include "dbus_monitor.hpp"
#include "dbus_singleton.hpp"
#include "event_service_manager.hpp"
#include "google/google_service_root.hpp"
#include "hostname_monitor.hpp"
#include "ibm/management_console_rest.hpp"
#include "image_upload.hpp"
#include "io_context_singleton.hpp"
#include "kvm_websocket.hpp"
#include "logging.hpp"
#include "login_routes.hpp"
#include "obmc_console.hpp"
#include "openbmc_dbus_rest.hpp"
#include "redfish.hpp"
#include "redfish_aggregator.hpp"
#include "user_monitor.hpp"
#include "vm_websocket.hpp"
#include "watchdog.hpp"
#include "webassets.hpp"

#include <boost/asio/io_context.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>

static void setLogLevel(const std::string& logLevel)
{
    const std::basic_string_view<char>* iter =
        std::ranges::find(crow::mapLogLevelFromName, logLevel);
    if (iter == crow::mapLogLevelFromName.end())
    {
        BMCWEB_LOG_ERROR("log-level {} not found", logLevel);
        return;
    }
    crow::getBmcwebCurrentLoggingLevel() = crow::getLogLevelFromName(logLevel);
    BMCWEB_LOG_INFO("Requested log-level change to: {}", logLevel);
}

int run()
{
    boost::asio::io_context& io = getIoContext();
    App app;

    std::shared_ptr<sdbusplus::asio::connection> systemBus =
        std::make_shared<sdbusplus::asio::connection>(io);
    crow::connections::systemBus = systemBus.get();

    auto server = sdbusplus::asio::object_server(systemBus);

    std::shared_ptr<sdbusplus::asio::dbus_interface> iface =
        server.add_interface("/xyz/openbmc_project/bmcweb",
                             "xyz.openbmc_project.bmcweb");

    iface->register_method("SetLogLevel", setLogLevel);

    iface->initialize();

    // Static assets need to be initialized before Authorization, because auth
    // needs to build the whitelist from the static routes

    if constexpr (BMCWEB_STATIC_HOSTING)
    {
        crow::webassets::requestRoutes(app);
    }

    if constexpr (BMCWEB_KVM)
    {
        crow::obmc_kvm::requestRoutes(app);
    }

    if constexpr (BMCWEB_REDFISH)
    {
        redfish::RedfishService redfish(app);

        // Create EventServiceManager instance and initialize Config
        redfish::EventServiceManager::getInstance();

        if constexpr (BMCWEB_REDFISH_AGGREGATION)
        {
            // Create RedfishAggregator instance and initialize Config
            redfish::RedfishAggregator::getInstance();
        }
    }

    if constexpr (BMCWEB_REST)
    {
        crow::dbus_monitor::requestRoutes(app);
        crow::image_upload::requestRoutes(app);
        crow::openbmc_mapper::requestRoutes(app);
    }

    if constexpr (BMCWEB_HOST_SERIAL_SOCKET)
    {
        crow::obmc_console::requestRoutes(app);
    }

    crow::obmc_vm::requestRoutes(app);

    if constexpr (BMCWEB_IBM_MANAGEMENT_CONSOLE)
    {
        crow::ibm_mc::requestRoutes(app);
    }

    if constexpr (BMCWEB_GOOGLE_API)
    {
        crow::google_api::requestRoutes(app);
    }

    crow::login_routes::requestRoutes(app);

    if constexpr (!BMCWEB_INSECURE_DISABLE_SSL)
    {
        BMCWEB_LOG_INFO("Start Hostname Monitor Service...");
        crow::hostname_monitor::registerHostnameSignal();
    }

    bmcweb::registerUserRemovedSignal();

    bmcweb::ServiceWatchdog watchdog;

    app.run();

    systemBus->request_name("xyz.openbmc_project.bmcweb");

    io.run();

    crow::connections::systemBus = nullptr;

    return 0;
}
