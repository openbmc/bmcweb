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
 * @param[i]   aResp             Async response object
 * @param[i]   fwVersionPurpose  Indicates what target to look for
 * @param[o]   jsonIdxStr        Index in aResp->res.jsonValue to write fw ver
 *
 * @return void
 */
void getActiveFwVersion(std::shared_ptr<AsyncResp> aResp,
                        const std::string &fwVersionPurpose,
                        const std::string &jsonIdxStr)
{
    // First retrieve everything under software path
    crow::connections::systemBus->async_method_call(
        [aResp, fwVersionPurpose, jsonIdxStr](
            const boost::system::error_code ec,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>
                &subtree) {
            if (ec)
            {
                messages::internalError(aResp->res);
                return;
            }
            for (auto &obj : subtree)
            {
                const std::vector<
                    std::pair<std::string, std::vector<std::string>>>
                    &connections = obj.second;

                // if can't parse fw id then return
                std::size_t idPos;
                if ((idPos = obj.first.rfind("/")) == std::string::npos)
                {
                    messages::internalError(aResp->res);
                    BMCWEB_LOG_DEBUG << "Can't parse firmware ID!!";
                    return;
                }
                std::string swId = obj.first.substr(idPos + 1);
                crow::connections::systemBus->async_method_call(
                    [aResp, swId, fwVersionPurpose, jsonIdxStr](
                        const boost::system::error_code error_code,
                        const boost::container::flat_map<
                            std::string, VariantType> &propertiesList) {
                        if (error_code)
                        {
                            messages::internalError(aResp->res);
                            return;
                        }
                        boost::container::flat_map<
                            std::string, VariantType>::const_iterator it =
                            propertiesList.find("Purpose");
                        if (it == propertiesList.end())
                        {
                            BMCWEB_LOG_DEBUG
                                << "Can't find property \"Purpose\"!";
                            messages::propertyMissing(aResp->res, "Purpose");
                            return;
                        }
                        const std::string *swInvPurpose =
                            std::get_if<std::string>(&it->second);
                        if (swInvPurpose == nullptr)
                        {
                            BMCWEB_LOG_DEBUG
                                << "wrong types for property\"Purpose\"!";
                            messages::propertyValueTypeError(aResp->res, "",
                                                             "Purpose");
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
                            messages::propertyMissing(aResp->res, "Version");
                            return;
                        }
                        const std::string *version =
                            std::get_if<std::string>(&it->second);
                        if (version == nullptr)
                        {
                            BMCWEB_LOG_DEBUG << "Error getting fw version";
                            messages::propertyMissing(aResp->res, "Version");
                            return;
                        }

                        aResp->res.jsonValue[jsonIdxStr] = *version;
                    },
                    obj.second[0].first, obj.first,
                    "org.freedesktop.DBus.Properties", "GetAll",
                    "xyz.openbmc_project.Software.Version");
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/software", int32_t(1),
        std::array<const char *, 1>{"xyz.openbmc_project.Software.Version"});

    return;
}
} // namespace fw_util
} // namespace redfish
