#pragma once
#include <boost/describe/enum.hpp>
#include <nlohmann/json.hpp>

namespace event
{
// clang-format off

enum class EventType{
    Invalid,
    StatusChange,
    ResourceUpdated,
    ResourceAdded,
    ResourceRemoved,
    Alert,
    MetricReport,
    Other,
};

enum class DiagnosticDataTypes{
    Invalid,
    Manager,
    PreOS,
    OS,
    OEM,
    CPER,
    CPERSection,
};

NLOHMANN_JSON_SERIALIZE_ENUM(EventType, {
    {EventType::Invalid, "Invalid"},
    {EventType::StatusChange, "StatusChange"},
    {EventType::ResourceUpdated, "ResourceUpdated"},
    {EventType::ResourceAdded, "ResourceAdded"},
    {EventType::ResourceRemoved, "ResourceRemoved"},
    {EventType::Alert, "Alert"},
    {EventType::MetricReport, "MetricReport"},
    {EventType::Other, "Other"},
});

BOOST_DESCRIBE_ENUM(EventType,

    Invalid,
    StatusChange,
    ResourceUpdated,
    ResourceAdded,
    ResourceRemoved,
    Alert,
    MetricReport,
    Other,
);

NLOHMANN_JSON_SERIALIZE_ENUM(DiagnosticDataTypes, {
    {DiagnosticDataTypes::Invalid, "Invalid"},
    {DiagnosticDataTypes::Manager, "Manager"},
    {DiagnosticDataTypes::PreOS, "PreOS"},
    {DiagnosticDataTypes::OS, "OS"},
    {DiagnosticDataTypes::OEM, "OEM"},
    {DiagnosticDataTypes::CPER, "CPER"},
    {DiagnosticDataTypes::CPERSection, "CPERSection"},
});

BOOST_DESCRIBE_ENUM(DiagnosticDataTypes,

    Invalid,
    Manager,
    PreOS,
    OS,
    OEM,
    CPER,
    CPERSection,
);

}
// clang-format on
