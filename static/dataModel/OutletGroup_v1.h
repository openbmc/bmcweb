#ifndef OUTLETGROUP_V1
#define OUTLETGROUP_V1

#include "Circuit_v1.h"
#include "NavigationReference_.h"
#include "OutletGroup_v1.h"
#include "Outlet_v1.h"
#include "Resource_v1.h"

enum class OutletGroupV1PowerState
{
    On,
    Off,
};
struct OutletGroupV1OemActions
{};
struct OutletGroupV1Actions
{
    OutletGroupV1OemActions oem;
};
struct OutletGroupV1Links
{
    ResourceV1Resource oem;
    OutletV1Outlet outlets;
};
struct OutletGroupV1OutletGroup
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource status;
    std::string createdBy;
    double powerOnDelaySeconds;
    double powerOffDelaySeconds;
    double powerCycleDelaySeconds;
    double powerRestoreDelaySeconds;
    CircuitV1Circuit powerRestorePolicy;
    ResourceV1Resource powerState;
    bool powerEnabled;
    NavigationReference_ powerWatts;
    NavigationReference_ energykWh;
    OutletGroupV1Links links;
    OutletGroupV1Actions actions;
};
#endif
