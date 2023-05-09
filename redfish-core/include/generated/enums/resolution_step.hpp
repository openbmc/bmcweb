#pragma once
#include <boost/describe/enum.hpp>
#include <nlohmann/json.hpp>

namespace resolution_step
{
// clang-format off

enum class ResolutionType{
    Invalid,
    ContactVendor,
    ReplaceComponent,
    FirmwareUpdate,
    Reset,
    PowerCycle,
    ResetToDefaults,
    CollectDiagnosticData,
    OEM,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ResolutionType, {
    {ResolutionType::Invalid, "Invalid"},
    {ResolutionType::ContactVendor, "ContactVendor"},
    {ResolutionType::ReplaceComponent, "ReplaceComponent"},
    {ResolutionType::FirmwareUpdate, "FirmwareUpdate"},
    {ResolutionType::Reset, "Reset"},
    {ResolutionType::PowerCycle, "PowerCycle"},
    {ResolutionType::ResetToDefaults, "ResetToDefaults"},
    {ResolutionType::CollectDiagnosticData, "CollectDiagnosticData"},
    {ResolutionType::OEM, "OEM"},
});

BOOST_DESCRIBE_ENUM(ResolutionType,

    Invalid,
    ContactVendor,
    ReplaceComponent,
    FirmwareUpdate,
    Reset,
    PowerCycle,
    ResetToDefaults,
    CollectDiagnosticData,
    OEM,
);

}
// clang-format on
