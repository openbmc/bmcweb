#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "logging.hpp"
#include "utility.hpp"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <dbus_singleton.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <cstddef>
#include <string>

namespace redfish
{
namespace dump_utils
{

inline void getValidDumpEntryForAttachment(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, const std::string& url,
    std::function<void(const std::string& objectPath,
                       const std::string& /*entryID*/,
                       const std::string& /*dumpType*/)>&& callback)
{
    std::string dumpType;
    std::string dumpId;
    std::string entryID;
    std::string entriesPath;

    if (crow::utility::readUrlSegments(url, "redfish", "v1", "Managers", "bmc",
                                       "LogServices", "Dump", "Entries",
                                       std::ref(entryID), "attachment"))
    {
        // BMC type dump
        dumpType = "BMC";
        entriesPath = "/redfish/v1/Managers/bmc/LogServices/Dump/Entries/";
        dumpId = entryID;
    }
    else if (crow::utility::readUrlSegments(
                 url, "redfish", "v1", "Systems", "system", "LogServices",
                 "Dump", "Entries", std::ref(entryID), "attachment"))
    {
        entriesPath = "/redfish/v1/Systems/system/LogServices/Dump/Entries/";

        // All these types system,resource,sbe,hwdump and host boot dumps are
        // currently being listed under
        // /Systems/system/LogServices/Dump/Entries/ redfish path. To
        // differentiate between the two, the dump entries would be listed as
        // System_<id> and Resource_<id> for the respective dumps. Hence the
        // dump id and type are being extracted here from the above format.

        std::size_t pos = entryID.find_first_of('_');
        if (pos == std::string::npos || (pos + 1) >= entryID.length())
        {
            // Requested ID is invalid
            messages::invalidObject(
                asyncResp->res,
                boost::urls::format(
                    "/redfish/v1/Systems/system/LogServices/Dump/Entries/{}",
                    entryID));
            return;
        }
        dumpType = boost::algorithm::to_lower_copy(entryID.substr(0, pos));
        dumpId = entryID.substr(pos + 1);
    }

    if (dumpType.empty() || entryID.empty())
    {
        // Besides of system,resource,sbe,hwdump and host boot dumps
        redfish::messages::resourceNotFound(asyncResp->res, "Dump", entryID);
        return;
    }

    auto getValidDumpEntryCallback =
        [asyncResp, entryID, dumpType, dumpId, entriesPath,
         callback](const boost::system::error_code& ec,
                   const dbus::utility::ManagedObjectType& resp) {
        if (ec.value() == EBADR)
        {
            messages::resourceNotFound(asyncResp->res, dumpType + " dump",
                                       entryID);
            return;
        }
        if (ec)
        {
            BMCWEB_LOG_ERROR("DumpEntry resp_handler got error {}", ec);
            messages::internalError(asyncResp->res);
            return;
        }

        std::string dumpEntryIdPath =
            "/xyz/openbmc_project/dump/" +
            std::string(boost::algorithm::to_lower_copy(dumpType)) + "/entry/" +
            dumpId;

        for (const auto& objectPath : resp)
        {
            if (objectPath.first.str == dumpEntryIdPath)
            {
                callback(dumpEntryIdPath, entryID, dumpType);
                return;
            }
        }
        BMCWEB_LOG_WARNING("Dump entry {} is not found", entryID);
        messages::resourceNotFound(asyncResp->res, dumpType + " dump", entryID);
    };

    dbus::utility::getManagedObjects(
        "xyz.openbmc_project.Dump.Manager",
        sdbusplus::message::object_path("/xyz/openbmc_project/dump"),
        std::move(getValidDumpEntryCallback));
}

} // namespace dump_utils
} // namespace redfish
