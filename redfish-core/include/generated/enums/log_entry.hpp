#pragma once
#include <nlohmann/json.hpp>

namespace log_entry
{
// clang-format off

enum class EventSeverity{
    Invalid,
    OK,
    Warning,
    Critical,
};

enum class LogEntryType{
    Invalid,
    Event,
    SEL,
    Oem,
};

enum class LogDiagnosticDataTypes{
    Invalid,
    Manager,
    PreOS,
    OS,
    OEM,
    CPER,
    CPERSection,
};

enum class OriginatorTypes{
    Invalid,
    Client,
    Internal,
    SupportingService,
};

NLOHMANN_JSON_SERIALIZE_ENUM(EventSeverity, {
    {EventSeverity::Invalid, "Invalid"},
    {EventSeverity::OK, "OK"},
    {EventSeverity::Warning, "Warning"},
    {EventSeverity::Critical, "Critical"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(LogEntryType, {
    {LogEntryType::Invalid, "Invalid"},
    {LogEntryType::Event, "Event"},
    {LogEntryType::SEL, "SEL"},
    {LogEntryType::Oem, "Oem"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(LogDiagnosticDataTypes, {
    {LogDiagnosticDataTypes::Invalid, "Invalid"},
    {LogDiagnosticDataTypes::Manager, "Manager"},
    {LogDiagnosticDataTypes::PreOS, "PreOS"},
    {LogDiagnosticDataTypes::OS, "OS"},
    {LogDiagnosticDataTypes::OEM, "OEM"},
    {LogDiagnosticDataTypes::CPER, "CPER"},
    {LogDiagnosticDataTypes::CPERSection, "CPERSection"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(OriginatorTypes, {
    {OriginatorTypes::Invalid, "Invalid"},
    {OriginatorTypes::Client, "Client"},
    {OriginatorTypes::Internal, "Internal"},
    {OriginatorTypes::SupportingService, "SupportingService"},
});

}
// clang-format on
