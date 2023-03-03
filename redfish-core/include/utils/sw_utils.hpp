#pragma once
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/resource.hpp"
#include "http/utility.hpp"
#include "utils/dbus_utils.hpp"

#include <boost/system/error_code.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace redfish
{
namespace sw_util
{
/* @brief String that indicates a bios software instance */
const char* biosPurpose =
    "xyz.openbmc_project.Software.Version.VersionPurpose.Host";

/* @brief String that indicates a BMC software instance */
const char* bmcPurpose =
    "xyz.openbmc_project.Software.Version.VersionPurpose.BMC";

/**
 * @brief Populate the running software version and image links
 *
 * @param[i,o] aResp             Async response object
 * @param[i]   swVersionPurpose  Indicates what target to look for
 * @param[i]   activeVersionPropName  Index in aResp->res.jsonValue to write
 * the running software version to
 * @param[i]   populateLinkToImages  Populate aResp->res "Links"
 * "ActiveSoftwareImage" with a link to the running software image and
 * "SoftwareImages" with a link to the all its software images
 *
 * @return void
 */
inline void
    populateSoftwareInformation(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                                const std::string& swVersionPurpose,
                                const std::string& activeVersionPropName,
                                const bool populateLinkToImages)
{
    // Used later to determine running (known on Redfish as active) Sw images
    dbus::utility::getAssociationEndPoints(
        "/xyz/openbmc_project/software/functional",
        [aResp, swVersionPurpose, activeVersionPropName, populateLinkToImages](
            const boost::system::error_code& ec,
            const dbus::utility::MapperEndPoints& functionalSw) {
        BMCWEB_LOG_DEBUG << "populateSoftwareInformation enter";
        if (ec)
        {
            BMCWEB_LOG_ERROR << "error_code = " << ec;
            BMCWEB_LOG_ERROR << "error msg = " << ec.message();
            messages::internalError(aResp->res);
            return;
        }

        if (functionalSw.empty())
        {
            // Could keep going and try to populate SoftwareImages but
            // something is seriously wrong, so just fail
            BMCWEB_LOG_ERROR << "Zero functional software in system";
            messages::internalError(aResp->res);
            return;
        }

        std::vector<std::string> functionalSwIds;
        // example functionalSw:
        // v as 2 "/xyz/openbmc_project/software/ace821ef"
        //        "/xyz/openbmc_project/software/230fb078"
        for (const auto& sw : functionalSw)
        {
            sdbusplus::message::object_path path(sw);
            std::string leaf = path.filename();
            if (leaf.empty())
            {
                continue;
            }

            functionalSwIds.push_back(leaf);
        }

        constexpr std::array<std::string_view, 1> interfaces = {
            "xyz.openbmc_project.Software.Version"};
        dbus::utility::getSubTree(
            "/xyz/openbmc_project/software", 0, interfaces,
            [aResp, swVersionPurpose, activeVersionPropName,
             populateLinkToImages, functionalSwIds](
                const boost::system::error_code& ec2,
                const dbus::utility::MapperGetSubTreeResponse& subtree) {
            if (ec2)
            {
                BMCWEB_LOG_ERROR << "error_code = " << ec2;
                BMCWEB_LOG_ERROR << "error msg = " << ec2.message();
                messages::internalError(aResp->res);
                return;
            }

            BMCWEB_LOG_DEBUG << "Found " << subtree.size() << " images";

            for (const std::pair<std::string,
                                 std::vector<std::pair<
                                     std::string, std::vector<std::string>>>>&
                     obj : subtree)
            {

                sdbusplus::message::object_path path(obj.first);
                std::string swId = path.filename();
                if (swId.empty())
                {
                    messages::internalError(aResp->res);
                    BMCWEB_LOG_ERROR << "Invalid software ID";

                    return;
                }

                bool runningImage = false;
                // Look at Ids from
                // /xyz/openbmc_project/software/functional
                // to determine if this is a running image
                if (std::find(functionalSwIds.begin(), functionalSwIds.end(),
                              swId) != functionalSwIds.end())
                {
                    runningImage = true;
                }

                // Now grab its version info
                sdbusplus::asio::getAllProperties(
                    *crow::connections::systemBus, obj.second[0].first,
                    obj.first, "xyz.openbmc_project.Software.Version",
                    [aResp, swId, runningImage, swVersionPurpose,
                     activeVersionPropName, populateLinkToImages](
                        const boost::system::error_code& ec3,
                        const dbus::utility::DBusPropertiesMap&
                            propertiesList) {
                    if (ec3)
                    {
                        BMCWEB_LOG_ERROR << "error_code = " << ec3;
                        BMCWEB_LOG_ERROR << "error msg = " << ec3.message();
                        // Have seen the code update app delete the D-Bus
                        // object, during code update, between the call to
                        // mapper and here. Just leave these properties off if
                        // resource not found.
                        if (ec3.value() == EBADR)
                        {
                            return;
                        }
                        messages::internalError(aResp->res);
                        return;
                    }
                    // example propertiesList
                    // a{sv} 2 "Version" s
                    // "IBM-witherspoon-OP9-v2.0.10-2.22" "Purpose"
                    // s
                    // "xyz.openbmc_project.Software.Version.VersionPurpose.Host"
                    const std::string* version = nullptr;
                    const std::string* swInvPurpose = nullptr;

                    const bool success = sdbusplus::unpackPropertiesNoThrow(
                        dbus_utils::UnpackErrorPrinter(), propertiesList,
                        "Purpose", swInvPurpose, "Version", version);

                    if (!success)
                    {
                        messages::internalError(aResp->res);
                        return;
                    }

                    if (version == nullptr || version->empty())
                    {
                        messages::internalError(aResp->res);
                        return;
                    }
                    if (swInvPurpose == nullptr ||
                        *swInvPurpose != swVersionPurpose)
                    {
                        // Not purpose we're looking for
                        return;
                    }

                    BMCWEB_LOG_DEBUG << "Image ID: " << swId;
                    BMCWEB_LOG_DEBUG << "Running image: " << runningImage;
                    BMCWEB_LOG_DEBUG << "Image purpose: " << *swInvPurpose;

                    if (populateLinkToImages)
                    {
                        nlohmann::json& softwareImageMembers =
                            aResp->res.jsonValue["Links"]["SoftwareImages"];
                        // Firmware images are at
                        // /redfish/v1/UpdateService/FirmwareInventory/<Id>
                        // e.g. .../FirmwareInventory/82d3ec86
                        nlohmann::json::object_t member;
                        member["@odata.id"] = crow::utility::urlFromPieces(
                            "redfish", "v1", "UpdateService",
                            "FirmwareInventory", swId);
                        softwareImageMembers.push_back(std::move(member));
                        aResp->res
                            .jsonValue["Links"]["SoftwareImages@odata.count"] =
                            softwareImageMembers.size();

                        if (runningImage)
                        {
                            nlohmann::json::object_t runningMember;
                            runningMember["@odata.id"] =
                                crow::utility::urlFromPieces(
                                    "redfish", "v1", "UpdateService",
                                    "FirmwareInventory", swId);
                            // Create the link to the running image
                            aResp->res
                                .jsonValue["Links"]["ActiveSoftwareImage"] =
                                std::move(runningMember);
                        }
                    }
                    if (!activeVersionPropName.empty() && runningImage)
                    {
                        aResp->res.jsonValue[activeVersionPropName] = *version;
                    }
                    });
            }
            });
        });
}

/**
 * @brief Translate input swState to Redfish state
 *
 * This function will return the corresponding Redfish state
 *
 * @param[i]   swState  The OpenBMC software state
 *
 * @return The corresponding Redfish state
 */
inline resource::State getRedfishSwState(const std::string& swState)
{
    if (swState == "xyz.openbmc_project.Software.Activation.Activations.Active")
    {
        return resource::State::Enabled;
    }
    if (swState == "xyz.openbmc_project.Software.Activation."
                   "Activations.Activating")
    {
        return resource::State::Updating;
    }
    if (swState == "xyz.openbmc_project.Software.Activation."
                   "Activations.StandbySpare")
    {
        return resource::State::StandbySpare;
    }
    BMCWEB_LOG_DEBUG << "Default sw state " << swState << " to Disabled";
    return resource::State::Disabled;
}

/**
 * @brief Translate input swState to Redfish health state
 *
 * This function will return the corresponding Redfish health state
 *
 * @param[i]   swState  The OpenBMC software state
 *
 * @return The corresponding Redfish health state
 */
inline std::string getRedfishSwHealth(const std::string& swState)
{
    if ((swState ==
         "xyz.openbmc_project.Software.Activation.Activations.Active") ||
        (swState == "xyz.openbmc_project.Software.Activation.Activations."
                    "Activating") ||
        (swState ==
         "xyz.openbmc_project.Software.Activation.Activations.Ready"))
    {
        return "OK";
    }
    BMCWEB_LOG_DEBUG << "Sw state " << swState << " to Warning";
    return "Warning";
}

/**
 * @brief Put status of input swId into json response
 *
 * This function will put the appropriate Redfish state of the input
 * software id to ["Status"]["State"] within the json response
 *
 * @param[i,o] aResp    Async response object
 * @param[i]   swId     The software ID to get status for
 * @param[i]   dbusSvc  The dbus service implementing the software object
 *
 * @return void
 */
inline void getSwStatus(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::shared_ptr<std::string>& swId,
                        const std::string& dbusSvc)
{
    BMCWEB_LOG_DEBUG << "getSwStatus: swId " << *swId << " svc " << dbusSvc;

    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, dbusSvc,
        "/xyz/openbmc_project/software/" + *swId,
        "xyz.openbmc_project.Software.Activation",
        [asyncResp,
         swId](const boost::system::error_code& errorCode,
               const dbus::utility::DBusPropertiesMap& propertiesList) {
        if (errorCode)
        {
            // not all swtypes are updateable, this is ok
            asyncResp->res.jsonValue["Status"]["State"] = "Enabled";
            return;
        }

        const std::string* swInvActivation = nullptr;

        const bool success = sdbusplus::unpackPropertiesNoThrow(
            dbus_utils::UnpackErrorPrinter(), propertiesList, "Activation",
            swInvActivation);

        if (!success)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        if (swInvActivation == nullptr)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        BMCWEB_LOG_DEBUG << "getSwStatus: Activation " << *swInvActivation;
        asyncResp->res.jsonValue["Status"]["State"] =
            getRedfishSwState(*swInvActivation);
        asyncResp->res.jsonValue["Status"]["Health"] =
            getRedfishSwHealth(*swInvActivation);
        });
}

/**
 * @brief Updates programmable status of input swId into json response
 *
 * This function checks whether software inventory component
 * can be programmable or not and fill's the "Updatable"
 * Property.
 *
 * @param[i,o] asyncResp  Async response object
 * @param[i]   swId       The software ID
 */
inline void
    getSwUpdatableStatus(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::shared_ptr<std::string>& swId)
{
    sdbusplus::asio::getProperty<std::vector<std::string>>(
        *crow::connections::systemBus, "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/software/updateable",
        "xyz.openbmc_project.Association", "endpoints",
        [asyncResp, swId](const boost::system::error_code& ec,
                          const std::vector<std::string>& objPaths) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << " error_code = " << ec
                             << " error msg =  " << ec.message();
            // System can exist with no updateable software,
            // so don't throw error here.
            return;
        }
        std::string reqSwObjPath = "/xyz/openbmc_project/software/" + *swId;

        if (std::find(objPaths.begin(), objPaths.end(), reqSwObjPath) !=
            objPaths.end())
        {
            asyncResp->res.jsonValue["Updateable"] = true;
            return;
        }
        });
}

} // namespace sw_util
} // namespace redfish
