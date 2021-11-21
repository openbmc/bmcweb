#include "redfish.hpp"

namespace redfish
{
// These were in account_service.hpp
using DbusVariantType = std::variant<bool, int32_t, std::string>;
using DbusInterfaceType = boost::container::flat_map<
    std::string, boost::container::flat_map<std::string, DbusVariantType>>;
using ManagedObjectType =
    std::vector<std::pair<sdbusplus::message::object_path, DbusInterfaceType>>;
using GetObjectType =
    std::vector<std::pair<std::string, std::vector<std::string>>>;
}

/* #include "../lib/certificate_service.hpp" */
namespace redfish
{
void requestRoutesCertificateService(App& app);
void requestRoutesCertificateActionGenerateCSR(App& app);
void requestRoutesCertificateActionsReplaceCertificate(App& app);
void requestRoutesHTTPSCertificate(App& app);
void requestRoutesHTTPSCertificateCollection(App& app);
void requestRoutesCertificateLocations(App& app);
void requestRoutesLDAPCertificateCollection(App& app);
void requestRoutesLDAPCertificate(App& app);
void requestRoutesTrustStoreCertificateCollection(App& app);
void requestRoutesTrustStoreCertificate(App& app);
}
/* #include "../lib/account_service.hpp" */
/* #include "../lib/bios.hpp" */
namespace redfish
{
void requestAccountServiceRoutes(App& app);
void requestRoutesBiosReset(App& app);
void requestRoutesBiosService(App& app);
}

namespace redfish
{
void requestRoutesChassisCollection(App& app);
void requestRoutesChassis(App& app);
void requestRoutesChassisResetAction(App& app);
void requestRoutesChassisResetActionInfo(App& app);
}

/* #include "../lib/ethernet.hpp" */
namespace redfish
{
void requestEthernetInterfacesRoutes(App& app);
}

/* #include "../lib/event_service.hpp" */
namespace redfish
{
void requestRoutesEventService(App& app);
void requestRoutesSubmitTestEvent(App& app);
void requestRoutesEventDestinationCollection(App& app);
void requestRoutesEventDestination(App& app);
}

/* #include "../lib/hypervisor_system.hpp" */
// moved to Ethernet
namespace redfish::hypervisor
{
void requestRoutesHypervisorSystems(App& app);
}

/* #include "../lib/log_services.hpp" */
namespace redfish {
void requestRoutesSystemLogServiceCollection(App& app);
void requestRoutesEventLogService(App& app);
void requestRoutesJournalEventLogClear(App& app);
void requestRoutesJournalEventLogEntryCollection(App& app);
void requestRoutesJournalEventLogEntry(App& app);
void requestRoutesDBusEventLogEntryCollection(App& app);
void requestRoutesDBusEventLogEntry(App& app);
void requestRoutesDBusEventLogEntryDownload(App& app);
void requestRoutesBMCLogServiceCollection(App& app);
void requestRoutesBMCJournalLogService(App& app);
void requestRoutesBMCJournalLogEntryCollection(App& app);
void requestRoutesBMCJournalLogEntry(App& app);
void requestRoutesBMCDumpService(App& app);
void requestRoutesBMCDumpEntryCollection(App& app);
void requestRoutesBMCDumpEntry(App& app);
void requestRoutesBMCDumpCreate(App& app);
void requestRoutesBMCDumpClear(App& app);
void requestRoutesSystemDumpService(App& app);
void requestRoutesSystemDumpEntryCollection(App& app);
void requestRoutesSystemDumpEntry(App& app);
void requestRoutesSystemDumpCreate(App& app);
void requestRoutesSystemDumpClear(App& app);
void requestRoutesCrashdumpService(App& app);
void requestRoutesCrashdumpClear(App& app);
void requestRoutesCrashdumpEntryCollection(App& app);
void requestRoutesCrashdumpEntry(App& app);
void requestRoutesCrashdumpFile(App& app);
void requestRoutesCrashdumpCollect(App& app);
void requestRoutesDBusLogServiceActionsClear(App& app);
void requestRoutesPostCodesLogService(App& app);
void requestRoutesPostCodesClear(App& app);
void requestRoutesPostCodesEntryCollection(App& app);
void requestRoutesPostCodesEntryAdditionalData(App& app);
void requestRoutesPostCodesEntry(App& app);
}

/* bios */
namespace redfish
{
void requestRoutesBiosService(App& app);
void requestRoutesBiosReset(App& app);
}

/* #include "../lib/managers.hpp" */
namespace redfish {
void requestRoutesManagerResetAction(App& app);
void requestRoutesManagerResetToDefaultsAction(App& app);
void requestRoutesManagerResetActionInfo(App& app);
void requestRoutesManager(App& app);
void requestRoutesManagerCollection(App& app);
}

/* #include "../lib/memory.hpp" */
namespace redfish
{
void requestRoutesMemoryCollection(App& app);
void requestRoutesMemory(App& app);
}

