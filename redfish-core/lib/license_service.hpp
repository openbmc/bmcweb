#pragma once

#include "registries.hpp"
#include "registries/base_message_registry.hpp"
#include "registries/openbmc_message_registry.hpp"
#include "sdsi_response_codes.hpp"
#include "task.hpp"

#include <app.hpp>
#include <boost/convert.hpp>
#include <boost/convert/strtol.hpp>
#include <dbus_utility.hpp>

#include <regex>

// new License Service

namespace redfish
{
namespace licenseService
{
static const std::string sdsiService = "xyz.openbmc_project.sdsi";
static const std::string featureEnableInterfaceName =
    "xyz.openbmc_project.CPU.FeatureEnable";
static const std::string sdsiObjectPath = "/xyz/openbmc_project/sdsi/";

inline bool getLastURLSegment(const std::string_view url,
                              std::string& featureName)
{
    std::size_t found = url.rfind('/');
    if (found == std::string::npos)
    {
        return false;
    }

    if ((found + 1) < url.length())
    {
        featureName = url.substr(found + 1);
        return true;
    }
    return false;
    // dummy comment
}

inline void
    handleAuthDevices(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      nlohmann::json& input, std::string& authDevice)

{
    std::vector<nlohmann::json> authDevices;
    if (!json_util::readJson(input, asyncResp->res, "AuthorizedDevices",
                             authDevices))
    {
        return;
    }
    else
    {
        if (authDevices.size() == 1)
        {
            const nlohmann::json& thisJson = authDevices[0];
            if ((thisJson.is_null()) || (thisJson.empty()))
            {
                messages::propertyValueTypeError(
                    asyncResp->res,
                    thisJson.dump(2, ' ', true,
                                  nlohmann::json::error_handler_t::replace),
                    "/Links/AuthorizedDevices/" + std::to_string(0));
                return;
            }
            else // extract the AuthorizedDevices from odata.id
            {
                // This is a copy, but it's required in this case because of how
                // readJson is structured
                nlohmann::json thisJsonCopy = thisJson;
                if (!json_util::readJson(thisJsonCopy, asyncResp->res,
                                         "@odata.id", authDevice))
                {
                    return;
                }
            }
        }
        else
        {
            messages::propertyValueFormatError(
                asyncResp->res,
                "array of " + std::to_string(authDevices.size()) + " objects",
                "#Links/AuthorizedDevices");
            return;
        }
    }
}
inline void fillCPULicenseCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    nlohmann::json& members, const std::string& objectPath,
    const std::string& method, const std::string& cpuInstance)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, &members, cpuInstance,
         method](const boost::system::error_code ec, bool feature_enable) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            if (feature_enable)
            {
                if (method == "GetProvisionState")
                {
                    members.push_back(
                        {{"@odata.id",
                          "/redfish/v1/LicenseService/Licenses/ProvisionLicense" +
                              cpuInstance}});
                }
                else
                {
                    members.push_back(
                        {{"@odata.id",
                          "/redfish/v1/LicenseService/Licenses/DynamicLicense" +
                              cpuInstance}});
                }
                asyncResp->res.jsonValue["Members@odata.count"] =
                    members.size();
            }
        },
        sdsiService, objectPath, featureEnableInterfaceName, method);
}
inline void fillCPULicenseInstance(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& objectPath, const std::string& method,
    const std::string& processorId, const std::string& licenseType,
    const std::string& reqURL)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, processorId, licenseType,
         reqURL](const boost::system::error_code ec, bool feature_enable) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            if (feature_enable)
            {
                asyncResp->res.jsonValue = {
                    {"@odata.id", reqURL},
                    {"@odata.type", "#Licenses.v1_0_0.Licenses"},
                    {"Id", licenseType + processorId},
                    {"Name", licenseType + " for " + processorId},
                    {"AuthorizationScope", "Device"}};
                asyncResp->res.jsonValue["Links"]["AuthorizedDevices"] = {
                    {{"@odata.id",
                      "/redfish/v1/Systems/system/Processors/" + processorId}}};
                if (licenseType == "ProvisionLicense")
                {
                    asyncResp->res.jsonValue["Oem"]["Intel"]["Links"]
                                            ["AuthorizedFeature"] = {
                        {{"@odata.id",
                          "/redfish/v1/Systems/system/Processors/" +
                              processorId + "/oem/Intel/ProvisionFeature"}}};
                }
                else
                {
                    asyncResp->res.jsonValue["Oem"]["Intel"]["Links"]
                                            ["AuthorizedFeature"] = {
                        {{"@odata.id",
                          "/redfish/v1/Systems/system/Processors/" +
                              processorId + "/oem/Intel/DynamicFeature"}}};
                }
            }
            else
            {
                messages::resourceNotFound(asyncResp->res,
                                           licenseType + processorId, reqURL);
                return;
            }
        },
        sdsiService, objectPath, featureEnableInterfaceName, method);
}

