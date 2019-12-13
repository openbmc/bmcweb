#pragma once

#include <app.h>

#include <boost/container/flat_map.hpp>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <openbmc_ibm_mc_rest.hpp>

namespace crow
{
namespace persistent_ibm_mc_lock
{
namespace fs = std::filesystem;
using stype = std::string;
using segmentflags = std::vector<std::pair<std::string, uint32_t>>;
using lockrecord = std::tuple<stype, stype, stype, uint64_t, segmentflags>;
using lockrequest = std::vector<lockrecord>;
using rc = std::pair<bool, std::variant<uint32_t, lockrecord>>;
using rcrelaselock = std::pair<bool, lockrecord>;
using rcgetlocklist = std::pair<
    bool,
    std::variant<std::string, std::vector<std::pair<uint32_t, lockrequest>>>>;

constexpr const char *internalServerError = "Internal Server Error";

bool createPersistentLockFilePath()
{
    // The path /var/lib/obmc will be created by initrdscripts
    // Create the directories for the persistent lock file
    std::error_code ec;
    if (!std::filesystem::is_directory("/var/lib/obmc/bmc-console-mgmt"))
    {
        std::filesystem::create_directory("/var/lib/obmc/bmc-console-mgmt", ec);
    }
    if (ec)
    {
        BMCWEB_LOG_DEBUG
            << "handleIbmPost: Failed to prepare bmc-console-mgmt directory. ec : "
            << ec;
        return false;
    }

    if (!std::filesystem::is_directory(
            "/var/lib/obmc/bmc-console-mgmt/locks"))
    {
        std::filesystem::create_directory(
            "/var/lib/obmc/bmc-console-mgmt/locks", ec);
    }
    if (ec)
    {
        BMCWEB_LOG_DEBUG
            << "handleIbmPost: Failed to prepare persistent lock file directory. ec : "
            << ec;
        return false;
    }
    return true;
}

class LockPersistence
{

public:
    static constexpr const char* filename = "/var/lib/obmc/bmc-console-mgmt/locks/ibm_mc_persistent_lock_data.json";
    LockPersistence()
    {
    }

    ~LockPersistence()
    {
    }

    static void loadLocks(std::map<uint32_t, lockrequest> &o_lockRecord)
    {
        std::ifstream persistentFile(filename);
        if (persistentFile.is_open())
        {
            auto data = nlohmann::json::parse(persistentFile, nullptr, false);
            if (data.is_discarded())
            {
                BMCWEB_LOG_ERROR
                    << "Error parsing persistent data in json file.";
            }
            else
            {
                BMCWEB_LOG_DEBUG << "The persistent lock data is available";
                for (const auto& item : data.items())
                {
                    BMCWEB_LOG_DEBUG << item.key();
                    BMCWEB_LOG_DEBUG << item.value();
                    lockrequest locks = item.value();
                    o_lockRecord.insert(std::pair<uint32_t,lockrequest>(std::stoul(item.key()),locks));
                    BMCWEB_LOG_DEBUG << "The persistent lock data loaded";
                }
            }
        }
    }

    static bool saveLocks(std::map<uint32_t, lockrequest> i_lockTable)
    {
        if (!std::filesystem::is_directory(
            "/var/lib/obmc/bmc-console-mgmt/locks"))
        {
            if(!createPersistentLockFilePath())
            {
                BMCWEB_LOG_DEBUG << "Failed to create lock persistent path";
                return false;
            }
        }
        std::ofstream persistentFile(filename);
        // set the permission of the file to 640
        fs::perms permission = fs::perms::owner_read | fs::perms::owner_write |
                               fs::perms::group_read;
        fs::permissions(filename, permission);
        nlohmann::json data;
        std::map<uint32_t, lockrequest>::iterator it ;
        for (it = i_lockTable.begin(); it!=i_lockTable.end(); ++it)
        {
            data[to_string(it->first)] = it->second;   
        }
        BMCWEB_LOG_DEBUG << "data is " << data;
        persistentFile << data;
        return true;
        
    }
};

} // namespace persistent_ibm_mc_lock
} // namespace crow
