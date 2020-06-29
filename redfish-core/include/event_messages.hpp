#pragma once
namespace redfish
{
namespace messages
{

nlohmann::json LockAcquired(const std::string& arg1, const std::string& arg2,
                            const uint32_t arg3)
{
    return nlohmann::json{
        {"EventType", "Alert"},
        {"MessageId", "OpenBMC.0.1.0.LockAcquired"},
        {"Message", "Management console " + arg1 +
                        " has acquired the lock using session " + arg2 +
                        " with transaction id " + std::to_string(arg3) + "."},
        {"MessageArgs", {arg1, arg2, arg3}},
        {"MessageSeverity", "OK"}};
}

} // namespace messages
} // namespace redfish
