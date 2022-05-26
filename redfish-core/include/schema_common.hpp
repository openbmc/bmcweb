#pragma once

struct SchemaVersion
{
    const std::string_view name;
    uint8_t versionMajor;
    uint8_t versionMinor;
    uint8_t versionPatch;
    bool includeInMetadata;

    std::string namespaceString() const
    {
        std::string ret;
        ret += name;
        // All zero versions mean the schema is unversioned
        if (versionMajor != 0 || versionMinor != 0 || versionPatch != 0)
        {
            ret += '.';
            ret += 'v';
            ret += std::to_string(versionMajor);
            ret += '_';
            ret += std::to_string(versionMinor);
            ret += '_';
            ret += std::to_string(versionPatch);
        }
        return ret;
    }

    std::string toString() const
    {
        std::string ret;
        ret += '#';
        ret += namespaceString();
        ret += '.';
        ret += name;
        return ret;
    }
};

namespace nlohmann
{
template <>
struct adl_serializer<SchemaVersion>
{
    // nlohmann requires a specific casing to look these up in adl
    // NOLINTNEXTLINE(readability-identifier-naming)
    static void to_json(json& j, const SchemaVersion& schema)
    {
        j = schema.toString();
    }
};
} // namespace nlohmann
