#include "redfish.hpp"

#include "bmcweb_config.h"

#include "account_service.hpp"
#include "aggregation_service.hpp"
#include "app.hpp"
#include "assembly.hpp"
#include "bios.hpp"
#include "cable.hpp"
#include "certificate_service.hpp"
#include "chassis.hpp"
#include "dump_offload.hpp"
#include "environment_metrics.hpp"
#include "ethernet.hpp"
#include "event_service.hpp"
#include "eventservice_sse.hpp"
#include "fabric_adapters.hpp"
#include "fabric_ports.hpp"
#include "fan.hpp"
#include "hypervisor_system.hpp"
#include "license_service.hpp"
#include "log_services.hpp"
#include "manager_diagnostic_data.hpp"
#include "managers.hpp"
#include "memory.hpp"
#include "message_registries.hpp"
#include "metadata.hpp"
#include "metric_report.hpp"
#include "metric_report_definition.hpp"
#include "network_protocol.hpp"
#include "odata.hpp"
#include "pcie.hpp"
#include "pcie_slots.hpp"
#include "power.hpp"
#include "power_subsystem.hpp"
#include "power_supply.hpp"
#include "processor.hpp"
#include "redfish_sessions.hpp"
#include "redfish_v1.hpp"
#include "roles.hpp"
#include "sensors.hpp"
#include "service_root.hpp"
#include "storage.hpp"
#include "systems.hpp"
#include "task.hpp"
#include "telemetry_service.hpp"
#include "thermal.hpp"
#include "thermal_metrics.hpp"
#include "thermal_subsystem.hpp"
#include "trigger.hpp"
#include "update_service.hpp"
#include "virtual_media.hpp"

