#pragma once

#include "redfish_util.hpp"
#include "registries.hpp"
#include "registries/license_message_registry.hpp"

#include <app.hpp>
#include <error_messages.hpp>
#include <registries/privilege_registry.hpp>
#include <utils/json_utils.hpp>

#include <array>
#include <filesystem>
#include <string>
#include <string_view>
#include <variant>
namespace redfish
{
namespace messages
{
static void addMessageToErrorJson(nlohmann::json& target,
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
            BMCWEB_LOG_ERROR("Attempt to add error message without MessageId");
            return;
        }

        auto messageFieldIterator = message.find("Message");
        if (messageFieldIterator == message.end())
        {
            BMCWEB_LOG_ERROR("Attempt to add error message without Message");
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

static void addMessageToJson(nlohmann::json& target,
                             const nlohmann::json& message,
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
inline nlohmann::json getLicenseLog(redfish::registries::license::Index name,
                                    std::span<const std::string_view> args)
{
    size_t index = static_cast<size_t>(name);
    if (index >= redfish::registries::base::registry.size())
    {
        return {};
    }
    return getLogFromRegistry(redfish::registries::license::header,
                              redfish::registries::license::registry, index,
                              args);
}

inline nlohmann::json licenseInstalled(std::string_view arg1)
{
    return getLicenseLog(redfish::registries::license::Index::licenseInstalled,
                         std::to_array({arg1}));
}

static void licenseInstalled(crow::Response& res,
                             const std::string& licenseString)
{
    res.result(boost::beast::http::status::ok);
    addMessageToJson(res.jsonValue, licenseInstalled(licenseString),
                     "LicenseString");
}

inline nlohmann::json invalidLicense()
{
    return getLicenseLog(redfish::registries::license::Index::invalidLicense,
                         {});
}

static void invalidLicense(crow::Response& res)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, invalidLicense());
}

inline nlohmann::json installFailed(std::string_view arg1)
{
    return getLicenseLog(redfish::registries::license::Index::installFailed,
                         std::to_array({arg1}));
}

static void installFailed(crow::Response& res, const std::string& reason)
{
    res.result(boost::beast::http::status::internal_server_error);
    addMessageToErrorJson(res.jsonValue, installFailed(reason));
}

inline nlohmann::json notApplicableToTarget()
{
    return getLicenseLog(
        redfish::registries::license::Index::notApplicableToTarget, {});
}

static void notApplicableToTarget(crow::Response& res)
{
    res.result(boost::beast::http::status::bad_request);
    addMessageToErrorJson(res.jsonValue, notApplicableToTarget());
}

} // namespace messages

static std::unique_ptr<sdbusplus::bus::match::match>&
    getLicenseActivationStatusMatch()
{
    static std::unique_ptr<sdbusplus::bus::match::match>
        licenseActivationStatusMatch;
    return licenseActivationStatusMatch;
}

inline void getLicenseEntryCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    sdbusplus::message::object_path path("/xyz/openbmc_project/license");
    dbus::utility::getManagedObjects(
        "xyz.openbmc_project.PLDM", path,
        [asyncResp](const boost::system::error_code& ec,
                    const dbus::utility::ManagedObjectType& resp) {
        if (ec)
        {
            BMCWEB_LOG_ERROR("LicenseEntry resp_handler got error {}", ec);
            // continue here to show zero members
            asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
            asyncResp->res.jsonValue["Members@odata.count"] = 0;
            return;
        }

        nlohmann::json& entriesArray = asyncResp->res.jsonValue["Members"];
        entriesArray = nlohmann::json::array();

        for (const auto& object : resp)
        {
            std::string entryID = object.first.filename();
            if (entryID.empty())
            {
                continue;
            }
            entriesArray.push_back({});
            nlohmann::json& thisEntry = entriesArray.back();
            thisEntry["@odata.id"] =
                std::format("/redfish/v1/LicenseService/Licenses/{}", entryID);
            thisEntry["Id"] = entryID;
            thisEntry["Name"] = entryID + " License Entry";
            asyncResp->res.jsonValue["Members@odata.count"] =
                entriesArray.size();
        }
    });
}

