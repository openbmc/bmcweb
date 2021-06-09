#ifndef MEDIACONTROLLER_V1
#define MEDIACONTROLLER_V1

#include "MediaController_v1.h"
#include "NavigationReferenceRedfish.h"
#include "PortCollection_v1.h"
#include "Resource_v1.h"

enum class MediaControllerV1MediaControllerType
{
    Memory,
};
struct MediaControllerV1OemActions
{};
struct MediaControllerV1Actions
{
    MediaControllerV1OemActions oem;
};
struct MediaControllerV1Links
{
    ResourceV1Resource oem;
    NavigationReferenceRedfish endpoints;
    NavigationReferenceRedfish memoryDomains;
};
struct MediaControllerV1MediaController
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    MediaControllerV1Links links;
    std::string manufacturer;
    std::string model;
    std::string serialNumber;
    std::string partNumber;
    ResourceV1Resource status;
    PortCollectionV1PortCollection ports;
    MediaControllerV1MediaControllerType mediaControllerType;
    MediaControllerV1Actions actions;
    std::string UUID;
    NavigationReferenceRedfish environmentMetrics;
};
#endif
