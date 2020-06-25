#pragma once
#include <registries.hpp>

namespace redfish::message_registries::resource_event
{
const Header header = {
    "Copyright 2014-2018 DMTF in cooperation with the Storage Networking "
    "Industry Association (SNIA). All rights reserved.",
    "#MessageRegistry.v1_4_0.MessageRegistry",
    "ResourceEvent.1.0.3",
    "ResourceEvent Message Registry",
    "en",
    "This registry defines the messages to use for resource events.",
    "ResourceEvent",
    "1.0.3",
    "DMTF",
};
constexpr const char* url =
    "https://redfish.dmtf.org/registries/ResourceEvent.1.0.3.json";

constexpr std::array<MessageEntry, 2> registry = {
    MessageEntry{"ResourceCreated",
                 {
                     "Indicates that all conditions of a successful creation "
                     "operation have been met.",
                     "The resource has been created successfully.",
                     "OK",
                     0,
                     {},
                     "None.",
                 }},
    MessageEntry{"ResourceRemoved",
                 {
                     "Indicates that all conditions of a successful remove "
                     "operation have been met.",
                     "The resource has been removed successfully.",
                     "OK",
                     0,
                     {},
                     "None.",
                 }},
};
} // namespace redfish::message_registries::resource_event
