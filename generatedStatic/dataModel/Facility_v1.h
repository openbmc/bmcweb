#ifndef FACILITY_V1
#define FACILITY_V1

#include "Facility_v1.h"
#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

enum class FacilityV1FacilityType
{
    Room,
    Floor,
    Building,
    Site,
};
struct FacilityV1OemActions
{};
struct FacilityV1Actions
{
    FacilityV1OemActions oem;
};
struct FacilityV1Links
{
    ResourceV1Resource oem;
    NavigationReferenceRedfish containedByFacility;
    NavigationReferenceRedfish containsFacilities;
    NavigationReferenceRedfish managedBy;
    NavigationReferenceRedfish containsChassis;
    NavigationReferenceRedfish floorPDUs;
    NavigationReferenceRedfish rackPDUs;
    NavigationReferenceRedfish transferSwitches;
    NavigationReferenceRedfish switchgear;
};
struct FacilityV1Facility
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    FacilityV1FacilityType facilityType;
    ResourceV1Resource status;
    ResourceV1Resource location;
    NavigationReferenceRedfish powerDomains;
    FacilityV1Links links;
    FacilityV1Actions actions;
    NavigationReferenceRedfish environmentMetrics;
    NavigationReferenceRedfish ambientMetrics;
};
#endif
