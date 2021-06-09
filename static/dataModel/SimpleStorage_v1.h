#ifndef SIMPLESTORAGE_V1
#define SIMPLESTORAGE_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"
#include "SimpleStorage_v1.h"

struct SimpleStorage_v1_Actions
{
    SimpleStorage_v1_OemActions oem;
};
struct SimpleStorage_v1_Device
{
    Resource_v1_Resource oem;
    std::string name;
    Resource_v1_Resource status;
    std::string manufacturer;
    std::string model;
    int64_t capacityBytes;
};
struct SimpleStorage_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ chassis;
    NavigationReference__ storage;
};
struct SimpleStorage_v1_OemActions
{};
struct SimpleStorage_v1_SimpleStorage
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string uefiDevicePath;
    SimpleStorage_v1_Device devices;
    Resource_v1_Resource status;
    SimpleStorage_v1_Links links;
    SimpleStorage_v1_Actions actions;
};
#endif
