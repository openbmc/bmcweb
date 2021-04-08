/*
// Copyright (c) 2018-2019 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#pragma once

#include "../lib/account_service.hpp"
#include "../lib/bios.hpp"
#include "../lib/certificate_service.hpp"
#include "../lib/chassis.hpp"
#include "../lib/ethernet.hpp"
#include "../lib/event_service.hpp"
#include "../lib/log_services.hpp"
#include "../lib/managers.hpp"
#include "../lib/memory.hpp"
#include "../lib/message_registries.hpp"
#include "../lib/metric_report.hpp"
#include "../lib/metric_report_definition.hpp"
#include "../lib/network_protocol.hpp"
#include "../lib/pcie.hpp"
#include "../lib/power.hpp"
#include "../lib/processor.hpp"
#include "../lib/redfish_sessions.hpp"
#include "../lib/roles.hpp"
#include "../lib/sensors.hpp"
#include "../lib/service_root.hpp"
#include "../lib/storage.hpp"
#include "../lib/systems.hpp"
#include "../lib/task.hpp"
#include "../lib/telemetry_service.hpp"
#include "../lib/thermal.hpp"
#include "../lib/update_service.hpp"
#ifdef BMCWEB_ENABLE_VM_NBDPROXY
#include "../lib/virtual_media.hpp"
#endif // BMCWEB_ENABLE_VM_NBDPROXY
#include "../lib/hypervisor_system.hpp"

namespace redfish
{
/*
 * @brief Top level class installing and providing Redfish services
 */
class RedfishService
{
  public:
    /*
     * @brief Redfish service constructor
     *
     * Loads Redfish configuration and installs schema resources
     *
     * @param[in] app   Crow app on which Redfish will initialize
     */
    RedfishService(App& app)
    {
        nodes.emplace_back(std::make_unique<AccountService>(app));
        nodes.emplace_back(std::make_unique<AccountsCollection>(app));
        nodes.emplace_back(std::make_unique<ManagerAccount>(app));
        nodes.emplace_back(std::make_unique<SessionCollection>(app));
        requestRoutesRoles(app);
        requestRoutesRoleCollection(app);
        requestRoutesServiceRoot(app);
        requestRoutesNetworkProtocol(app);
        nodes.emplace_back(std::make_unique<SessionService>(app));
        nodes.emplace_back(std::make_unique<EthernetCollection>(app));
        nodes.emplace_back(std::make_unique<EthernetInterface>(app));
        requestRoutesThermal(app);
        requestRoutesManagerCollection(app);
        requestRoutesManager(app);
        requestRoutesManagerResetAction(app);
        requestRoutesManagerResetActionInfo(app);
        requestRoutesManagerResetToDefaultsAction(app);
        requestRoutesPower(app);
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
        nodes.emplace_back(
            std::make_unique<VlanNetworkInterfaceCollection>(app));
        nodes.emplace_back(std::make_unique<VlanNetworkInterface>(app));

        requestRoutesSystemLogServiceCollection(app);
        requestRoutesEventLogService(app);

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
        nodes.emplace_back(std::make_unique<VirtualMedia>(app));
        nodes.emplace_back(std::make_unique<VirtualMediaCollection>(app));
        nodes.emplace_back(
            std::make_unique<VirtualMediaActionInsertMedia>(app));
        nodes.emplace_back(std::make_unique<VirtualMediaActionEjectMedia>(app));
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

        nodes.emplace_back(
            std::make_unique<HypervisorInterfaceCollection>(app));
        nodes.emplace_back(std::make_unique<HypervisorInterface>(app));
        nodes.emplace_back(std::make_unique<HypervisorSystem>(app));
        nodes.emplace_back(std::make_unique<HypervisorActionsReset>(app));
        nodes.emplace_back(std::make_unique<HypervisorResetActionInfo>(app));

        requestRoutesTelemetryService(app);
        requestRoutesMetricReportDefinitionCollection(app);
        requestRoutesMetricReportDefinition(app);
        requestRoutesMetricReportCollection(app);
        requestRoutesMetricReport(app);

        for (const auto& node : nodes)
        {
            node->initPrivileges();
        }
    }

  private:
    std::vector<std::unique_ptr<Node>> nodes;
};

} // namespace redfish
