#ifndef FACILITY_V1
#define FACILITY_V1

#include "Facility_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"

enum class Facility_v1_FacilityType
{
    Room,
    Floor,
    Building,
    Site,
};
struct Facility_v1_Actions
{
    Facility_v1_OemActions oem;
};
struct Facility_v1_Facility
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Facility_v1_FacilityType facilityType;
    Resource_v1_Resource status;
    Resource_v1_Resource location;
    NavigationReference__ powerDomains;
    Facility_v1_Links links;
    Facility_v1_Actions actions;
};
struct Facility_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ containedByFacility;
    NavigationReference__ containsFacilities;
    NavigationReference__ managedBy;
    NavigationReference__ containsChassis;
    NavigationReference__ floorPDUs;
    NavigationReference__ rackPDUs;
    NavigationReference__ transferSwitches;
    NavigationReference__ switchgear;
};
struct Facility_v1_OemActions
{};
#endif
