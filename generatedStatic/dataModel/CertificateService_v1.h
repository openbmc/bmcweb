#ifndef CERTIFICATESERVICE_V1
#define CERTIFICATESERVICE_V1

#include "CertificateLocations_v1.h"
#include "CertificateService_v1.h"
#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct CertificateServiceV1OemActions
{};
struct CertificateServiceV1Actions
{
    CertificateServiceV1OemActions oem;
};
struct CertificateServiceV1CertificateService
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    CertificateServiceV1Actions actions;
    CertificateLocationsV1CertificateLocations certificateLocations;
};
struct CertificateServiceV1GenerateCSRResponse
{
    NavigationReferenceRedfish certificateCollection;
    std::string cSRString;
};
#endif
