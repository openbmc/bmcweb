#ifndef CERTIFICATELOCATIONS_V1
#define CERTIFICATELOCATIONS_V1

#include "CertificateLocations_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"

struct CertificateLocations_v1_Actions
{
    CertificateLocations_v1_OemActions oem;
};
struct CertificateLocations_v1_CertificateLocations
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    CertificateLocations_v1_Links links;
    CertificateLocations_v1_Actions actions;
};
struct CertificateLocations_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ certificates;
};
struct CertificateLocations_v1_OemActions
{};
#endif
