#pragma once
#include <async_resp.hpp>
#include <string>

namespace redfish
{
namespace fw_util
{
/* @brief String that indicates a bios firmware instance */
constexpr const char *biosPurpose =
    "xyz.openbmc_project.Software.Version.VersionPurpose.Host";

/* @brief String that indicates a BMC firmware instance */
constexpr const char *bmcPurpose =
    "xyz.openbmc_project.Software.Version.VersionPurpose.BMC";

/**
 * @brief Put fw version of input type into async response json structure
 *
 * @param[i,o] aResp             Async response object
 * @param[i]   fwVersionPurpose  Indicates what target to look for
 * @param[i]   jsonIdxStr        Index in aResp->res.jsonValue to write fw ver
 *
 * @return void
 */
void getActiveFwVersion(std::shared_ptr<AsyncResp> aResp,
                        const std::string &fwVersionPurpose,
                        const std::string &jsonIdxStr)
{
    // Get active FW images
    crow::connections::systemBus->async_method_call(
        [aResp, fwVersionPurpose,
         jsonIdxStr](const boost::system::error_code ec,
                     const std::variant<std::vector<std::string>> &resp) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "error_code = " << ec;
                BMCWEB_LOG_ERROR << "error msg = " << ec.message();
                messages::internalError(aResp->res);
                return;
            }
            const std::vector<std::string> *functionalFw =
                std::get_if<std::vector<std::string>>(&resp);
            if ((functionalFw == nullptr) || (functionalFw->size() == 0))
            {
                BMCWEB_LOG_ERROR << "Zero functional software in system";
                messages::internalError(aResp->res);
                return;
            }
            // example functionalFw:
            // v as 2 "/xyz/openbmc_project/software/ace821ef"
            //        "/xyz/openbmc_project/software/230fb078"
            for (auto &fw : *functionalFw)
            {
                // if can't parse fw id then return
                std::string::size_type idPos = fw.rfind("/");
                if (idPos == std::string::npos)
                {
                    messages::internalError(aResp->res);
                    BMCWEB_LOG_DEBUG << "Can't parse firmware ID!!";
                    return;
                }
                std::string swId = fw.substr(idPos + 1);

                // Now find service that hosts it
                crow::connections::systemBus->async_method_call(
                    [aResp, fw, swId, fwVersionPurpose, jsonIdxStr](
                        const boost::system::error_code ec,
                        const std::vector<std::pair<
                            std::string, std::vector<std::string>>> &objInfo) {
                        if (ec)
                        {
                            BMCWEB_LOG_DEBUG << "error_code = " << ec;
                            BMCWEB_LOG_DEBUG << "error msg = " << ec.message();
                            messages::internalError(aResp->res);
                            return;
                        }
                        // Example objInfo
                        // a{sas} 1 "org.open_power.Software.Host.Updater" 10
                        // "org.freedesktop.DBus.Introspectable"
                        // "org.freedesktop.DBus.Peer"
                        // "org.freedesktop.DBus.Properties"
                        // "org.openbmc.Associations"
                        // "xyz.openbmc_project.Common.FilePath"
                        // "xyz.openbmc_project.Object.Delete"
                        // "xyz.openbmc_project.Software.Activation"
                        // "xyz.openbmc_project.Software.ExtendedVersion"
                        // "xyz.openbmc_project.Software.RedundancyPriority"
                        // "xyz.openbmc_project.Software.Version"

                        // Ensure we only got one service back
                        if (objInfo.size() != 1)
                        {
                            BMCWEB_LOG_ERROR << "Invalid Object Size "
                                             << objInfo.size();
                            messages::internalError(aResp->res);
                            return;
                        }

                        // Now grab its version info
                        crow::connections::systemBus->async_method_call(
                            [aResp, swId, fwVersionPurpose, jsonIdxStr](
                                const boost::system::error_code ec,
                                const boost::container::flat_map<
                                    std::string, VariantType> &propertiesList) {
                                if (ec)
                                {
                                    BMCWEB_LOG_ERROR << "error_code = " << ec;
                                    BMCWEB_LOG_ERROR << "error msg = "
                                                     << ec.message();
                                    messages::internalError(aResp->res);
                                    return;
                                }
                                // example propertiesList
                                // a{sv} 2 "Version" s
                                // "IBM-witherspoon-OP9-v2.0.10-2.22" "Purpose"
                                // s
                                // "xyz.openbmc_project.Software.Version.VersionPurpose.Host"

                                boost::container::flat_map<
                                    std::string, VariantType>::const_iterator
                                    it = propertiesList.find("Purpose");
                                if (it == propertiesList.end())
                                {
                                    BMCWEB_LOG_DEBUG
                                        << "Can't find property \"Purpose\"!";
                                    messages::internalError(aResp->res);
                                    return;
                                }
                                const std::string *swInvPurpose =
                                    std::get_if<std::string>(&it->second);
                                if (swInvPurpose == nullptr)
                                {
                                    BMCWEB_LOG_DEBUG << "wrong types for "
                                                        "property \"Purpose\"!";
                                    messages::internalError(aResp->res);
                                    return;
                                }

                                if (*swInvPurpose != fwVersionPurpose)
                                {
                                    // Not purpose we're looking for
                                    return;
                                }
                                it = propertiesList.find("Version");
                                if (it == propertiesList.end())
                                {
                                    BMCWEB_LOG_DEBUG
                                        << "Can't find property \"Version\"!";
                                    messages::internalError(aResp->res);
                                    return;
                                }
                                const std::string *version =
                                    std::get_if<std::string>(&it->second);
                                if (version == nullptr)
                                {
                                    BMCWEB_LOG_DEBUG
                                        << "Error getting fw version";
                                    messages::internalError(aResp->res);
                                    return;
                                }
                                aResp->res.jsonValue[jsonIdxStr] = *version;
                            },
                            objInfo[0].first, fw,
                            "org.freedesktop.DBus.Properties", "GetAll",
                            "xyz.openbmc_project.Software.Version");
                    },
                    "xyz.openbmc_project.ObjectMapper",
                    "/xyz/openbmc_project/object_mapper",
                    "xyz.openbmc_project.ObjectMapper", "GetObject", fw,
                    std::array<const char *, 1>{
                        "xyz.openbmc_project.Software.Activation"});
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/software/functional",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Association", "endpoints");

    return;
}
} // namespace fw_util
} // namespace redfish
