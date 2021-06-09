#ifndef FABRICADAPTER_V1
#define FABRICADAPTER_V1

#include "FabricAdapter_v1.h"
#include "NavigationReference__.h"
#include "PCIeDevice_v1.h"
#include "PortCollection_v1.h"
#include "Resource_v1.h"
#include "RouteEntryCollection_v1.h"
#include "VCATEntryCollection_v1.h"

struct FabricAdapter_v1_Actions
{
    FabricAdapter_v1_OemActions oem;
};
struct FabricAdapter_v1_FabricAdapter
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource status;
    PortCollection_v1_PortCollection ports;
    std::string manufacturer;
    std::string model;
    std::string SKU;
    std::string serialNumber;
    std::string partNumber;
    std::string sparePartNumber;
    std::string aSICRevisionIdentifier;
    std::string aSICPartNumber;
    std::string aSICManufacturer;
    std::string firmwareVersion;
    string UUID;
    PCIeDevice_v1_PCIeDevice pCIeInterface;
    FabricAdapter_v1_GenZ genZ;
    FabricAdapter_v1_Actions actions;
    FabricAdapter_v1_Links links;
};
struct FabricAdapter_v1_GenZ
{
    RouteEntryCollection_v1_RouteEntryCollection SSDT;
    RouteEntryCollection_v1_RouteEntryCollection MSDT;
    VCATEntryCollection_v1_VCATEntryCollection requestorVCAT;
    VCATEntryCollection_v1_VCATEntryCollection responderVCAT;
    std::string rITable;
    std::string PIDT;
};
struct FabricAdapter_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ endpoints;
};
struct FabricAdapter_v1_OemActions
{};
#endif
