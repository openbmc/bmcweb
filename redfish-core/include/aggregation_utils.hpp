#pragma once
/****************************************************************
 *                 READ THIS WARNING FIRST
 * This is an auto-generated header which contains definitions
 * for Redfish DMTF defined schemas.
 * DO NOT modify this registry outside of running the
 * update_schemas.py script.  The definitions contained within
 * this file are owned by DMTF.  Any modifications to these files
 * should be first pushed to the relevant registry in the DMTF
 * github organization.
 ***************************************************************/
// clang-format off
#include <array>
#include <unordered_map>
#include <unordered_set>

namespace redfish
{
constexpr std::array topCollections{
    "/redfish/v1/AggregationService/Aggregates",
    "/redfish/v1/AggregationService/AggregationSources",
    "/redfish/v1/AggregationService/ConnectionMethods",
    "/redfish/v1/Cables",
    "/redfish/v1/CertificateService/CertificateCollection",
    "/redfish/v1/CertificateService/CertificateLocations/Certificates",
    "/redfish/v1/Chassis",
    "/redfish/v1/ComponentIntegrity",
    "/redfish/v1/CompositionService/ActivePool",
    "/redfish/v1/CompositionService/CompositionReservations",
    "/redfish/v1/CompositionService/FreePool",
    "/redfish/v1/CompositionService/ResourceBlocks",
    "/redfish/v1/CompositionService/ResourceZones",
    "/redfish/v1/EventService/Subscriptions",
    "/redfish/v1/Fabrics",
    "/redfish/v1/Facilities",
    "/redfish/v1/JobService/Jobs",
    "/redfish/v1/JobService/LogService/Entries",
    "/redfish/v1/JsonSchemas",
    "/redfish/v1/KeyService/NVMeoFKeyPolicies",
    "/redfish/v1/KeyService/NVMeoFSecrets",
    "/redfish/v1/LicenseService/Licenses",
    "/redfish/v1/Managers",
    "/redfish/v1/NVMeDomains",
    "/redfish/v1/PowerEquipment/ElectricalBuses",
    "/redfish/v1/PowerEquipment/FloorPDUs",
    "/redfish/v1/PowerEquipment/ManagedBy",
    "/redfish/v1/PowerEquipment/PowerShelves",
    "/redfish/v1/PowerEquipment/RackPDUs",
    "/redfish/v1/PowerEquipment/Switchgear",
    "/redfish/v1/PowerEquipment/TransferSwitches",
    "/redfish/v1/RegisteredClients",
    "/redfish/v1/Registries",
    "/redfish/v1/ResourceBlocks",
    "/redfish/v1/SessionService/Sessions",
    "/redfish/v1/Sessions",
    "/redfish/v1/Storage",
    "/redfish/v1/StorageServices",
    "/redfish/v1/StorageSystems",
    "/redfish/v1/Systems",
    "/redfish/v1/TaskService/Tasks",
    "/redfish/v1/TelemetryService/LogService/Entries",
    "/redfish/v1/TelemetryService/MetricDefinitions",
    "/redfish/v1/TelemetryService/MetricReportDefinitions",
    "/redfish/v1/TelemetryService/MetricReports",
    "/redfish/v1/TelemetryService/Triggers",
    "/redfish/v1/UpdateService/ClientCertificates",
    "/redfish/v1/UpdateService/FirmwareInventory",
    "/redfish/v1/UpdateService/RemoteServerCertificates",
    "/redfish/v1/UpdateService/SoftwareInventory",
};

const std::unordered_map<std::string, std::unordered_set<std::string>>
    topCollectionsParents = {
        {"/redfish/v1",
         {
             "/redfish/v1/AggregationService",
             "/redfish/v1/Cables",
             "/redfish/v1/CertificateService",
             "/redfish/v1/Chassis",
             "/redfish/v1/ComponentIntegrity",
             "/redfish/v1/CompositionService",
             "/redfish/v1/EventService",
             "/redfish/v1/Fabrics",
             "/redfish/v1/Facilities",
             "/redfish/v1/JobService",
             "/redfish/v1/JsonSchemas",
             "/redfish/v1/KeyService",
             "/redfish/v1/LicenseService",
             "/redfish/v1/Managers",
             "/redfish/v1/NVMeDomains",
             "/redfish/v1/PowerEquipment",
             "/redfish/v1/RegisteredClients",
             "/redfish/v1/Registries",
             "/redfish/v1/ResourceBlocks",
             "/redfish/v1/SessionService",
             "/redfish/v1/Sessions",
             "/redfish/v1/Storage",
             "/redfish/v1/StorageServices",
             "/redfish/v1/StorageSystems",
             "/redfish/v1/Systems",
             "/redfish/v1/TaskService",
             "/redfish/v1/TelemetryService",
             "/redfish/v1/UpdateService",
         }},
        {"/redfish/v1/AggregationService",
         {
             "/redfish/v1/AggregationService/Aggregates",
             "/redfish/v1/AggregationService/AggregationSources",
             "/redfish/v1/AggregationService/ConnectionMethods",
         }},
        {"/redfish/v1/CertificateService",
         {
             "/redfish/v1/CertificateService/CertificateCollection",
             "/redfish/v1/CertificateService/CertificateLocations",
         }},
        {"/redfish/v1/CertificateService/CertificateLocations",
         {
             "/redfish/v1/CertificateService/CertificateLocations/Certificates",
         }},
        {"/redfish/v1/CompositionService",
         {
             "/redfish/v1/CompositionService/ActivePool",
             "/redfish/v1/CompositionService/CompositionReservations",
             "/redfish/v1/CompositionService/FreePool",
             "/redfish/v1/CompositionService/ResourceBlocks",
             "/redfish/v1/CompositionService/ResourceZones",
         }},
        {"/redfish/v1/EventService",
         {
             "/redfish/v1/EventService/Subscriptions",
         }},
        {"/redfish/v1/JobService",
         {
             "/redfish/v1/JobService/Jobs",
             "/redfish/v1/JobService/LogService",
         }},
        {"/redfish/v1/JobService/LogService",
         {
             "/redfish/v1/JobService/LogService/Entries",
         }},
        {"/redfish/v1/KeyService",
         {
             "/redfish/v1/KeyService/NVMeoFKeyPolicies",
             "/redfish/v1/KeyService/NVMeoFSecrets",
         }},
        {"/redfish/v1/LicenseService",
         {
             "/redfish/v1/LicenseService/Licenses",
         }},
        {"/redfish/v1/PowerEquipment",
         {
             "/redfish/v1/PowerEquipment/ElectricalBuses",
             "/redfish/v1/PowerEquipment/FloorPDUs",
             "/redfish/v1/PowerEquipment/ManagedBy",
             "/redfish/v1/PowerEquipment/PowerShelves",
             "/redfish/v1/PowerEquipment/RackPDUs",
             "/redfish/v1/PowerEquipment/Switchgear",
             "/redfish/v1/PowerEquipment/TransferSwitches",
         }},
        {"/redfish/v1/SessionService",
         {
             "/redfish/v1/SessionService/Sessions",
         }},
        {"/redfish/v1/TaskService",
         {
             "/redfish/v1/TaskService/Tasks",
         }},
        {"/redfish/v1/TelemetryService",
         {
             "/redfish/v1/TelemetryService/LogService",
             "/redfish/v1/TelemetryService/MetricDefinitions",
             "/redfish/v1/TelemetryService/MetricReportDefinitions",
             "/redfish/v1/TelemetryService/MetricReports",
             "/redfish/v1/TelemetryService/Triggers",
         }},
        {"/redfish/v1/TelemetryService/LogService",
         {
             "/redfish/v1/TelemetryService/LogService/Entries",
         }},
        {"/redfish/v1/UpdateService",
         {
             "/redfish/v1/UpdateService/ClientCertificates",
             "/redfish/v1/UpdateService/FirmwareInventory",
             "/redfish/v1/UpdateService/RemoteServerCertificates",
             "/redfish/v1/UpdateService/SoftwareInventory",
         }},
};
} // namespace redfish
