#ifndef FACILITY_V1
#define FACILITY_V1

#include "Facility_v1.h"
#include "NavigationReference_.h"
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
    NavigationReference_ containedByFacility;
    NavigationReference_ containsFacilities;
    NavigationReference_ managedBy;
    NavigationReference_ containsChassis;
    NavigationReference_ floorPDUs;
    NavigationReference_ rackPDUs;
    NavigationReference_ transferSwitches;
    NavigationReference_ switchgear;
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
    NavigationReference_ powerDomains;
    FacilityV1Links links;
    FacilityV1Actions actions;
    NavigationReference_ environmentMetrics;
    NavigationReference_ ambientMetrics;
};
#endif
