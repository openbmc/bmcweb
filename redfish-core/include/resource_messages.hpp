namespace redfish
{
namespace messages
{

nlohmann::json ResourceCreated(void)
{
    return nlohmann::json{
        {"@odata.type", "#MessageRegistry.v1_4_0.MessageRegistry"},
        {"MessageId", "ResourceEvent.1.0.3.ResourceCreated"},
        {"Message", "The resource has been created successfully."},
        {"MessageArgs", {}},
        {"Severity", "OK"},
        {"Resolution", "None."}};
}

nlohmann::json ResourceRemoved(void)
{
    return nlohmann::json{
        {"@odata.type", "#MessageRegistry.v1_4_0.MessageRegistry"},
        {"MessageId", "ResourceEvent.1.0.3.ResourceRemoved"},
        {"Message", "The resource has been removed successfully."},
        {"MessageArgs", {}},
        {"Severity", "OK"},
        {"Resolution", "None."}};
}

} // namespace messages
} // namespace redfish