namespace redfish
{

RedfishService::RedfishService(App& app)
{
    requestRoutesMetadata(app);
    requestRoutesOdata(app);

    requestAccountServiceRoutes(app);
    if constexpr (BMCWEB_REDFISH_AGGREGATION)
    {
        requestRoutesAggregationService(app);
        requestRoutesAggregationSourceCollection(app);
        requestRoutesAggregationSource(app);
    }
    requestRoutesRoles(app);
    requestRoutesRoleCollection(app);
    requestRoutesServiceRoot(app);
    requestRoutesNetworkProtocol(app);
    requestRoutesSession(app);
    requestEthernetInterfacesRoutes(app);
    if constexpr (BMCWEB_REDFISH_ALLOW_DEPRECATED_POWER_THERMAL)
    {
        requestRoutesThermal(app);
        requestRoutesPower(app);
    }
    if constexpr (BMCWEB_REDFISH_NEW_POWERSUBSYSTEM_THERMALSUBSYSTEM)
    {
        requestRoutesEnvironmentMetrics(app);
        requestRoutesPowerSubsystem(app);
        requestRoutesPowerSupply(app);
        requestRoutesPowerSupplyCollection(app);
        requestRoutesThermalMetrics(app);
        requestRoutesThermalSubsystem(app);
        requestRoutesFan(app);
        requestRoutesFanCollection(app);
    }
    requestRoutesManagerCollection(app);
    requestRoutesManager(app);
    requestRoutesManagerResetAction(app);
    requestRoutesManagerResetActionInfo(app);
    requestRoutesManagerResetToDefaultsAction(app);
    requestRoutesManagerDiagnosticData(app);
    requestRoutesChassisCollection(app);
    requestRoutesChassis(app);
    requestRoutesChassisResetAction(app);
    requestRoutesChassisResetActionInfo(app);
    requestRoutesChassisDrive(app);
    requestRoutesChassisDriveName(app);
    requestRoutesUpdateService(app);
    requestRoutesStorageCollection(app);
    requestRoutesStorage(app);
    requestRoutesStorageControllerCollection(app);
    requestRoutesStorageController(app);
    requestRoutesDrive(app);
    requestRoutesCable(app);
    requestRoutesCableCollection(app);
    requestRoutesAssembly(app);
    if constexpr (BMCWEB_INSECURE_TFTP_UPDATE)
    {
        requestRoutesUpdateServiceActionsSimpleUpdate(app);
    }
    requestRoutesUpdateServiceActionsOemConcurrentUpdate(app);
    requestRoutesSoftwareInventoryCollection(app);
    requestRoutesSoftwareInventory(app);

    requestRoutesSystemLogServiceCollection(app);
    requestRoutesEventLogService(app);
    requestRoutesPostCodesEntryAdditionalData(app);

    requestRoutesPostCodesLogService(app);
    requestRoutesPostCodesClear(app);
    requestRoutesPostCodesEntry(app);
    requestRoutesPostCodesEntryCollection(app);

    if constexpr (BMCWEB_REDFISH_DUMP_LOG)
    {
        requestRoutesSystemDumpService(app);
        requestRoutesSystemDumpEntryCollection(app);
        requestRoutesSystemDumpEntry(app);
        requestRoutesSystemDumpCreate(app);
        requestRoutesSystemDumpClear(app);

        requestRoutesBMCDumpService(app);
        requestRoutesBMCDumpEntryCollection(app);
        requestRoutesBMCDumpEntry(app);
        requestRoutesBMCDumpEntryDownload(app);
        requestRoutesBMCDumpCreate(app);
        requestRoutesBMCDumpClear(app);

        requestRoutesFaultLogDumpService(app);
        requestRoutesFaultLogDumpEntryCollection(app);
        requestRoutesFaultLogDumpEntry(app);
        requestRoutesFaultLogDumpClear(app);
    }

    requestRoutesBMCLogServiceCollection(app);
    if constexpr (BMCWEB_REDFISH_BMC_JOURNAL)
    {
        requestRoutesBMCJournalLogService(app);
        requestRoutesBMCJournalLogEntryCollection(app);
        requestRoutesBMCJournalLogEntry(app);
    }

    if constexpr (BMCWEB_AUDIT_EVENTS)
    {
        requestRoutesAuditLogService(app);
        requestRoutesAuditLogEntry(app);
        requestRoutesAuditLogEntryCollection(app);
        requestRoutesFullAuditLogDownload(app);
    }

    if constexpr (BMCWEB_REDFISH_CPU_LOG)
    {
        requestRoutesCrashdumpService(app);
        requestRoutesCrashdumpEntryCollection(app);
        requestRoutesCrashdumpEntry(app);
        requestRoutesCrashdumpFile(app);
        requestRoutesCrashdumpClear(app);
        requestRoutesCrashdumpCollect(app);
    }

    requestRoutesProcessorCollection(app);
    requestRoutesProcessor(app);
    requestRoutesOperatingConfigCollection(app);
    requestRoutesOperatingConfig(app);
    requestRoutesMemoryCollection(app);
    requestRoutesMemory(app);
    requestRoutesSubProcessors(app);
    requestRoutesSubProcessorsCore(app);

    requestRoutesSystems(app);
    requestRoutesSystemActionsOemExecutePanelFunction(app);
    requestRoutesBiosService(app);
    requestRoutesBiosSettings(app);
    requestRoutesBiosReset(app);

    if constexpr (BMCWEB_VM_NBDPROXY)
    {
        requestNBDVirtualMediaRoutes(app);
    }

    if constexpr (BMCWEB_REDFISH_DBUS_LOG)
    {
        requestRoutesCELogService(app);
        requestRoutesDBusLogServiceActionsClear(app);
        requestRoutesDBusCELogServiceActionsClear(app);
        requestRoutesDBusEventLogEntryCollection(app);
        requestRoutesDBusCELogEntryCollection(app);
        requestRoutesDBusEventLogEntry(app);
        requestRoutesDBusCELogEntry(app);
        requestRoutesDBusEventLogEntryDownload(app);
        requestRoutesDBusCEEventLogEntryDownload(app);
        requestRoutesDBusEventLogEntryDownloadPelJson(app);
        requestRoutesDBusCELogEntryDownloadPelJson(app);
    }
    if constexpr (BMCWEB_REDFISH_LICENSE)
    {
        requestRoutesLicenseService(app);
        requestRoutesLicenseEntryCollection(app);
        requestRoutesLicenseEntry(app);
    }
    else
    {
        requestRoutesJournalEventLogEntryCollection(app);
        requestRoutesJournalEventLogEntry(app);
        requestRoutesJournalEventLogClear(app);
    }

    if constexpr (BMCWEB_REDFISH_HOST_LOGGER)
    {
        requestRoutesSystemHostLogger(app);
        requestRoutesSystemHostLoggerCollection(app);
        requestRoutesSystemHostLoggerLogEntry(app);
    }

    if constexpr (BMCWEB_HW_ISOLATION)
    {
        requestRoutesSystemHardwareIsolationLogService(app);
    }
    requestRoutesMessageRegistryFileCollection(app);
    requestRoutesMessageRegistryFile(app);
    requestRoutesMessageRegistry(app);

    requestRoutesCertificateService(app);
    requestRoutesHTTPSCertificate(app);
    requestRoutesLDAPCertificate(app);
    requestRoutesTrustStoreCertificate(app);

    requestRoutesPCIeSlots(app);
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
    requestRoutesEventServiceSse(app);
    requestRoutesEventDestinationCollection(app);
    requestRoutesEventDestination(app);
    requestRoutesFabricAdapters(app);
    requestRoutesFabricAdapterCollection(app);
    requestRoutesFabricPort(app);
    requestRoutesSubmitTestEvent(app);

    requestRoutesHypervisorSystems(app);
    crow::obmc_dump::requestRoutes(app);

    requestRoutesTelemetryService(app);
    requestRoutesMetricReportDefinitionCollection(app);
    requestRoutesMetricReportDefinition(app);
    requestRoutesMetricReportCollection(app);
    requestRoutesMetricReport(app);
    requestRoutesTriggerCollection(app);
    requestRoutesTrigger(app);

    // Note, this must be the last route registered
    requestRoutesRedfish(app);
}

} // namespace redfish
