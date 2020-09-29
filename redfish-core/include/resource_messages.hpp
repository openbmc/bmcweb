#pragma once

namespace redfish
{
namespace messages
{

inline nlohmann::json resourceChanged()
{
    return nlohmann::json{
        {"EventType", "ResourceChanged"},
        {"MessageId", "ResourceEvent.1.0.3.ResourceChanged"},
        {"Message", "One or more resource properties have changed."},
        {"MessageArgs", {}},
        {"Severity", "OK"},
        {"MessageSeverity", "OK"}};
}

inline nlohmann::json resourceCreated()
{
    return nlohmann::json{
        {"EventType", "ResourceAdded"},
        {"MessageId", "ResourceEvent.1.0.3.ResourceCreated"},
        {"Message", "The resource has been created successfully."},
        {"MessageArgs", {}},
        {"Severity", "OK"},
        {"MessageSeverity", "OK"}};
}

inline nlohmann::json resourceRemoved()
{
    return nlohmann::json{
        {"EventType", "ResourceRemoved"},
        {"MessageId", "ResourceEvent.1.0.3.ResourceRemoved"},
        {"Message", "The resource has been removed successfully."},
        {"MessageArgs", {}},
        {"Severity", "OK"},
        {"MessageSeverity", "OK"}};
}

} // namespace messages
} // namespace redfish