inline void requestRoutesLicenseService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/LicenseService/")
        .privileges({{"Login"}})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/LicenseService/";
        asyncResp->res.jsonValue["@odata.type"] =
            "#LicenseService.v1_0_0.LicenseService";
        asyncResp->res.jsonValue["Name"] = "License Service";
        asyncResp->res.jsonValue["Id"] = "LicenseService";

        asyncResp->res.jsonValue["Licenses"]["@odata.id"] =
            "/redfish/v1/LicenseService/Licenses";
        asyncResp->res.jsonValue["Actions"] = {
            {"#LicenseService.Install",
             {{
                 "target",
                 "/redfish/v1/LicenseService/Actions/"
                 "LicenseService.Install",
             }}}};
    });
}

static void resetLicenseActivationStatus(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    std::string value{"com.ibm.License.LicenseManager.Status.Pending"};
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code& ec) {
        if (ec)
        {
            BMCWEB_LOG_ERROR("DBUS response error: Unable to set "
                             "the LicenseString property {}",
                             ec);
            messages::internalError(asyncResp->res);
            return;
        }
        getLicenseActivationStatusMatch() = nullptr;
    },
        "com.ibm.License.Manager", "/com/ibm/license",
        "org.freedesktop.DBus.Properties", "Set",
        "com.ibm.License.LicenseManager", "LicenseActivationStatus",
        std::variant<std::string>(value));
}

static void
    resetLicenseString(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    std::string value;
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code& ec) {
        if (ec)
        {
            BMCWEB_LOG_ERROR("DBUS response error: Unable to set "
                             "the LicenseString property {}",
                             ec);
            messages::internalError(asyncResp->res);
            return;
        }
        resetLicenseActivationStatus(asyncResp);
    },
        "com.ibm.License.Manager", "/com/ibm/license",
        "org.freedesktop.DBus.Properties", "Set",
        "com.ibm.License.LicenseManager", "LicenseString",
        std::variant<std::string>(value));
}

