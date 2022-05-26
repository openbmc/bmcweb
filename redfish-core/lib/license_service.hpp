#pragma once

#include "registries.hpp"
#include "registries/base_message_registry.hpp"
#include "registries/openbmc_message_registry.hpp"
#include "sdsi_helper.hpp"
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
static const std::string featureEnableInterfaceName =
    "xyz.openbmc_project.CPU.FeatureEnable";
static const std::string sdsiObjectPath = "/xyz/openbmc_project/sdsi/";
// Interfaces which imply a D-Bus object represents a Processor
constexpr std::array<const char*, 1> cpuLicenseInterfaces = {
    "xyz.openbmc_project.CPU.FeatureEnable"};

inline std::string getLicenseTypeFromMethod(std::string_view method)
{
    if (method == "GetProvisionState")
    {
        return "ProvisionLicense";
    }
    else
    {
        return "DynamicLicense";
    }
}

inline void fillCPULicenseCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& objectPath, const std::string& method,
    const std::string& cpuInstance, const std::string& service)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, cpuInstance, method](const boost::system::error_code ec,
                                         bool feature_enable) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            if (feature_enable)
            {
                nlohmann::json::object_t member;
                member["@odata.id"] =
                    crow::utility::urlFromPieces(
                        "redfish", "v1", "LicenseService", "Licenses",
                        std::string(getLicenseTypeFromMethod(method)),
                        cpuInstance)
                        .string();

                (asyncResp->res.jsonValue["Members"])
                    .push_back(std::move(member));
                asyncResp->res.jsonValue["Members@odata.count"] =
                    (asyncResp->res.jsonValue["Members"]).size();
            }
        },
        service, objectPath, featureEnableInterfaceName, method);
}
inline void fillCPULicenseInstance(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& objectPath, const std::string& method,
    const std::string& processorId, const std::string& licenseType,
    const std::string& service)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, processorId,
         licenseType](const boost::system::error_code ec, bool feature_enable) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            if (!feature_enable)
            {
                messages::resourceNotFound(asyncResp->res, "Licenses",
                                           licenseType + processorId);
                return;
            }
            asyncResp->res.jsonValue["@odata.id"] =
                "/redfish/v1/LicenseService/Licenses/" + licenseType +
                processorId;
            asyncResp->res.jsonValue["@odata.type"] = "#License.v1_0_0.License";
            asyncResp->res.jsonValue["Id"] = licenseType + processorId;
            asyncResp->res.jsonValue["Name"] =
                licenseType + " for " + processorId;
            asyncResp->res.jsonValue["AuthorizationScope"] = "Device";
            nlohmann::json::array_t authorizedeDevices;
            nlohmann::json::object_t authDevice;
            authDevice["@odata.id"] =
                "/redfish/v1/Systems/system/Processors/" + processorId;
            authorizedeDevices.push_back(std::move(authDevice));

            asyncResp->res.jsonValue["Links"]["AuthorizedDevices"] =
                std::move(authorizedeDevices);

            if (licenseType == "ProvisionLicense")
            {
                asyncResp->res.jsonValue["Oem"]["Intel"]["Links"]
                                        ["AuthorizedFeature"]["@odata.id"] =
                    "/redfish/v1/Systems/system/Processors/" + processorId +
                    "/oem/Intel/ProvisionFeature";
            }
            else
            {
                asyncResp->res.jsonValue["Oem"]["Intel"]["Links"]
                                        ["AuthorizedFeature"]["@odata.id"] =
                    "/redfish/v1/Systems/system/Processors/" + processorId +
                    "/oem/Intel/DynamicFeature";
            }
            asyncResp->res.jsonValue["Oem"]["Intel"]["@odata.type"] =
                "#OemLicense.v1_0_0.License";
        },
        service, objectPath, featureEnableInterfaceName, method);
}