inline void
    createCPULicense(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const crow::Request& req, const std::string& objectPath,
                     const std::string& processorId,
                     const std::string& reqFeature,
                     const std::string& licenseString)
{
    std::string signalMatchStr, method;
    if (reqFeature == "ProvisionFeature")
    {

        signalMatchStr = "type='signal',interface='";
        signalMatchStr += featureEnableInterfaceName;
        signalMatchStr += "',member='";
        signalMatchStr += "ProvisionValid";
        signalMatchStr += "', path='";
        signalMatchStr += sdsiObjectPath;
        signalMatchStr += processorId;
        signalMatchStr += "'";
        method = "Provision";
    }
    else
    {
        signalMatchStr = "type='signal',interface='";
        signalMatchStr += featureEnableInterfaceName;
        signalMatchStr += "',member='";
        signalMatchStr += "FeaturesValid";
        signalMatchStr += "', path='";
        signalMatchStr += sdsiObjectPath;
        signalMatchStr += processorId;
        signalMatchStr += "'";
        method = "EnableFeatures";
    }

    auto createCPULicenseCallback = [asyncResp, signalMatchStr,
                                     payload(task::Payload(req))](
                                        const boost::system::error_code ec,
                                        int sdsiresponsecode) {
        if (ec)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        if (sdsiresponsecode ==
            static_cast<int>(redfish::SDSiResponseCode::sdsiSuccess))
        {

            std::shared_ptr<task::TaskData> task = task::TaskData::createTask(
                [](boost::system::error_code err, sdbusplus::message::message&,
                   const std::shared_ptr<task::TaskData>& taskData) {
                    if (!err)
                    {
                        taskData->messages.emplace_back(
                            messages::taskCompletedOK(
                                std::to_string(taskData->index)));
                        taskData->state = "Completed";
                    }
                    return task::completed;
                },
                signalMatchStr);
            task->startTimer(std::chrono::minutes(5));
            task->populateResp(asyncResp->res);
            task->payload.emplace(std::move(payload));
        }
        else if (sdsiresponsecode ==
                 static_cast<int>(
                     redfish::SDSiResponseCode::sdsiMethodInProgress))
        {
            messages::serviceTemporarilyUnavailable(asyncResp->res, "60");
            return;
        }
        else
        {
            messages::internalError(asyncResp->res);
            return;
        }
    };
    crow::connections::systemBus->async_method_call(
        std::move(createCPULicenseCallback), sdsiService, objectPath,
        featureEnableInterfaceName, method, licenseString);
}

inline bool getLicenseTypeAndProcessorId(const std::string& reqFeature,
                                         std::string& getMethod,
                                         std::string& licenseType,
                                         std::string& processorId)
{
    const std::regex provisionLicenseExpr("ProvisionLicensecpu([^/]+)");
    const std::regex dynamicLicenseExpr("DynamicLicensecpu([^/]+)");

    std::cmatch match;

    if (std::regex_match(reqFeature.c_str(), match, provisionLicenseExpr))
    {
        processorId = "cpu" + std::string(match[1].first);
        licenseType = "ProvisionLicense";
        getMethod = "GetProvisionState";
        return true;
    }
    else if (std::regex_match(reqFeature.c_str(), match, dynamicLicenseExpr))
    {
        processorId = "cpu" + std::string(match[1].first);
        licenseType = "DynamicLicense";
        getMethod = "GetFeatureState";
        return true;
    }
    else
    {
        return false;
    }
}

inline void addCPUProvisionDynamicLicenseCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp](boost::system::error_code ec,
                    const dbus::utility::MapperGetSubTreeResponse& subtree) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            nlohmann::json& members = asyncResp->res.jsonValue["Members"];
            members = nlohmann::json::array();
            std::string cpuinstance;
            for (const auto& [objectPath, serviceMap] : subtree)
            {
                // Ignore any configs without ending with desired
                // cpu name
                if (objectPath.empty() || serviceMap.empty())
                {
                    continue;
                }

                getLastURLSegment(objectPath, cpuinstance);
                fillCPULicenseCollection(asyncResp, members, objectPath,
                                         "GetProvisionState", cpuinstance);
                fillCPULicenseCollection(asyncResp, members, objectPath,
                                         "GetFeatureState", cpuinstance);
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/sdsi", 0,
        std::array<const char*, 1>{"xyz.openbmc_project.CPU.FeatureEnable"});
}