inline void
    getLicenseActivationAck(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& status,
                            const std::string& licenseString)
{
    if (status == "com.ibm.License.LicenseManager.Status.ActivationFailed")
    {
        BMCWEB_LOG_WARNING("LicenseActivationStatus: ActivationFailed");
        messages::installFailed(asyncResp->res, "ActivationFailed");
    }
    else if (status == "com.ibm.License.LicenseManager.Status.InvalidLicense")
    {
        BMCWEB_LOG_WARNING("LicenseActivationStatus: InvalidLicense");
        messages::invalidLicense(asyncResp->res);
    }
    else if (status == "com.ibm.License.LicenseManager.Status.IncorrectSystem")
    {
        BMCWEB_LOG_WARNING("LicenseActivationStatus: IncorrectSystem");
        messages::notApplicableToTarget(asyncResp->res);
    }
    else if (status ==
             "com.ibm.License.LicenseManager.Status.IncorrectSequence")
    {
        BMCWEB_LOG_WARNING("LicenseActivationStatus: IncorrectSequence");
        messages::notApplicableToTarget(asyncResp->res);
    }
    else if (status == "com.ibm.License.LicenseManager.Status.InvalidHostState")
    {
        BMCWEB_LOG_WARNING("LicenseActivationStatus: InvalidHostState");
        messages::installFailed(asyncResp->res, "InvalidHostState");
    }
    else if (status == "com.ibm.License.LicenseManager.Status.Activated")
    {
        BMCWEB_LOG_INFO("License Activated");
        messages::licenseInstalled(asyncResp->res, licenseString);
    }
    else
    {
        auto ires = asyncResp->res.resultInt();
        BMCWEB_LOG_ERROR("License Activation Failed: {}", ires);
        messages::internalError(asyncResp->res);
    }
    // Reset LicenseString D-bus property to empty string
    // after request status populated by pldmd.
    resetLicenseString(asyncResp);
}
static void
    handleLicensePost(const crow::Request& req,
                      const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    std::string licenseString;
    if (!json_util::readJsonAction(req, asyncResp->res, "LicenseString",
                                   licenseString))
    {
        return;
    }

    if (licenseString.empty())
    {
        messages::propertyMissing(asyncResp->res, "LicenseString");
        return;
    }

    // Only allow one License install at a time
    if (getLicenseActivationStatusMatch() != nullptr)
    {
        messages::resourceInUse(asyncResp->res);
        return;
    }

    std::shared_ptr<boost::asio::steady_timer> timeout =
        std::make_shared<boost::asio::steady_timer>(
            crow::connections::systemBus->get_io_context());
    timeout->expires_after(std::chrono::seconds(20));
    crow::connections::systemBus->async_method_call(
        [timeout, asyncResp,
         licenseString](const boost::system::error_code& ec) {
        if (ec)
        {
            BMCWEB_LOG_ERROR("LicenseString resp_handler got error {}", ec);
            messages::internalError(asyncResp->res);
            return;
        }

        auto timeoutHandler =
            [asyncResp, timeout](const boost::system::error_code& errCode) {
            resetLicenseString(asyncResp);
            if (errCode)
            {
                if (errCode != boost::asio::error::operation_aborted)
                {
                    BMCWEB_LOG_ERROR("Async_wait failed {}", errCode);
                    messages::internalError(asyncResp->res);
                    return;
                }
            }
            else
            {
                BMCWEB_LOG_WARNING("Timed out waiting for HostInterface to "
                                   "serve license upload request");
                messages::serviceTemporarilyUnavailable(asyncResp->res, "60");
                return;
            }
        };

        timeout->async_wait(timeoutHandler);

        auto callback = [asyncResp, timeout,
                         licenseString](sdbusplus::message::message& m) {
            BMCWEB_LOG_DEBUG("Response Matched ");
            boost::container::flat_map<std::string, std::variant<std::string>>
                values;
            std::string iface;
            m.read(iface, values);
            if (iface == "com.ibm.License.LicenseManager")
            {
                auto findStatus = values.find("LicenseActivationStatus");
                if (findStatus != values.end())
                {
                    BMCWEB_LOG_INFO("Found Status property change");
                    std::string* status =
                        std::get_if<std::string>(&(findStatus->second));
                    if (status == nullptr)
                    {
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    // Ignore D-bus propertyChanged signal for
                    // status value "Pending"
                    if (*status ==
                        "com.ibm.License.LicenseManager.Status.Pending")
                    {
                        return;
                    }
                    getLicenseActivationAck(asyncResp, *status, licenseString);
                    timeout->cancel();
                }
            }
        };

        getLicenseActivationStatusMatch() =
            std::make_unique<sdbusplus::bus::match::match>(
                *crow::connections::systemBus,
                "interface='org.freedesktop.DBus.Properties',type='"
                "signal',"
                "member='PropertiesChanged',path='/com/ibm/license'",
                callback);
    },
        "com.ibm.License.Manager", "/com/ibm/license",
        "org.freedesktop.DBus.Properties", "Set",
        "com.ibm.License.LicenseManager", "LicenseString",
        std::variant<std::string>(licenseString));
}
inline void requestRoutesLicenseEntryCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/LicenseService/Licenses")
        .privileges({{"Login"}})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        asyncResp->res.jsonValue["@odata.type"] =
            "#LicenseCollection.LicenseCollection";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/LicenseService/Licenses";
        asyncResp->res.jsonValue["Name"] = "License Collection";

        getLicenseEntryCollection(asyncResp);
    });

    BMCWEB_ROUTE(app, "/redfish/v1/LicenseService/Licenses")
        .privileges({{"ConfigureManager"}})
        .methods(boost::beast::http::verb::post)(handleLicensePost);
}

