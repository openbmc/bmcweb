#ifndef FABRICADAPTER_V1
#define FABRICADAPTER_V1

#include "FabricAdapter_v1.h"
#include "NavigationReference_.h"
#include "PCIeDevice_v1.h"
#include "PortCollection_v1.h"
#include "Resource_v1.h"
#include "RouteEntryCollection_v1.h"
#include "VCATEntryCollection_v1.h"

struct FabricAdapterV1OemActions
{};
struct FabricAdapterV1Actions
{
    FabricAdapterV1OemActions oem;
};
struct FabricAdapterV1GenZ
{
    RouteEntryCollectionV1RouteEntryCollection SSDT;
    RouteEntryCollectionV1RouteEntryCollection MSDT;
    VCATEntryCollectionV1VCATEntryCollection requestorVCAT;
    VCATEntryCollectionV1VCATEntryCollection responderVCAT;
    std::string rITable;
    std::string PIDT;
};
struct FabricAdapterV1Links
{
    ResourceV1Resource oem;
    NavigationReference_ endpoints;
};
struct FabricAdapterV1FabricAdapter
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource status;
    PortCollectionV1PortCollection ports;
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
    std::string UUID;
    PCIeDeviceV1PCIeDevice pCIeInterface;
    FabricAdapterV1GenZ genZ;
    FabricAdapterV1Actions actions;
    FabricAdapterV1Links links;
};
#endif