/* #include "../lib/message_registries.hpp" */
namespace redfish
{
void requestRoutesMessageRegistryFileCollection(App& app);
void requestRoutesMessageRegistryFile(App& app);
void requestRoutesMessageRegistry(App& app);
}

/* #include "../lib/metric_report.hpp" */
namespace redfish
{
void requestRoutesMetricReportCollection(App& app);
void requestRoutesMetricReport(App& app);
}

/* #include "../lib/metric_report_definition.hpp" */
namespace redfish
{
void requestRoutesMetricReportDefinitionCollection(App& app);
void requestRoutesMetricReportDefinition(App& app);
}

/* #include "../lib/network_protocol.hpp" */
// Moved to hypervisor_system.cpp
namespace redfish
{
void requestRoutesNetworkProtocol(App& app);
}

/* #include "../lib/pcie.hpp" */
namespace redfish { // moved to chassis.cpp
void requestRoutesSystemPCIeDeviceCollection(App& app);
void requestRoutesSystemPCIeDevice(App& app);
void requestRoutesSystemPCIeFunctionCollection(App& app);
void requestRoutesSystemPCIeFunction(App& app);
}

/* #include "../lib/power.hpp" */
// moved to sensors
namespace redfish
{
void requestRoutesPower(App& app);
}

/* #include "../lib/processor.hpp" */
// moved to Chassis
namespace redfish
{
void requestRoutesProcessor(App& app);
void requestRoutesProcessorCollection(App& app);
void requestRoutesOperatingConfig(App& app);
void requestRoutesOperatingConfigCollection(App& app);
}
/* #include "../lib/redfish_sessions.hpp" */
// moved to persistent_data.cpp
namespace redfish
{
void requestRoutesSession(App& app);
}

/* #include "../lib/roles.hpp" */
// Added to chassis
namespace redfish
{
void requestRoutesRoles(App& app);
void requestRoutesRoleCollection(App& app);
}
/* #include "../lib/sensors.hpp" */ 
namespace redfish
{
void requestRoutesSensorCollection(App& app);
void requestRoutesSensor(App& app);
}

/* #include "../lib/service_root.hpp" */
namespace redfish
{
void requestRoutesServiceRoot(App& app);
}

/* #include "../lib/storage.hpp" */
namespace redfish
{
void requestRoutesStorageCollection(App& app);
void requestRoutesStorage(App& app);
void requestRoutesDrive(App& app);
}

/* #include "../lib/systems.hpp" */
namespace redfish
{
void requestRoutesSystemActionsReset(App& app);
void requestRoutesSystems(App& app);
void requestRoutesSystemResetActionInfo(App& app);
void requestRoutesSystemsCollection(App& app);
}
/* #include "../lib/task.hpp" */
namespace redfish
{
void requestRoutesTaskMonitor(App& app);
void requestRoutesTask(App& app);
void requestRoutesTaskCollection(App& app);
void requestRoutesTaskService(App& app);
}

/* #include "../lib/telemetry_service.hpp" */
namespace redfish
{
void requestRoutesTelemetryService(App& app);
}

/* #include "../lib/thermal.hpp" */
namespace redfish
{
void requestRoutesThermal(App& app);

}

/* #include "../lib/update_service.hpp" */
// Moved to Task
namespace redfish
{
void requestRoutesUpdateServiceActionsSimpleUpdate(App& app);
void requestRoutesUpdateService(App& app);
void requestRoutesSoftwareInventoryCollection(App& app);
void requestRoutesSoftwareInventory(App& app);
}

/* #include "../lib/virtual_media.hpp" */
// Moved to account service
namespace redfish
{
void requestNBDVirtualMediaRoutes(App& app);
}


