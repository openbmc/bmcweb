#include "persistent_data.hpp"

#include <charconv>
#include <string>

// Generates a system-specific ID based on an initial uid.
inline std::string systemUniqueId(uint64_t init)
{
    std::string uuid = persistent_data::getConfig().systemUuid;
    uuid = uuid.substr(0, 8);
    uuid += "0x";
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const char* end = uuid.data() + uuid.size();
    uint64_t hash = 0;
    std::from_chars_result ret = std::from_chars(uuid.data(), end, hash);

    if (ret.ec != std::errc())
    {
        BMCWEB_LOG_CRITICAL << "UUID WASNT HEX?";
        return "";
    }
    // Hash combine with a random unique ID that represents the
    // first aggregaged resource.  Intentionally leave the last
    // few bits as 0 so that in theory we can have multiple
    // resource in the future by indexing
    hash ^= init;
    return std::to_string(hash);
}
