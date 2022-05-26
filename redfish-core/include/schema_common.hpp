#pragma once

struct SchemaVersion
{
    const char* name;
    const char* version;

    std::string toString() const
    {
        std::string ret;
        ret += "#";
        ret += name;
        ret += ".";
        if (version != std::string_view(""))
        {
            ret += version;
            ret += ".";
        }
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