inline void createCPULicense(
    task::Payload&& payload,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& objectPath, const std::string& processorId,
    const std::string& reqFeature, const std::string& licenseString,
    const std::string& service)
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

    auto createCPULicenseTaskCallback = [asyncResp, signalMatchStr, payload](
                                            const boost::system::error_code ec,
                                            int sdsiresponsecode) {
        if (ec)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        if (static_cast<redfish::SDSiResponseCode>(sdsiresponsecode) ==
            redfish::SDSiResponseCode::sdsiSuccess)
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
        else if (static_cast<redfish::SDSiResponseCode>(sdsiresponsecode) ==
                 redfish::SDSiResponseCode::sdsiMethodInProgress)
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
        std::move(createCPULicenseTaskCallback), service, objectPath,
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
    if (std::regex_match(reqFeature.c_str(), match, dynamicLicenseExpr))
    {
        processorId = "cpu" + std::string(match[1].first);
        licenseType = "DynamicLicense";
        getMethod = "GetFeatureState";
        return true;
    }
    return false;
}

inline void addProcessorFeatureLicenses(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{

    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec,
                    const dbus::utility::MapperGetObject& getObjectType) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "ObjectMapper::GetObject call failed: "
                                 << ec;
                messages::internalError(asyncResp->res);

                return;
            }
            std::string service = getObjectType.begin()->first;
            BMCWEB_LOG_DEBUG << "GetObjectType: " << service;
            crow::connections::systemBus->async_method_call(
                [asyncResp, service](
                    boost::system::error_code ec,
                    const dbus::utility::MapperGetSubTreeResponse& subtree) {
                    if (ec)
                    {
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    nlohmann::json& members =
                        asyncResp->res.jsonValue["Members"];
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
                        cpuinstance = std::string(
                            (boost::urls::parse_path(objectPath).value())
                                .back());
                        fillCPULicenseCollection(asyncResp, objectPath,
                                                 "GetProvisionState",
                                                 cpuinstance, service);
                        fillCPULicenseCollection(asyncResp, objectPath,
                                                 "GetFeatureState", cpuinstance,
                                                 service);
                    }
                },
                "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetSubTree",
                sdsiObjectPath, 0,
                std::array<const char*, 1>{
                    "xyz.openbmc_project.CPU.FeatureEnable"});
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject",
        "/xyz/openbmc_project/sdsi", std::array<const char*, 0>());
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

inline bool getLicenseTypeFromAuthDevices(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    std::vector<std::string>& authDevices, std::string& licenseType)
{
    std::string authDevice;
    for (const std::string& authdev : authDevices)
    {
        authDevice =
            std::string((boost::urls::parse_path(authdev).value()).back());
        if (crow::utility::urlFromPieces("redfish", "v1", "Systems", "system",
                                         "Processors", authDevice)
                .string() == authdev)
        {
            if (authDevices.size() > 1)
            {
                messages::propertyValueFormatError(
                    asyncResp->res,
                    "array of " + std::to_string(authDevices.size()) +
                        " objects",
                    "#Links/AuthorizedDevices");
                return false;
            }
            licenseType = "cpuLicense";
            return true;
        }
        messages::propertyValueNotInList(asyncResp->res, authdev,
                                         "Links/AuthororizedDevices/0");
        return false;
    }
    return false;
}

