#ifndef CERTIFICATELOCATIONS_V1
#define CERTIFICATELOCATIONS_V1

#include "CertificateLocations_v1.h"
#include "NavigationReference_.h"
#include "Resource_v1.h"

struct CertificateLocationsV1OemActions
{};
struct CertificateLocationsV1Actions
{
    CertificateLocationsV1OemActions oem;
};
struct CertificateLocationsV1Links
{
    ResourceV1Resource oem;
    NavigationReference_ certificates;
};
struct CertificateLocationsV1CertificateLocations
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    CertificateLocationsV1Links links;
    CertificateLocationsV1Actions actions;
};
#endif
