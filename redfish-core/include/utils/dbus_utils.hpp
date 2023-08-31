#pragma once

#include "logging.hpp"
#include "nlohmann/json.hpp"

#include <sdbusplus/unpack_properties.hpp>

#include <climits>

using json = nlohmann::json;

using valueType = std::variant<uint8_t, uint16_t, uint32_t, uint64_t, int16_t,
                               int32_t, int64_t, bool, double, std::string,
                               decltype(nlohmann::json::value_t::null)>;

// Dbus data types
enum property_type
{
    IsInt16,
    IsInt32,
    IsInt64,
    IsUInt16,
    IsUInt32,
    IsUInt64,
    IsDouble,
    IsBool,
    IsString
};

using myvariant = std::variant<uint8_t, uint16_t, uint32_t, uint64_t, int16_t,
                               int32_t, int64_t, double, std::string, bool>;

class mydatatype_t
{

  public:
    myvariant value_t;
    property_type valueType;

    mydatatype_t()
    {}

    mydatatype_t(uint8_t a)
    {
        value_t = a;
        valueType = IsBool;
    }
    mydatatype_t(uint16_t a)
    {
        value_t = a;
        valueType = IsUInt16;
    }
    mydatatype_t(uint32_t a)
    {
        value_t = a;
        valueType = IsUInt32;
    }
    mydatatype_t(uint64_t a)
    {
        value_t = a;
        valueType = IsUInt64;
    }
    mydatatype_t(int16_t a)
    {
        value_t = a;
        valueType = IsInt16;
    }
    mydatatype_t(int32_t a)
    {
        value_t = a;
        valueType = IsInt32;
    }
    mydatatype_t(int64_t a)
    {
        value_t = a;
        valueType = IsInt64;
    }
    mydatatype_t(double a)
    {
        value_t = a;
        valueType = IsDouble;
    }
    mydatatype_t(const std::string a)
    {
        value_t = a;
        valueType = IsString;
    }
};

void to_json(json& j, const mydatatype_t& p)
{
    std::regex pattern("BE_ERROR.*");

    auto propVal = p.value_t;
    auto type = p.valueType;

    switch (type)
    {
        case IsInt16:
        {
            auto int16Value = std::get<int16_t>(propVal);
            if (int16Value != SHRT_MAX)
                j = int16Value;
            break;
        }
        case IsInt32:
        {
            auto int32Value = std::get<int32_t>(propVal);
            if (int32Value != LONG_MAX)
                j = int32Value;
            break;
        }
        case IsInt64:
        {
            auto int64Value = std::get<int64_t>(propVal);
            if (int64Value != LLONG_MAX)
                j = int64Value;
            break;
        }

        case IsUInt16:
        {
            auto uint16Value = std::get<uint16_t>(propVal);
            if (uint16Value != USHRT_MAX)
                j = uint16Value;
            break;
        }
        case IsUInt32:
        {
            auto uint32Value = std::get<uint32_t>(propVal);
            if (uint32Value != ULONG_MAX)
                j = uint32Value;
            break;
        }
        case IsUInt64:
        {
            auto uint64Value = std::get<uint64_t>(propVal);
            if (uint64Value != ULLONG_MAX)
                j = uint64Value;
            break;
        }
        case IsDouble:
        {
            auto doubleValue = std::get<double>(propVal);
            if (!std::isnan(doubleValue))
                j = doubleValue;
            break;
        }
        case IsString:
        {
            auto stringValue = std::get<std::string>(propVal);
            if (!std::regex_match(stringValue, pattern))
                j = stringValue;
            break;
        }
        case IsBool:
        {
            auto boolValue = std::get<uint8_t>(propVal);
            if (boolValue == 0)
                j = false;
            else if (boolValue == 1)
                j = true;
            break;
        }
    }
}

namespace redfish
{
namespace dbus_utils
{

struct UnpackErrorPrinter
{
    void operator()(const sdbusplus::UnpackErrorReason reason,
                    const std::string& property) const noexcept
    {
        BMCWEB_LOG_ERROR(
            "DBUS property error in property: {}, reason: {}", property,
            static_cast<std::underlying_type_t<sdbusplus::UnpackErrorReason>>(
                reason));
    }
};

} // namespace dbus_utils
} // namespace redfish