inline void translateLicenseTypeDbusToRedfish(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& licenseType)
{
    if (licenseType == "com.ibm.License.Entry.LicenseEntry.Type.Purchased")
    {
        asyncResp->res.jsonValue["LicenseType"] = "Production";
    }
    else if (licenseType == "com.ibm.License.Entry.LicenseEntry.Type.Prototype")
    {
        asyncResp->res.jsonValue["LicenseType"] = "Prototype";
    }
    else if (licenseType == "com.ibm.License.Entry.LicenseEntry.Type.Trial")
    {
        asyncResp->res.jsonValue["LicenseType"] = "Trial";
    }
    else
    {
        // Any other values would be invalid
        BMCWEB_LOG_ERROR("LicenseType value was not valid: {}", licenseType);
        messages::internalError(asyncResp->res);
    }
}

inline void translateAuthorizationTypeDbusToRedfish(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& authorizationType)
{
    if (authorizationType ==
        "com.ibm.License.Entry.LicenseEntry.AuthorizationType.Unlimited")
    {
        asyncResp->res.jsonValue["AuthorizationScope"] = "Service";
    }
    else if (authorizationType ==
             "com.ibm.License.Entry.LicenseEntry.AuthorizationType.Device")
    {
        asyncResp->res.jsonValue["AuthorizationScope"] = "Device";
    }
    else if (authorizationType ==
             "com.ibm.License.Entry.LicenseEntry.AuthorizationType.Capacity")
    {
        asyncResp->res.jsonValue["AuthorizationScope"] = "Capacity";
    }
    else
    {
        // Any other values would be invalid
        BMCWEB_LOG_ERROR("AuthorizationType value is not valid: {}",
                         authorizationType);
        messages::internalError(asyncResp->res);
    }
}

