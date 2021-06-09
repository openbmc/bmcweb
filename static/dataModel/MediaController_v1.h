#ifndef MEDIACONTROLLER_V1
#define MEDIACONTROLLER_V1

#include "MediaController_v1.h"
#include "NavigationReference__.h"
#include "PortCollection_v1.h"
#include "Resource_v1.h"

enum class MediaController_v1_MediaControllerType {
    Memory,
};
struct MediaController_v1_Actions
{
    MediaController_v1_OemActions oem;
};
struct MediaController_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ endpoints;
    NavigationReference__ memoryDomains;
};
struct MediaController_v1_MediaController
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    MediaController_v1_Links links;
    std::string manufacturer;
    std::string model;
    std::string serialNumber;
    std::string partNumber;
    Resource_v1_Resource status;
    PortCollection_v1_PortCollection ports;
    MediaController_v1_MediaControllerType mediaControllerType;
    MediaController_v1_Actions actions;
    string UUID;
};
struct MediaController_v1_OemActions
{
};
#endif