inline void addCPULicense(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const crow::Request& req,
                          const std::string& authFeature,
                          const std::string& authDevice,
                          const std::string& licenseString)
{
    std::string processorId;
    processorId =
        std::string((boost::urls::parse_path(authDevice).value()).back());
    std::string oemAuthFeatureType, method;
    oemAuthFeatureType =
        std::string((boost::urls::parse_path(authFeature).value()).back());

    if (!((oemAuthFeatureType == "ProvisionFeature") ||
          (oemAuthFeatureType == "DynamicFeature")))
    {
        messages::propertyValueNotInList(asyncResp->res, authFeature,
                                         "AuthorizedFeature");
        return;
    }

    // authdevice + /Oem/Intel/featuretype must be equal to authfeature.
    if (!((authDevice + "/Oem/Intel/" + oemAuthFeatureType) == authFeature))
    {
        messages::propertyValueNotInList(asyncResp->res, authFeature,
                                         "Oem/Intel/Links/AuthorizedFeature");
        return;
    }
    crow::connections::systemBus->async_method_call(
        [asyncResp, req, processorId, oemAuthFeatureType, licenseString,
         authDevice](const boost::system::error_code ec,
                     const dbus::utility::MapperGetObject& getObjectType) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "ObjectMapper::GetObject call failed: "
                                 << ec;
                messages::internalError(asyncResp->res);

                return;
            }
            std::string service = getObjectType.begin()->first;
            BMCWEB_LOG_DEBUG << "GetObjectType: " << service;

            crow::connections::systemBus->async_method_call(
                [asyncResp, payload(task::Payload(req)), processorId,
                 oemAuthFeatureType, licenseString, authDevice,
                 service](boost::system::error_code ec,
                          const dbus::utility::MapperGetSubTreeResponse&
                              subtree) mutable {
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
                        if (!objectPath.ends_with(processorId) ||
                            serviceMap.empty())
                        {
                            continue;
                        }
                        bool found = false;
                        for (const auto& [serviceName, interfaceList] :
                             serviceMap)
                        {
                            if (std::find_first_of(
                                    interfaceList.begin(), interfaceList.end(),
                                    licenseService::cpuLicenseInterfaces
                                        .begin(),
                                    licenseService::cpuLicenseInterfaces
                                        .end()) != interfaceList.end())
                            {
                                found = true;
                                break;
                            }
                        }
                        if (!found)
                        {
                            continue;
                        }
                        createCPULicense(std::move(payload), asyncResp,
                                         objectPath, processorId,
                                         oemAuthFeatureType, licenseString,
                                         service);
                        return;
                    }
                    messages::propertyValueNotInList(
                        asyncResp->res, authDevice,
                        "Links/0/AuthorizedDevices");
                },
                "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetSubTree",
                sdsiObjectPath, 0,
                std::array<const char*, 1>{
                    "xyz.openbmc_project.CPU.FeatureEnable"});
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject",
        "/xyz/openbmc_project/sdsi", std::array<const char*, 0>());
}
} // namespace licenseService

inline void
    handleLicenseServiceGet(App& app, const crow::Request& req,
                            const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
    {
        return;
    }
    asyncResp->res.jsonValue["@odata.type"] =
        "#LicenseService.v1_0_0.LicenseService";
    asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/LicenseService";
    asyncResp->res.jsonValue["Id"] = "LicenseService";
    asyncResp->res.jsonValue["Name"] = "License Service";
    asyncResp->res.jsonValue["Description"] =
        "Actions available to manage Licenses";
    asyncResp->res.jsonValue["ServiceEnabled"] = true;
    asyncResp->res.jsonValue["Licenses"]["@odata.id"] =
        "/redfish/v1/LicenseService/Licenses";

} // requestRoutesLicenseService

inline void handleLicenseCollectionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
    {
        return;
    }
    asyncResp->res.jsonValue["@odata.type"] =
        "#LicenseCollection.LicenseCollection";
    asyncResp->res.jsonValue["Name"] = "License Collection";
    asyncResp->res.jsonValue["@odata.id"] =
        "/redfish/v1/LicenseService/Licenses/";

    licenseService::addProcessorFeatureLicenses(asyncResp);
}

inline void handleLicenseCollectionPost(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
    {
        return;
    }
    std::string licenseString, authScope, authFeature, processorId;
    std::optional<nlohmann::json> oemObject;
    std::vector<nlohmann::json> linksAuthDevArray;
    std::vector<std::string> authDevices;

    if (!redfish::json_util::readJsonPatch(req, asyncResp->res, "LicenseString",
                                           licenseString, "AuthorizationScope",
                                           authScope, "Links/AuthorizedDevices",
                                           linksAuthDevArray, "Oem", oemObject))
    {
        return;
    }

    if ((authScope != "Device") && (authScope != "Capacity") &&
        (authScope != "Service"))
    {
        messages::propertyValueNotInList(asyncResp->res, authScope,
                                         "AuthorizationScope");
        return;
    }

    if (linksAuthDevArray.empty())
    {
        messages::propertyValueTypeError(asyncResp->res, "[]",
                                         "/Links/AuthorizedDevices/");
        return;
    }

    for (nlohmann::json authDevObj : linksAuthDevArray)
    {
        std::string authDev;
        if (!json_util::readJson(authDevObj, asyncResp->res, "@odata.id",
                                 authDev))

        {
            return;
        }
        authDevices.push_back(authDev);
    }

    std::string licenseType;
    if (!licenseService::getLicenseTypeFromAuthDevices(asyncResp, authDevices,
                                                       licenseType))
    {
        return;
    }
    if (licenseType == "cpuLicense")
    {
        std::string authDevice = authDevices[0];
        if (authScope != "Device")
        {
            messages::propertyValueNotInList(asyncResp->res, authScope,
                                             "AuthorizationScope");
            return;
        }
        if (!oemObject || !(licenseService::readAuthFeature(
                              asyncResp, *oemObject, authFeature)))
        {
            messages::propertyMissing(asyncResp->res,
                                      "Oem/Intel/Links/AuthorizedFeature");
            return;
        }
        licenseService::addCPULicense(asyncResp, req, authFeature, authDevice,
                                      licenseString);
    }
}

