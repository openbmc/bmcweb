namespace redfish
{
namespace messages
{

nlohmann::json ResourceCreated(void)
{
    return nlohmann::json{
        {"EventType", "ResourceAdded"},
        {"MessageId", "ResourceEvent.1.0.3.ResourceCreated"},
        {"Message", "The resource has been created successfully."},
        {"MessageArgs", {}},
        {"Severity", "OK"}};
}

nlohmann::json ResourceRemoved(void)
{
    return nlohmann::json{
        {"EventType", "ResourceRemoved"},
        {"MessageId", "ResourceEvent.1.0.3.ResourceRemoved"},
        {"Message", "The resource has been removed successfully."},
        {"MessageArgs", {}},
        {"Severity", "OK"}};
}

} // namespace messages
} // namespace redfish
