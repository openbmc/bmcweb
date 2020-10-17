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
#include "../lib/thermal.hpp"
#include "../lib/update_service.hpp"
#ifdef BMCWEB_ENABLE_VM_NBDPROXY
#include "../lib/virtual_media.hpp"
#endif // BMCWEB_ENABLE_VM_NBDPROXY
#include "../lib/hypervisor_ethernet.hpp"

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
        nodes.emplace_back(std::make_unique<Roles>(app));
        nodes.emplace_back(std::make_unique<RoleCollection>(app));
        nodes.emplace_back(std::make_unique<ServiceRoot>(app));
        nodes.emplace_back(std::make_unique<NetworkProtocol>(app));
        nodes.emplace_back(std::make_unique<SessionService>(app));
        nodes.emplace_back(std::make_unique<EthernetCollection>(app));
        nodes.emplace_back(std::make_unique<EthernetInterface>(app));
        nodes.emplace_back(std::make_unique<Thermal>(app));
        nodes.emplace_back(std::make_unique<ManagerCollection>(app));
        nodes.emplace_back(std::make_unique<Manager>(app));
        nodes.emplace_back(std::make_unique<ManagerResetAction>(app));
        nodes.emplace_back(std::make_unique<ManagerResetActionInfo>(app));
        nodes.emplace_back(std::make_unique<ManagerResetToDefaultsAction>(app));
        nodes.emplace_back(std::make_unique<Power>(app));
        nodes.emplace_back(std::make_unique<ChassisCollection>(app));
        nodes.emplace_back(std::make_unique<Chassis>(app));
        nodes.emplace_back(std::make_unique<ChassisResetAction>(app));
        nodes.emplace_back(std::make_unique<ChassisResetActionInfo>(app));
        nodes.emplace_back(std::make_unique<UpdateService>(app));
        nodes.emplace_back(std::make_unique<StorageCollection>(app));
        nodes.emplace_back(std::make_unique<Storage>(app));
        nodes.emplace_back(std::make_unique<Drive>(app));
#ifdef BMCWEB_INSECURE_ENABLE_REDFISH_FW_TFTP_UPDATE
        nodes.emplace_back(
            std::make_unique<UpdateServiceActionsSimpleUpdate>(app));
#endif
        nodes.emplace_back(std::make_unique<SoftwareInventoryCollection>(app));
        nodes.emplace_back(std::make_unique<SoftwareInventory>(app));
        nodes.emplace_back(
            std::make_unique<VlanNetworkInterfaceCollection>(app));
        nodes.emplace_back(std::make_unique<VlanNetworkInterface>(app));

        nodes.emplace_back(std::make_unique<SystemLogServiceCollection>(app));
        nodes.emplace_back(std::make_unique<EventLogService>(app));

        nodes.emplace_back(std::make_unique<PostCodesLogService>(app));
        nodes.emplace_back(std::make_unique<PostCodesClear>(app));
        nodes.emplace_back(std::make_unique<PostCodesEntry>(app));
        nodes.emplace_back(std::make_unique<PostCodesEntryCollection>(app));

#ifdef BMCWEB_ENABLE_REDFISH_DUMP_LOG
        nodes.emplace_back(std::make_unique<SystemDumpService>(app));
        nodes.emplace_back(std::make_unique<SystemDumpEntryCollection>(app));
        nodes.emplace_back(std::make_unique<SystemDumpEntry>(app));
        nodes.emplace_back(std::make_unique<SystemDumpCreate>(app));
        nodes.emplace_back(std::make_unique<SystemDumpClear>(app));

        nodes.emplace_back(std::make_unique<BMCDumpService>(app));
        nodes.emplace_back(std::make_unique<BMCDumpEntryCollection>(app));
        nodes.emplace_back(std::make_unique<BMCDumpEntry>(app));
        nodes.emplace_back(std::make_unique<BMCDumpCreate>(app));
        nodes.emplace_back(std::make_unique<BMCDumpClear>(app));
#endif

#ifndef BMCWEB_ENABLE_REDFISH_DBUS_LOG_ENTRIES
        nodes.emplace_back(
            std::make_unique<JournalEventLogEntryCollection>(app));
        nodes.emplace_back(std::make_unique<JournalEventLogEntry>(app));
        nodes.emplace_back(std::make_unique<JournalEventLogClear>(app));
#endif

        nodes.emplace_back(std::make_unique<BMCLogServiceCollection>(app));
#ifdef BMCWEB_ENABLE_REDFISH_BMC_JOURNAL
        nodes.emplace_back(std::make_unique<BMCJournalLogService>(app));
        nodes.emplace_back(std::make_unique<BMCJournalLogEntryCollection>(app));
        nodes.emplace_back(std::make_unique<BMCJournalLogEntry>(app));
#endif