inline void
    getLicenseEntryById(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& licenseEntryID)
{
    sdbusplus::message::object_path path("/xyz/openbmc_project/license");
    dbus::utility::getManagedObjects(
        "xyz.openbmc_project.PLDM", path,

        [asyncResp,
         licenseEntryID](const boost::system::error_code& ec,
                         const dbus::utility::ManagedObjectType& resp) {
        if (ec)
        {
            BMCWEB_LOG_ERROR("LicenseEntry resp_handler got error {}",
                             ec.value());
            messages::internalError(asyncResp->res);
            return;
        }

        std::string licenseEntryPath = "/xyz/openbmc_project/license/entry/" +
                                       licenseEntryID;

        for (const auto& objectPath : resp)
        {
            if (objectPath.first.str != licenseEntryPath)
            {
                continue;
            }

            uint64_t expirationTime = 0;
            const uint32_t* deviceNumPtr = nullptr;
            const std::string* serialNumPtr = nullptr;
            const std::string* licenseNamePtr = nullptr;
            const std::string* licenseTypePtr = nullptr;
            const std::string* authorizationTypePtr = nullptr;
            bool available = false;
            bool state = false;

            for (const auto& interfaceMap : objectPath.second)
            {
                if (interfaceMap.first == "com.ibm.License.Entry.LicenseEntry")
                {
                    for (const auto& propertyMap : interfaceMap.second)
                    {
                        if (propertyMap.first == "Name")
                        {
                            licenseNamePtr =
                                std::get_if<std::string>(&propertyMap.second);
                            if (licenseNamePtr == nullptr)
                            {
                                messages::internalError(asyncResp->res);
                                break;
                            }
                        }
                        else if (propertyMap.first == "Type")
                        {
                            licenseTypePtr =
                                std::get_if<std::string>(&propertyMap.second);
                            if (licenseTypePtr == nullptr)
                            {
                                messages::internalError(asyncResp->res);
                                break;
                            }
                        }
                        else if (propertyMap.first == "AuthorizationType")
                        {
                            authorizationTypePtr =
                                std::get_if<std::string>(&propertyMap.second);
                            if (authorizationTypePtr == nullptr)
                            {
                                messages::internalError(asyncResp->res);
                                break;
                            }
                        }
                        else if (propertyMap.first == "AuthDeviceNumber")
                        {
                            deviceNumPtr =
                                std::get_if<uint32_t>(&propertyMap.second);
                            if (deviceNumPtr == nullptr)
                            {
                                messages::internalError(asyncResp->res);
                                break;
                            }
                        }
                        else if (propertyMap.first == "ExpirationTime")
                        {
                            const uint64_t* timePtr =
                                std::get_if<uint64_t>(&propertyMap.second);
                            if (timePtr == nullptr)
                            {
                                messages::internalError(asyncResp->res);
                                break;
                            }
                            expirationTime = *timePtr;
                        }
                        else if (propertyMap.first == "SerialNumber")
                        {
                            serialNumPtr =
                                std::get_if<std::string>(&propertyMap.second);
                            if (serialNumPtr == nullptr)
                            {
                                messages::internalError(asyncResp->res);
                                break;
                            }
                        }
                    }
                }
                else if (interfaceMap.first ==
                         "xyz.openbmc_project.State.Decorator.Availability")
                {
                    for (const auto& propertyMap : interfaceMap.second)
                    {
                        if (propertyMap.first == "Available")
                        {
                            const bool* availablePtr =
                                std::get_if<bool>(&propertyMap.second);
                            if (availablePtr == nullptr)
                            {
                                messages::internalError(asyncResp->res);
                                break;
                            }
                            available = *availablePtr;
                        }
                    }
                }
                if (interfaceMap.first == "xyz.openbmc_project.State.Decorator."
                                          "OperationalStatus")
                {
                    for (const auto& propertyMap : interfaceMap.second)
                    {
                        if (propertyMap.first == "Functional")
                        {
                            const bool* functionalPtr =
                                std::get_if<bool>(&propertyMap.second);
                            if (functionalPtr == nullptr)
                            {
                                messages::internalError(asyncResp->res);
                                break;
                            }
                            state = *functionalPtr;
                        }
                    }
                }
            }
            asyncResp->res.jsonValue["@odata.type"] = "#License.v1_0_0.License";
            asyncResp->res.jsonValue["@odata.id"] =
                "/redfish/v1/LicenseService/Licenses/" + licenseEntryID;
            asyncResp->res.jsonValue["Id"] = licenseEntryID;
            asyncResp->res.jsonValue["SerialNumber"] = *serialNumPtr;
            asyncResp->res.jsonValue["Name"] = *licenseNamePtr;
            asyncResp->res.jsonValue["ExpirationDate"] =
                redfish::time_utils::getDateTimeUint(expirationTime);
            translateLicenseTypeDbusToRedfish(asyncResp, *licenseTypePtr);
            translateAuthorizationTypeDbusToRedfish(asyncResp,
                                                    *authorizationTypePtr);
            asyncResp->res.jsonValue["MaxAuthorizedDevices"] = *deviceNumPtr;

            if (available)
            {
                asyncResp->res.jsonValue["Status"]["Health"] = "OK";
                if (state)
                {
                    asyncResp->res.jsonValue["Status"]["State"] = "Enabled";
                }
                else
                {
                    asyncResp->res.jsonValue["Status"]["State"] = "Disabled";
                }
            }
            else
            {
                asyncResp->res.jsonValue["Status"]["Health"] = "Critical";
                asyncResp->res.jsonValue["Status"]["State"] =
                    "UnavailableOffline";
            }
        }
    });
}

inline void requestRoutesLicenseEntry(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/LicenseService/Licenses/<str>/")
        .privileges({{"Login"}})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& param) {
        getLicenseEntryById(asyncResp, param);
    });
}
} // namespace redfish
