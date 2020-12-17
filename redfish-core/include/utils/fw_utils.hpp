#pragma once
#include <async_resp.hpp>

#include <algorithm>
#include <string>
#include <variant>
#include <vector>

namespace redfish
{
namespace fw_util
{
/* @brief String that indicates a bios firmware instance */
constexpr const char* biosPurpose =
    "xyz.openbmc_project.Software.Version.VersionPurpose.Host";

/* @brief String that indicates a BMC firmware instance */
constexpr const char* bmcPurpose =
    "xyz.openbmc_project.Software.Version.VersionPurpose.BMC";

/**
 * @brief Populate the running firmware version and image links
 *
 * @param[i,o] aResp             Async response object
 * @param[i]   fwVersionPurpose  Indicates what target to look for
 * @param[i]   activeVersionPropName  Index in aResp->res.jsonValue to write
 * the running firmware version to
 * @param[i]   populateLinkToImages  Populate aResp->res "Links"
 * "ActiveSoftwareImage" with a link to the running firmware image and
 * "SoftwareImages" with a link to the all its firmware images
 *
 * @return void
 */
inline void
    populateFirmwareInformation(const std::shared_ptr<AsyncResp>& aResp,
                                const std::string& fwVersionPurpose,
                                const std::string& activeVersionPropName,
                                const bool populateLinkToImages)
{
    // Used later to determine running (known on Redfish as active) FW images
    crow::connections::systemBus->async_method_call(
        [aResp, fwVersionPurpose, activeVersionPropName, populateLinkToImages](
            const boost::system::error_code ec,
            const std::variant<std::vector<std::string>>& resp) {
            BMCWEB_LOG_DEBUG << "populateFirmwareInformation enter";
            if (ec)
            {
                BMCWEB_LOG_ERROR << "error_code = " << ec;
                BMCWEB_LOG_ERROR << "error msg = " << ec.message();
                messages::internalError(aResp->res);
                return;
            }

            const std::vector<std::string>* functionalFw =
                std::get_if<std::vector<std::string>>(&resp);
            if ((functionalFw == nullptr) || (functionalFw->size() == 0))
            {
                // Could keep going and try to populate SoftwareImages but
                // something is seriously wrong, so just fail
                BMCWEB_LOG_ERROR << "Zero functional software in system";
                messages::internalError(aResp->res);
                return;
            }

            std::vector<std::string> functionalFwIds;
            // example functionalFw:
            // v as 2 "/xyz/openbmc_project/software/ace821ef"
            //        "/xyz/openbmc_project/software/230fb078"
            for (auto& fw : *functionalFw)
            {

                std::string::size_type idPos = fw.rfind('/');
                if (idPos == std::string::npos)
                {
                    BMCWEB_LOG_DEBUG << "Can't parse firmware ID!";
                    continue;
                }
                idPos++;
                if (idPos >= fw.size())
                {
                    BMCWEB_LOG_DEBUG << "Invalid firmware ID";
                    continue;
                }
                functionalFwIds.push_back(fw.substr(idPos));
            }

            crow::connections::systemBus->async_method_call(
                [aResp, fwVersionPurpose, activeVersionPropName,
                 populateLinkToImages, functionalFwIds](
                    const boost::system::error_code ec2,
                    const std::vector<
                        std::pair<std::string,
                                  std::vector<std::pair<
                                      std::string, std::vector<std::string>>>>>&
                        subtree) {
                    if (ec2)
                    {
                        BMCWEB_LOG_ERROR << "error_code = " << ec2;
                        BMCWEB_LOG_ERROR << "error msg = " << ec2.message();
                        messages::internalError(aResp->res);
                        return;
                    }

                    BMCWEB_LOG_DEBUG << "Found " << subtree.size() << " images";

                    for (const std::pair<
                             std::string,
                             std::vector<std::pair<
                                 std::string, std::vector<std::string>>>>& obj :
                         subtree)
                    {
                        // if can't parse fw id then return
                        std::string::size_type idPos = obj.first.rfind('/');
                        if (idPos == std::string::npos)
                        {
                            messages::internalError(aResp->res);
                            BMCWEB_LOG_ERROR << "Can't parse firmware ID!!";
                            return;
                        }
                        idPos++;
                        if (idPos >= obj.first.size())
                        {
                            messages::internalError(aResp->res);
                            BMCWEB_LOG_ERROR << "Invalid firmware ID";
                            return;
                        }
                        std::string swId = obj.first.substr(idPos);

                        bool runningImage = false;
                        // Look at Ids from
                        // /xyz/openbmc_project/software/functional
                        // to determine if this is a running image
                        if (std::find(functionalFwIds.begin(),
                                      functionalFwIds.end(),
                                      swId) != functionalFwIds.end())
                        {
                            runningImage = true;
                        }

                        // Now grab its version info
                        crow::connections::systemBus->async_method_call(
                            [aResp, swId, runningImage, fwVersionPurpose,
                             activeVersionPropName, populateLinkToImages](
                                const boost::system::error_code ec3,
                                const boost::container::flat_map<
                                    std::string,
                                    std::variant<bool, std::string, uint64_t,
                                                 uint32_t>>& propertiesList) {
                                if (ec3)
                                {
                                    BMCWEB_LOG_ERROR << "error_code = " << ec3;
                                    BMCWEB_LOG_ERROR << "error msg = "
                                                     << ec3.message();
                                    messages::internalError(aResp->res);
                                    return;
                                }
                                // example propertiesList
                                // a{sv} 2 "Version" s
                                // "IBM-witherspoon-OP9-v2.0.10-2.22" "Purpose"
                                // s
                                // "xyz.openbmc_project.Software.Version.VersionPurpose.Host"

                                boost::container::flat_map<
                                    std::string,
                                    std::variant<bool, std::string, uint64_t,
                                                 uint32_t>>::const_iterator it =
                                    propertiesList.find("Purpose");
                                if (it == propertiesList.end())
                                {
                                    BMCWEB_LOG_ERROR
                                        << "Can't find property \"Purpose\"!";
                                    messages::internalError(aResp->res);
                                    return;
                                }
                                const std::string* swInvPurpose =
                                    std::get_if<std::string>(&it->second);
                                if (swInvPurpose == nullptr)
                                {
                                    BMCWEB_LOG_ERROR << "wrong types for "
                                                        "property \"Purpose\"!";
                                    messages::internalError(aResp->res);
                                    return;
                                }

                                BMCWEB_LOG_DEBUG << "Image ID: " << swId;
                                BMCWEB_LOG_DEBUG << "Image purpose: "
                                                 << *swInvPurpose;
                                BMCWEB_LOG_DEBUG << "Running image: "
                                                 << runningImage;

                                if (*swInvPurpose != fwVersionPurpose)
                                {
                                    // Not purpose we're looking for
                                    return;
                                }

                                if (populateLinkToImages)
                                {
                                    nlohmann::json& softwareImageMembers =
                                        aResp->res.jsonValue["Links"]
                                                            ["SoftwareImages"];
                                    // Firmware images are at
                                    // /redfish/v1/UpdateService/FirmwareInventory/<Id>
                                    // e.g. .../FirmwareInventory/82d3ec86
                                    softwareImageMembers.push_back(
                                        {{"@odata.id",
                                          "/redfish/v1/UpdateService/"
                                          "FirmwareInventory/" +
                                              swId}});
                                    aResp->res.jsonValue
                                        ["Links"]
                                        ["SoftwareImages@odata.count"] =
                                        softwareImageMembers.size();

                                    if (runningImage)
                                    {
                                        // Create the link to the running image
                                        aResp->res
                                            .jsonValue["Links"]
                                                      ["ActiveSoftwareImage"] =
                                            {{"@odata.id",
                                              "/redfish/v1/UpdateService/"
                                              "FirmwareInventory/" +
                                                  swId}};
                                    }
                                }
                                if (!activeVersionPropName.empty() &&
                                    runningImage)
                                {
                                    it = propertiesList.find("Version");
                                    if (it == propertiesList.end())
                                    {
                                        BMCWEB_LOG_ERROR
                                            << "Can't find property "
                                               "\"Version\"!";
                                        messages::internalError(aResp->res);
                                        return;
                                    }
                                    const std::string* version =
                                        std::get_if<std::string>(&it->second);
                                    if (version == nullptr)
                                    {
                                        BMCWEB_LOG_ERROR
                                            << "Error getting fw version";
                                        messages::internalError(aResp->res);
                                        return;
                                    }

                                    aResp->res
                                        .jsonValue[activeVersionPropName] =
                                        *version;
                                }
                            },
                            obj.second[0].first, obj.first,
                            "org.freedesktop.DBus.Properties", "GetAll",
                            "xyz.openbmc_project.Software.Version");
                    }
                },
                "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetSubTree",
                "/xyz/openbmc_project/software", static_cast<int32_t>(0),
                std::array<const char*, 1>{
                    "xyz.openbmc_project.Software.Version"});
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/software/functional",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Association", "endpoints");

    return;
}

/**
 * @brief Translate input fwState to Redfish state
 *
 * This function will return the corresponding Redfish state
 *
 * @param[i]   fwState  The OpenBMC firmware state
 *
 * @return The corresponding Redfish state
 */
inline std::string getRedfishFWState(const std::string& fwState)
{
    if (fwState == "xyz.openbmc_project.Software.Activation.Activations.Active")
    {
        return "Enabled";
    }
    if (fwState == "xyz.openbmc_project.Software.Activation."
                   "Activations.Activating")
    {
        return "Updating";
    }
    if (fwState == "xyz.openbmc_project.Software.Activation."
                   "Activations.StandbySpare")
    {
        return "StandbySpare";
    }
    BMCWEB_LOG_DEBUG << "Default fw state " << fwState << " to Disabled";
    return "Disabled";
}

/**
 * @brief Translate input fwState to Redfish health state
 *
 * This function will return the corresponding Redfish health state
 *
 * @param[i]   fwState  The OpenBMC firmware state
 *
 * @return The corresponding Redfish health state
 */
inline std::string getRedfishFWHealth(const std::string& fwState)
{
    if ((fwState ==
         "xyz.openbmc_project.Software.Activation.Activations.Active") ||
        (fwState == "xyz.openbmc_project.Software.Activation.Activations."
                    "Activating") ||
        (fwState ==
         "xyz.openbmc_project.Software.Activation.Activations.Ready"))
    {
        return "OK";
    }
    BMCWEB_LOG_DEBUG << "FW state " << fwState << " to Warning";
    return "Warning";
}

/**
 * @brief Put status of input swId into json response
 *
 * This function will put the appropriate Redfish state of the input
 * firmware id to ["Status"]["State"] within the json response
 *
 * @param[i,o] aResp    Async response object
 * @param[i]   swId     The software ID to get status for
 * @param[i]   dbusSvc  The dbus service implementing the software object
 *
 * @return void
 */
inline void getFwStatus(const std::shared_ptr<AsyncResp>& asyncResp,
                        const std::shared_ptr<std::string>& swId,
                        const std::string& dbusSvc)
{
    BMCWEB_LOG_DEBUG << "getFwStatus: swId " << *swId << " svc " << dbusSvc;

    crow::connections::systemBus->async_method_call(
        [asyncResp,
         swId](const boost::system::error_code errorCode,
               const boost::container::flat_map<
                   std::string, std::variant<bool, std::string, uint64_t,
                                             uint32_t>>& propertiesList) {
            if (errorCode)
            {
                // not all fwtypes are updateable, this is ok
                asyncResp->res.jsonValue["Status"]["State"] = "Enabled";
                return;
            }
            boost::container::flat_map<
                std::string, std::variant<bool, std::string, uint64_t,
                                          uint32_t>>::const_iterator it =
                propertiesList.find("Activation");
            if (it == propertiesList.end())
            {
                BMCWEB_LOG_DEBUG << "Can't find property \"Activation\"!";
                messages::propertyMissing(asyncResp->res, "Activation");
                return;
            }
            const std::string* swInvActivation =
                std::get_if<std::string>(&it->second);
            if (swInvActivation == nullptr)
            {
                BMCWEB_LOG_DEBUG << "wrong types for property\"Activation\"!";
                messages::propertyValueTypeError(asyncResp->res, "",
                                                 "Activation");
                return;
            }
            BMCWEB_LOG_DEBUG << "getFwStatus: Activation " << *swInvActivation;
            asyncResp->res.jsonValue["Status"]["State"] =
                getRedfishFWState(*swInvActivation);
            asyncResp->res.jsonValue["Status"]["Health"] =
                getRedfishFWHealth(*swInvActivation);
        },
        dbusSvc, "/xyz/openbmc_project/software/" + *swId,
        "org.freedesktop.DBus.Properties", "GetAll",
        "xyz.openbmc_project.Software.Activation");
}

/**
 * @brief Updates programmable status of input swId into json response
 *
 * This function checks whether firmware inventory component
 * can be programmable or not and fill's the "Updateable"
 * Property.
 *
 * @param[i,o] asyncResp  Async response object
 * @param[i]   fwId       The firmware ID
 */
inline void getFwUpdateableStatus(const std::shared_ptr<AsyncResp>& asyncResp,
                                  const std::shared_ptr<std::string>& fwId)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, fwId](const boost::system::error_code ec,
                          const std::variant<std::vector<std::string>>& resp) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << " error_code = " << ec
                                 << " error msg =  " << ec.message();
                // System can exist with no updateable firmware,
                // so don't throw error here.
                return;
            }
            const std::vector<std::string>* objPaths =
                std::get_if<std::vector<std::string>>(&resp);
            if (objPaths)
            {
                std::string reqFwObjPath =
                    "/xyz/openbmc_project/software/" + *fwId;

                if (std::find((*objPaths).begin(), (*objPaths).end(),
                              reqFwObjPath) != (*objPaths).end())
                {
                    asyncResp->res.jsonValue["Updateable"] = true;
                    return;
                }
            }
            return;
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/software/updateable",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Association", "endpoints");

    return;
}

} // namespace fw_util
} // namespace redfish
