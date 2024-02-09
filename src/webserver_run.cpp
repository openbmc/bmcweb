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
#include "kvm_websocket.hpp"
#include "logging.hpp"
#include "login_routes.hpp"
#include "obmc_console.hpp"
#include "obmc_shell.hpp"
#include "openbmc_dbus_rest.hpp"
#include "redfish.hpp"
#include "redfish_aggregator.hpp"
#include "user_monitor.hpp"
#include "vm_websocket.hpp"
#include "webassets.hpp"

#include <boost/asio/io_context.hpp>
#include <event_dbus_monitor.hpp>
#include <obmc_hypervisor.hpp>
#include <sdbusplus/asio/connection.hpp>

#include <memory>

int run()
{
    auto io = std::make_shared<boost::asio::io_context>();
    App app(io);

    sdbusplus::asio::connection systemBus(*io);
    crow::connections::systemBus = &systemBus;

    // Static assets need to be initialized before Authorization, because auth
    // needs to build the whitelist from the static routes

#ifdef BMCWEB_ENABLE_STATIC_HOSTING
    crow::webassets::requestRoutes(app);
#endif

#ifdef BMCWEB_ENABLE_KVM
    crow::obmc_kvm::requestRoutes(app);
#endif

#ifdef BMCWEB_ENABLE_REDFISH
    redfish::RedfishService redfish(app);

    // Create EventServiceManager instance and initialize Config
    redfish::EventServiceManager::getInstance(&*io);

#ifdef BMCWEB_ENABLE_REDFISH_AGGREGATION
    // Create RedfishAggregator instance and initialize Config
    redfish::RedfishAggregator::getInstance(&*io);
#endif
#endif

#ifdef BMCWEB_ENABLE_DBUS_REST
    crow::image_upload::requestRoutes(app);
    crow::openbmc_mapper::requestRoutes(app);
#endif

#ifdef BMCWEB_ENABLE_EVENT_SUBSCRIPTION_WEBSOCKET
    crow::dbus_monitor::requestRoutes(app);
#endif

#ifdef BMCWEB_ENABLE_HOST_SERIAL_WEBSOCKET
    crow::obmc_console::requestRoutes(app);
#endif
#ifdef BMCWEB_ENABLE_HYPERVISOR_SERIAL_WEBSOCKET
    crow::obmc_hypervisor::requestRoutes(app);
#endif

#ifdef BMCWEB_ENABLE_BMC_SHELL_WEBSOCKET
    crow::obmc_shell::requestRoutes(app);
#endif

#ifdef BMCWEB_ENABLE_VM_WEBSOCKET
    crow::obmc_vm::requestRoutes(app);
#endif

#ifdef BMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE
    crow::ibm_mc::requestRoutes(app);
    crow::ibm_mc_lock::Lock::getInstance();
    // Start BMC and Host state change dbus monitor
    crow::dbus_monitor::registerStateChangeSignal();
    // Start Dump created signal monitor for BMC and System Dump
    crow::dbus_monitor::registerDumpUpdateSignal();
    // Start BIOS Attr change dbus monitor
    crow::dbus_monitor::registerBIOSAttrUpdateSignal();
    // Start event log entry created monitor
    crow::dbus_monitor::registerEventLogCreatedSignal();
    // Start PostCode change signal
    crow::dbus_monitor::registerPostCodeChangeSignal();
    // Start hypervisor app dbus monitor for hypervisor
    // network configurations
    crow::dbus_monitor::registerVMIConfigChangeSignal();
    // Start Platform and Partition SAI state change monitor
    crow::dbus_monitor::registerSAIStateChangeSignal();
#endif

#ifdef BMCWEB_ENABLE_GOOGLE_API
    crow::google_api::requestRoutes(app);
#endif

    crow::login_routes::requestRoutes(app);

#ifdef BMCWEB_ENABLE_VM_NBDPROXY
    crow::nbd_proxy::requestRoutes(app);
#endif

#ifndef BMCWEB_ENABLE_REDFISH_DBUS_LOG_ENTRIES
    int rc = redfish::EventServiceManager::startEventLogMonitor(*io);
    if (rc != 0)
    {
        BMCWEB_LOG_ERROR("Redfish event handler setup failed...");
        return rc;
    }
#endif

    if constexpr (bmcwebEnableTLS)
    {
        BMCWEB_LOG_INFO("Start Hostname Monitor Service...");
        crow::hostname_monitor::registerHostnameSignal();
    }

    bmcweb::registerUserRemovedSignal();

    app.run();
    io->run();

    crow::connections::systemBus = nullptr;

    return 0;
}
