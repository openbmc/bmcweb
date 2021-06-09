#ifndef CERTIFICATESERVICE_V1
#define CERTIFICATESERVICE_V1

#include "CertificateLocations_v1.h"
#include "CertificateService_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"

struct CertificateService_v1_Actions
{
    CertificateService_v1_OemActions oem;
};
struct CertificateService_v1_CertificateService
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    CertificateService_v1_Actions actions;
    CertificateLocations_v1_CertificateLocations certificateLocations;
};
struct CertificateService_v1_GenerateCSRResponse
{
    NavigationReference__ certificateCollection;
    std::string cSRString;
};
struct CertificateService_v1_OemActions
{};
#endif
