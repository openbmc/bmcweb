#include "error_message_utils.hpp"

#include "logging.hpp"

#include <nlohmann/json.hpp>

#include <string>
#include <string_view>

namespace redfish
{

namespace messages
{

void addMessageToErrorJson(nlohmann::json& target,
                           const nlohmann::json& message)
{
    auto& error = target["error"];

    // If this is the first error message, fill in the information from the
    // first error message to the top level struct
    if (!error.is_object())
    {
        auto messageIdIterator = message.find("MessageId");
        if (messageIdIterator == message.end())
        {
            BMCWEB_LOG_CRITICAL(
                "Attempt to add error message without MessageId");
            return;
        }

        auto messageFieldIterator = message.find("Message");
        if (messageFieldIterator == message.end())
        {
            BMCWEB_LOG_CRITICAL("Attempt to add error message without Message");
            return;
        }
        error["code"] = *messageIdIterator;
        error["message"] = *messageFieldIterator;
    }
    else
    {
        // More than 1 error occurred, so the message has to be generic
        error["code"] = std::string(messageVersionPrefix) + "GeneralError";
        error["message"] = "A general error has occurred. See Resolution for "
                           "information on how to resolve the error.";
    }

    // This check could technically be done in the default construction
    // branch above, but because we need the pointer to the extended info field
    // anyway, it's more efficient to do it here.
    auto& extendedInfo = error[messages::messageAnnotation];
    if (!extendedInfo.is_array())
    {
        extendedInfo = nlohmann::json::array();
    }

    extendedInfo.push_back(message);
}

void moveErrorsToErrorJson(nlohmann::json& target, nlohmann::json& source)
{
    if (!source.is_object())
    {
        return;
    }
    auto errorIt = source.find("error");
    if (errorIt == source.end())
    {
        // caller puts error message in root
        messages::addMessageToErrorJson(target, source);
        source.clear();
        return;
    }
    auto extendedInfoIt = errorIt->find(messages::messageAnnotation);
    if (extendedInfoIt == errorIt->end())
    {
        return;
    }
    const nlohmann::json::array_t* extendedInfo =
        (*extendedInfoIt).get_ptr<const nlohmann::json::array_t*>();
    if (extendedInfo == nullptr)
    {
        source.erase(errorIt);
        return;
    }
    for (const nlohmann::json& message : *extendedInfo)
    {
        addMessageToErrorJson(target, message);
    }
    source.erase(errorIt);
}

void addMessageToJsonRoot(nlohmann::json& target, const nlohmann::json& message)
{
    if (!target[messages::messageAnnotation].is_array())
    {
        // Force object to be an array
        target[messages::messageAnnotation] = nlohmann::json::array();
    }

    target[messages::messageAnnotation].push_back(message);
}

void addMessageToJson(nlohmann::json& target, const nlohmann::json& message,
                      std::string_view fieldPath)
{
    std::string extendedInfo(fieldPath);
    extendedInfo += messages::messageAnnotation;

    nlohmann::json& field = target[extendedInfo];
    if (!field.is_array())
    {
        // Force object to be an array
        field = nlohmann::json::array();
    }

    // Object exists and it is an array so we can just push in the message
    field.push_back(message);
}

} // namespace messages
} // namespace redfish
