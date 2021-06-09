#ifndef OUTLETGROUP_V1
#define OUTLETGROUP_V1

#include "Circuit_v1.h"
#include "NavigationReference__.h"
#include "Outlet_v1.h"
#include "OutletGroup_v1.h"
#include "Resource_v1.h"

enum class OutletGroup_v1_PowerState {
    On,
    Off,
};
struct OutletGroup_v1_Actions
{
    OutletGroup_v1_OemActions oem;
};
struct OutletGroup_v1_Links
{
    Resource_v1_Resource oem;
    Outlet_v1_Outlet outlets;
};
struct OutletGroup_v1_OemActions
{
};
struct OutletGroup_v1_OutletGroup
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource status;
    std::string createdBy;
    double powerOnDelaySeconds;
    double powerOffDelaySeconds;
    double powerCycleDelaySeconds;
    double powerRestoreDelaySeconds;
    Circuit_v1_Circuit powerRestorePolicy;
    Resource_v1_Resource powerState;
    bool powerEnabled;
    NavigationReference__ powerWatts;
    NavigationReference__ energykWh;
    OutletGroup_v1_Links links;
    OutletGroup_v1_Actions actions;
};
#endif