namespace redfish
{
RedfishService::RedfishService(App& app)
{
    requestAccountServiceRoutes(app);
    requestRoutesRoles(app);
    requestRoutesRoleCollection(app);
    requestRoutesServiceRoot(app);
    requestRoutesNetworkProtocol(app);
    requestRoutesSession(app);
    requestEthernetInterfacesRoutes(app);
#ifdef BMCWEB_ALLOW_DEPRECATED_POWER_THERMAL
    requestRoutesThermal(app);
    requestRoutesPower(app);
#endif
    requestRoutesManagerCollection(app);
    requestRoutesManager(app);
    requestRoutesManagerResetAction(app);
    requestRoutesManagerResetActionInfo(app);
    requestRoutesManagerResetToDefaultsAction(app);
    requestRoutesChassisCollection(app);
    requestRoutesChassis(app);
    requestRoutesChassisResetAction(app);
    requestRoutesChassisResetActionInfo(app);
    requestRoutesUpdateService(app);
    requestRoutesStorageCollection(app);
    requestRoutesStorage(app);
    requestRoutesDrive(app);
#ifdef BMCWEB_INSECURE_ENABLE_REDFISH_FW_TFTP_UPDATE
    requestRoutesUpdateServiceActionsSimpleUpdate(app);
#endif
    requestRoutesSoftwareInventoryCollection(app);
    requestRoutesSoftwareInventory(app);

    requestRoutesSystemLogServiceCollection(app);
    requestRoutesEventLogService(app);
    requestRoutesPostCodesEntryAdditionalData(app);

    requestRoutesPostCodesLogService(app);
    requestRoutesPostCodesClear(app);
    requestRoutesPostCodesEntry(app);
    requestRoutesPostCodesEntryCollection(app);

#ifdef BMCWEB_ENABLE_REDFISH_DUMP_LOG
    requestRoutesSystemDumpService(app);
    requestRoutesSystemDumpEntryCollection(app);
    requestRoutesSystemDumpEntry(app);
    requestRoutesSystemDumpCreate(app);
    requestRoutesSystemDumpClear(app);

    requestRoutesBMCDumpService(app);
    requestRoutesBMCDumpEntryCollection(app);
    requestRoutesBMCDumpEntry(app);
    requestRoutesBMCDumpCreate(app);
    requestRoutesBMCDumpClear(app);
#endif

#ifndef BMCWEB_ENABLE_REDFISH_DBUS_LOG_ENTRIES
    requestRoutesJournalEventLogEntryCollection(app);
    requestRoutesJournalEventLogEntry(app);
    requestRoutesJournalEventLogClear(app);
#endif

    requestRoutesBMCLogServiceCollection(app);
#ifdef BMCWEB_ENABLE_REDFISH_BMC_JOURNAL
    requestRoutesBMCJournalLogService(app);
    requestRoutesBMCJournalLogEntryCollection(app);
    requestRoutesBMCJournalLogEntry(app);
#endif

#ifdef BMCWEB_ENABLE_REDFISH_CPU_LOG
    requestRoutesCrashdumpService(app);
    requestRoutesCrashdumpEntryCollection(app);
    requestRoutesCrashdumpEntry(app);
    requestRoutesCrashdumpFile(app);
    requestRoutesCrashdumpClear(app);
    requestRoutesCrashdumpCollect(app);
#endif // BMCWEB_ENABLE_REDFISH_CPU_LOG

    requestRoutesProcessorCollection(app);
    requestRoutesProcessor(app);
    requestRoutesOperatingConfigCollection(app);
    requestRoutesOperatingConfig(app);
    requestRoutesMemoryCollection(app);
    requestRoutesMemory(app);

    requestRoutesSystemsCollection(app);
    requestRoutesSystems(app);
    requestRoutesSystemActionsReset(app);
    requestRoutesSystemResetActionInfo(app);
    requestRoutesBiosService(app);
    requestRoutesBiosReset(app);

#ifdef BMCWEB_ENABLE_VM_NBDPROXY
    requestNBDVirtualMediaRoutes(app);
#endif // BMCWEB_ENABLE_VM_NBDPROXY

#ifdef BMCWEB_ENABLE_REDFISH_DBUS_LOG_ENTRIES
    requestRoutesDBusLogServiceActionsClear(app);
    requestRoutesDBusEventLogEntryCollection(app);
    requestRoutesDBusEventLogEntry(app);
    requestRoutesDBusEventLogEntryDownload(app);
#endif

    requestRoutesMessageRegistryFileCollection(app);
    requestRoutesMessageRegistryFile(app);
    requestRoutesMessageRegistry(app);

    requestRoutesCertificateService(app);
    requestRoutesCertificateActionGenerateCSR(app);
    requestRoutesCertificateActionsReplaceCertificate(app);
    requestRoutesHTTPSCertificate(app);
    requestRoutesHTTPSCertificateCollection(app);
    requestRoutesCertificateLocations(app);
    requestRoutesLDAPCertificateCollection(app);
    requestRoutesLDAPCertificate(app);
    requestRoutesTrustStoreCertificateCollection(app);
    requestRoutesTrustStoreCertificate(app);

    requestRoutesSystemPCIeFunctionCollection(app);
    requestRoutesSystemPCIeFunction(app);
    requestRoutesSystemPCIeDeviceCollection(app);
    requestRoutesSystemPCIeDevice(app);

    requestRoutesSensorCollection(app);
    requestRoutesSensor(app);

    requestRoutesTaskMonitor(app);
    requestRoutesTaskService(app);
    requestRoutesTaskCollection(app);
    requestRoutesTask(app);
    requestRoutesEventService(app);
    requestRoutesEventDestinationCollection(app);
    requestRoutesEventDestination(app);
    requestRoutesSubmitTestEvent(app);

    hypervisor::requestRoutesHypervisorSystems(app);

    requestRoutesTelemetryService(app);
    requestRoutesMetricReportDefinitionCollection(app);
    requestRoutesMetricReportDefinition(app);
    requestRoutesMetricReportCollection(app);
    requestRoutesMetricReport(app);
}

}