#pragma once

#include <logging.hpp>

#include <filesystem>
#include <fstream>

namespace crow
{
namespace ibm_utils
{

inline bool createDirectory(const std::string_view path)
{
    // Create persistent directory
    std::error_code ec;

    BMCWEB_LOG_DEBUG << "Creating persistent directory : " << path;

    if (!std::filesystem::is_directory(path, ec))
    {
        std::filesystem::create_directories(path, ec);
        // set the permission of the directory to 700
        std::filesystem::perms permission = std::filesystem::perms::owner_all;
        std::filesystem::permissions(path, permission);
    }
    if (ec)
    {
        BMCWEB_LOG_DEBUG << "Failed to prepare persistent directory. ec : "
                         << ec;
        return false;
    }
    return true;
}

} // namespace ibm_utils
} // namespace crow
