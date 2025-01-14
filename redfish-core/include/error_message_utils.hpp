#include <nlohmann/json.hpp>

#include <string_view>

namespace redfish
{

namespace messages
{

constexpr const char* messageVersionPrefix = "Base.1.19.0.";
constexpr const char* messageAnnotation = "@Message.ExtendedInfo";

void moveErrorsToErrorJson(nlohmann::json& target, nlohmann::json& source);
void addMessageToJsonRoot(nlohmann::json& target,
                          const nlohmann::json& message);

void addMessageToJson(nlohmann::json& target, const nlohmann::json& message,
                      std::string_view fieldPath);

void addMessageToErrorJson(nlohmann::json& target,
                           const nlohmann::json& message);
} // namespace messages
} // namespace redfish