inline bool readAuthFeature(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            nlohmann::json& oemObject, std::string& authFeature)
{
    if (nlohmann::json oemIntelObject;
        !oemObject.empty() &&
        json_util::readJson(oemObject, asyncResp->res, "Intel", oemIntelObject))
    {
        if (nlohmann::json linkObject;
            !oemIntelObject.empty() &&
            json_util::readJson(oemIntelObject, asyncResp->res, "Links",
                                linkObject))
        {
            if (nlohmann::json authFeatureObject;
                !linkObject.empty() &&
                json_util::readJson(linkObject, asyncResp->res,
                                    "AuthorizedFeature", authFeatureObject))
            {
                if (json_util::readJson(authFeatureObject, asyncResp->res,
                                        "@odata.id", authFeature))
                {
                    return true;
                }
            }
        }
    }
    return false;
}

inline void addCPULicense(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const crow::Request& req, nlohmann::json& oemObject,
                          const std::string& authDevice,
                          const std::string& licenseString)
{
    std::string processorId, authFeature;
    if (!getLastURLSegment(authDevice, processorId))
    {
        messages::propertyValueNotInList(asyncResp->res, authDevice,
                                         "Links/0/AuthorizedDevices");
        return;
    }
    if ((!readAuthFeature(asyncResp, oemObject, authFeature)))
    {
        messages::propertyMissing(asyncResp->res,
                                  "Oem/Intel/Links/AuthorizedFeature");
        return;
    }
    const std::regex AuthFeatureRegex(
        "^/redfish/v1/Systems/system/Processors/([^/]+)/Oem/Intel/([^/]+)$");
    if (!std::regex_match(authFeature, AuthFeatureRegex))
    {
        messages::propertyValueNotInList(asyncResp->res, authFeature,
                                         "AuthorizedFeature");
        return;
    }

    std::string oemAuthFeatureType, method;
    if ((!getLastURLSegment(authFeature, oemAuthFeatureType)) ||
        (!((oemAuthFeatureType == "ProvisionFeature") ||
           (oemAuthFeatureType == "DynamicFeature"))))
    {
        messages::propertyValueNotInList(asyncResp->res, authFeature,
                                         "AuthorizedFeature");
        return;
    }
    // authdevice + /Oem/Intel/featuretype must be equal to authfeature.
    if (!((authDevice + "/Oem/Intel/" + oemAuthFeatureType) == authFeature))
    {
        messages::propertyValueNotInList(asyncResp->res, authFeature,
                                         "AuthorizedFeature");
        return;
    }
    crow::connections::systemBus->async_method_call(
        [&req, asyncResp, processorId, oemAuthFeatureType, licenseString,
         authDevice](boost::system::error_code ec,
                     const dbus::utility::MapperGetSubTreeResponse& subtree) {
            if (ec)
            {
                BMCWEB_LOG_WARNING << "D-Bus error: " << ec << ", "
                                   << ec.message();
                messages::internalError(asyncResp->res);
                return;
            }
            // validate the cpu instance to make sure resource
            // exists and throw 404 if not.
            for (const auto& [objectPath, serviceMap] : subtree)
            {
                // Ignore any configs without ending with desired
                // cpu name
                if (!objectPath.ends_with(processorId) || serviceMap.empty())
                {
                    continue;
                }
                createCPULicense(asyncResp, req, objectPath, processorId,
                                 oemAuthFeatureType, licenseString);
                return;
            }
            messages::propertyValueNotInList(asyncResp->res, authDevice,
                                             "Links/0/AuthorizedDevices");
            return;
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/sdsi", 0,
        std::array<const char*, 1>{"xyz.openbmc_project.CPU.FeatureEnable"});
}
} // namespace licenseService

inline void requestRoutesLicenseService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/LicenseService/")
        .privileges({{"ConfigureComponents"}})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue = {
                    {"@odata.type", "#LicenseService.v1_0_0.LicenseService"},
                    {"@odata.id", "/redfish/v1/LicenseService"},
                    {"Id", "LicenseService"},
                    {"Name", "License Service"},
                    {"Description", "Actions available to manage Licenses"},
                    {"LicenseExpirationWarningDays", 0},
                    {"ServiceEnabled", true}};

                asyncResp->res.jsonValue["Licenses"] = {
                    {"@odata.id", "/redfish/v1/LicenseService/Licenses"}};
            });
} // requestRoutesLicenseService

inline void requestRoutesLicesnsesCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/LicenseService/Licenses/")
        .privileges({{"ConfigureComponents"}})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#LicenseCollection.LicenseCollection";
                asyncResp->res.jsonValue["Name"] = "License Collection";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/LicenseService/Licenses/";

                licenseService::addCPUProvisionDynamicLicenseCollection(
                    asyncResp);
            });
}

inline void requestRoutesLicensePost(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/LicenseService/Licenses/")
        .privileges({{"ConfigureComponents"}})
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                std::string licenseString, authScope, authFeature, authDevice,
                    processorId;
                std::optional<nlohmann::json> oemObject;
                nlohmann::json linksObject;

                if (!redfish::json_util::readJsonPatch(
                        req, asyncResp->res, "LicenseString", licenseString,
                        "AuthorizationScope", authScope, "Links", linksObject,
                        "Oem", oemObject))
                {
                    return;
                }

                if (linksObject.is_null() || linksObject.empty())
                {

                    messages::propertyValueTypeError(
                        asyncResp->res,
                        linksObject.dump(
                            2, ' ', true,
                            nlohmann::json::error_handler_t::replace),
                        "Links");
                    return;
                }

                if (authScope != "Device" || authScope != "Capacity" ||
                    authScope != "Service")
                {
                    messages::propertyValueNotInList(asyncResp->res, authScope,
                                                     "AuthorizationScope");
                    return;
                }

                licenseService::handleAuthDevices(asyncResp, linksObject,
                                                  authDevice);
                if (authDevice.empty())
                {
                    // add property mising for Authorixation device.
                    messages::propertyMissing(asyncResp->res,
                                              "Links/AuthorizedDevices");
                    return;
                }

                // As of now we are supporting License post only for CPUx
                // Provising and Dynamic Feature so make sure, authdevice is of
                // the same pattern as of  valid CPU instance.
                const std::regex ProcessorRegex(
                    "^/redfish/v1/Systems/system/Processors/([^/]+)$");
                if (std::regex_match(authDevice, ProcessorRegex))
                {
                    if (authScope != "Device")
                    {
                        messages::propertyValueNotInList(
                            asyncResp->res, authScope, "AuthorizationScope");
                        return;
                    }

                    nlohmann::json& oemCPUObject = *oemObject;

                    licenseService::addCPULicense(asyncResp, req, oemCPUObject,
                                                  authDevice, licenseString);
                }
                // Add error property valeue not in list if only support for CPU
                // provisioning and dynamic features All other licensing
                // services when implemented should comes under else if here,
                // Throwing bad request for all other now
                else
                {
                    messages::propertyValueNotInList(
                        asyncResp->res, authDevice,
                        "Links/0/AuthorizedDevices");
                    return;
                }
            });
}

// Handler for License Instance
inline void requestRoutesLicensesInstance(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/LicenseService/Licenses/<str>/")
        .privileges({{"ConfigureComponents"}})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& param) {
                if (param.empty())
                {
                    messages::internalError(asyncResp->res);
                    return;
                }

                std::string licenseType, processorId, getMethod;
                if (licenseService::getLicenseTypeAndProcessorId(
                        param, getMethod, licenseType, processorId))
                {
                    // Handle Provision and Dynamic Licenses for CPU
                    crow::connections::systemBus->async_method_call(
                        [&req, asyncResp, processorId, licenseType, param,
                         getMethod](
                            boost::system::error_code ec,
                            const dbus::utility::MapperGetSubTreeResponse&
                                subtree) {
                            if (ec)
                            {
                                BMCWEB_LOG_WARNING << "D-Bus error: " << ec
                                                   << ", " << ec.message();
                                messages::internalError(asyncResp->res);
                                return;
                            }

                            for (const auto& [objectPath, serviceMap] : subtree)
                            {
                                // Ignore any configs without ending with
                                // desired cpu name
                                if (!objectPath.ends_with(processorId) ||
                                    serviceMap.empty())
                                {
                                    continue;
                                }
                                licenseService::fillCPULicenseInstance(
                                    asyncResp, objectPath, getMethod,
                                    processorId, licenseType,
                                    std::string(req.url));
                                return;
                            }
                            messages::resourceNotFound(asyncResp->res, param,
                                                       std::string(req.url));
                        },
                        "xyz.openbmc_project.ObjectMapper",
                        "/xyz/openbmc_project/object_mapper",
                        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
                        "/xyz/openbmc_project/sdsi", 0,
                        std::array<const std::string, 1>{
                            "xyz.openbmc_project.CPU.FeatureEnable"});
                }
                else
                {
                    // Handle license for other resoures here.
                    messages::resourceNotFound(asyncResp->res, param,
                                               std::string(req.url));
                }
            });
}

} // namespace redfish
