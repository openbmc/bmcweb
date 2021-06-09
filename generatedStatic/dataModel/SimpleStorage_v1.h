#ifndef SIMPLESTORAGE_V1
#define SIMPLESTORAGE_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"
#include "SimpleStorage_v1.h"

struct SimpleStorageV1OemActions
{};
struct SimpleStorageV1Actions
{
    SimpleStorageV1OemActions oem;
};
struct SimpleStorageV1Device
{
    ResourceV1Resource oem;
    std::string name;
    ResourceV1Resource status;
    std::string manufacturer;
    std::string model;
    int64_t capacityBytes;
};
struct SimpleStorageV1Links
{
    ResourceV1Resource oem;
    NavigationReferenceRedfish chassis;
    NavigationReferenceRedfish storage;
};
struct SimpleStorageV1SimpleStorage
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string uefiDevicePath;
    SimpleStorageV1Device devices;
    ResourceV1Resource status;
    SimpleStorageV1Links links;
    SimpleStorageV1Actions actions;
};
#endif