#ifdef BMCWEB_ENABLE_REDFISH_CPU_LOG
        nodes.emplace_back(std::make_unique<CrashdumpService>(app));
        nodes.emplace_back(std::make_unique<CrashdumpEntryCollection>(app));
        nodes.emplace_back(std::make_unique<CrashdumpEntry>(app));
        nodes.emplace_back(std::make_unique<CrashdumpFile>(app));
        nodes.emplace_back(std::make_unique<CrashdumpClear>(app));
        nodes.emplace_back(std::make_unique<OnDemandCrashdump>(app));
        nodes.emplace_back(std::make_unique<TelemetryCrashdump>(app));
#ifdef BMCWEB_ENABLE_REDFISH_RAW_PECI
        nodes.emplace_back(std::make_unique<SendRawPECI>(app));
#endif // BMCWEB_ENABLE_REDFISH_RAW_PECI
#endif // BMCWEB_ENABLE_REDFISH_CPU_LOG

        nodes.emplace_back(std::make_unique<ProcessorCollection>(app));
        nodes.emplace_back(std::make_unique<Processor>(app));
        nodes.emplace_back(std::make_unique<OperatingConfigCollection>(app));
        nodes.emplace_back(std::make_unique<OperatingConfig>(app));
        nodes.emplace_back(std::make_unique<MemoryCollection>(app));
        nodes.emplace_back(std::make_unique<Memory>(app));

        nodes.emplace_back(std::make_unique<SystemsCollection>(app));
        nodes.emplace_back(std::make_unique<Systems>(app));
        nodes.emplace_back(std::make_unique<SystemActionsReset>(app));
        nodes.emplace_back(std::make_unique<SystemResetActionInfo>(app));
        nodes.emplace_back(std::make_unique<BiosService>(app));
        nodes.emplace_back(std::make_unique<BiosReset>(app));
#ifdef BMCWEB_ENABLE_VM_NBDPROXY
        nodes.emplace_back(std::make_unique<VirtualMedia>(app));
        nodes.emplace_back(std::make_unique<VirtualMediaCollection>(app));
        nodes.emplace_back(
            std::make_unique<VirtualMediaActionInsertMedia>(app));
        nodes.emplace_back(std::make_unique<VirtualMediaActionEjectMedia>(app));
#endif // BMCWEB_ENABLE_VM_NBDPROXY
#ifdef BMCWEB_ENABLE_REDFISH_DBUS_LOG_ENTRIES
        nodes.emplace_back(std::make_unique<DBusLogServiceActionsClear>(app));
        nodes.emplace_back(std::make_unique<DBusEventLogEntryCollection>(app));
        nodes.emplace_back(std::make_unique<DBusEventLogEntry>(app));
#endif

        nodes.emplace_back(
            std::make_unique<MessageRegistryFileCollection>(app));
        nodes.emplace_back(std::make_unique<MessageRegistryFile>(app));
        nodes.emplace_back(std::make_unique<MessageRegistry>(app));
        nodes.emplace_back(std::make_unique<CertificateService>(app));
        nodes.emplace_back(
            std::make_unique<CertificateActionsReplaceCertificate>(app));
        nodes.emplace_back(std::make_unique<CertificateLocations>(app));
        nodes.emplace_back(std::make_unique<HTTPSCertificateCollection>(app));
        nodes.emplace_back(std::make_unique<HTTPSCertificate>(app));
        nodes.emplace_back(std::make_unique<LDAPCertificateCollection>(app));
        nodes.emplace_back(std::make_unique<LDAPCertificate>(app));
        nodes.emplace_back(std::make_unique<CertificateActionGenerateCSR>(app));
        nodes.emplace_back(
            std::make_unique<TrustStoreCertificateCollection>(app));
        nodes.emplace_back(std::make_unique<TrustStoreCertificate>(app));
        nodes.emplace_back(std::make_unique<SystemPCIeFunctionCollection>(app));
        nodes.emplace_back(std::make_unique<SystemPCIeFunction>(app));
        nodes.emplace_back(std::make_unique<SystemPCIeDeviceCollection>(app));
        nodes.emplace_back(std::make_unique<SystemPCIeDevice>(app));

        nodes.emplace_back(std::make_unique<SensorCollection>(app));
        nodes.emplace_back(std::make_unique<Sensor>(app));

        nodes.emplace_back(std::make_unique<TaskMonitor>(app));
        nodes.emplace_back(std::make_unique<TaskService>(app));
        nodes.emplace_back(std::make_unique<TaskCollection>(app));
        nodes.emplace_back(std::make_unique<Task>(app));
        nodes.emplace_back(std::make_unique<EventService>(app));
        nodes.emplace_back(std::make_unique<EventDestinationCollection>(app));
        nodes.emplace_back(std::make_unique<EventDestination>(app));
        nodes.emplace_back(std::make_unique<SubmitTestEvent>(app));

        nodes.emplace_back(
            std::make_unique<HypervisorInterfaceCollection>(app));
        nodes.emplace_back(std::make_unique<HypervisorInterface>(app));
        nodes.emplace_back(std::make_unique<HypervisorSystem>(app));

        for (const auto& node : nodes)
        {
            node->initPrivileges();
        }
    }

  private:
    std::vector<std::unique_ptr<Node>> nodes;
};

} // namespace redfish
