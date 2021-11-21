#pragma once

#include <sessions.hpp>

namespace persistent_data
{

class ConfigFile
{
    uint64_t jsonRevision = 1;

  public:
    // todo(ed) should read this from a fixed location somewhere, not CWD
    static constexpr const char* filename = "bmcweb_persistent_data.json";

    ConfigFile()
    {
        readData();
    }

    ~ConfigFile()
    {
        // Make sure we aren't writing stale sessions
        persistent_data::SessionStore::getInstance().applySessionTimeouts();
        if (persistent_data::SessionStore::getInstance().needsWrite())
        {
            writeData();
        }
    }

    // TODO(ed) this should really use protobuf, or some other serialization
    // library, but adding another dependency is somewhat outside the scope of
    // this application for the moment
    void readData();
    void writeData();

    std::string systemUuid{""};
};

inline ConfigFile& getConfig()
{
    static ConfigFile f;
    return f;
}

} // namespace persistent_data