// Handler for License Instance
inline void
    handleLicenseGet(App& app, const crow::Request& req,
                     const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const std::string& param)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
    {
        return;
    }
    std::string licenseType, processorId, getMethod;
    if (!licenseService::getLicenseTypeAndProcessorId(param, getMethod,
                                                      licenseType, processorId))
    {
        // Handle license for other resoures here.
        messages::resourceNotFound(asyncResp->res, "Licenses", param);
        return;
    }

    // Handle Provision and Dynamic Licenses for CPU

    crow::connections::systemBus->async_method_call(
        [asyncResp, licenseType, processorId, getMethod,
         param](const boost::system::error_code ec,
                const dbus::utility::MapperGetObject& getObjectType) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "ObjectMapper::GetObject call failed: "
                                 << ec;
                messages::internalError(asyncResp->res);

                return;
            }
            std::string service = getObjectType.begin()->first;
            BMCWEB_LOG_DEBUG << "GetObjectType: " << service;

            crow::connections::systemBus->async_method_call(
                [asyncResp, processorId, licenseType, param, getMethod,
                 service](
                    boost::system::error_code ec,
                    const dbus::utility::MapperGetSubTreeResponse& subtree) {
                    if (ec)
                    {
                        BMCWEB_LOG_WARNING << "D-Bus error: " << ec << ", "
                                           << ec.message();
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    for (const auto& [objectPath, serviceMap] : subtree)
                    {
                        // Ignore any configs without ending with
                        // desired cpu name
                        if (!boost::ends_with(objectPath, processorId) ||
                            serviceMap.empty())
                        {
                            continue;
                        }

                        bool found = false;
                        for (const auto& [serviceName, interfaceList] :
                             serviceMap)
                        {
                            if (std::find_first_of(
                                    interfaceList.begin(), interfaceList.end(),
                                    licenseService::cpuLicenseInterfaces
                                        .begin(),
                                    licenseService::cpuLicenseInterfaces
                                        .end()) != interfaceList.end())
                            {
                                found = true;
                                break;
                            }
                        }
                        if (!found)
                        {
                            continue;
                        }
                        licenseService::fillCPULicenseInstance(
                            asyncResp, objectPath, getMethod, processorId,
                            licenseType, service);
                        return;
                    }
                    messages::resourceNotFound(asyncResp->res, "Licenses",
                                               param);
                },
                "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetSubTree",
                licenseService::sdsiObjectPath, 0,
                std::array<const char*, 1>{
                    "xyz.openbmc_project.CPU.FeatureEnable"});
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject",
        "/xyz/openbmc_project/sdsi", std::array<const char*, 0>());
}

inline void requestLicenseServiceRoutes(App& app)
{

    BMCWEB_ROUTE(app, "/redfish/v1/LicenseService/")
        .privileges(redfish::privileges::getLicenseService)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleLicenseServiceGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/LicenseService/Licenses/")
        .privileges(redfish::privileges::getLicenseCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleLicenseCollectionGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/LicenseService/Licenses/")
        .privileges(redfish::privileges::postLicenseCollection)
        .methods(boost::beast::http::verb::post)(
            std::bind_front(handleLicenseCollectionPost, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/LicenseService/Licenses/<str>/")
        .privileges(redfish::privileges::getLicense)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleLicenseGet, std::ref(app)));
}

} // namespace redfish
