#pragma once
#include <boost/describe/enum.hpp>
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
    CXL,
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

enum class CXLEntryType{
    Invalid,
    DynamicCapacity,
    Informational,
    Warning,
    Failure,
    Fatal,
};

NLOHMANN_JSON_SERIALIZE_ENUM(EventSeverity, {
    {EventSeverity::Invalid, "Invalid"},
    {EventSeverity::OK, "OK"},
    {EventSeverity::Warning, "Warning"},
    {EventSeverity::Critical, "Critical"},
});

BOOST_DESCRIBE_ENUM(EventSeverity,

    Invalid,
    OK,
    Warning,
    Critical,
);

NLOHMANN_JSON_SERIALIZE_ENUM(LogEntryType, {
    {LogEntryType::Invalid, "Invalid"},
    {LogEntryType::Event, "Event"},
    {LogEntryType::SEL, "SEL"},
    {LogEntryType::Oem, "Oem"},
    {LogEntryType::CXL, "CXL"},
});

BOOST_DESCRIBE_ENUM(LogEntryType,

    Invalid,
    Event,
    SEL,
    Oem,
    CXL,
);

NLOHMANN_JSON_SERIALIZE_ENUM(LogDiagnosticDataTypes, {
    {LogDiagnosticDataTypes::Invalid, "Invalid"},
    {LogDiagnosticDataTypes::Manager, "Manager"},
    {LogDiagnosticDataTypes::PreOS, "PreOS"},
    {LogDiagnosticDataTypes::OS, "OS"},
    {LogDiagnosticDataTypes::OEM, "OEM"},
    {LogDiagnosticDataTypes::CPER, "CPER"},
    {LogDiagnosticDataTypes::CPERSection, "CPERSection"},
});

BOOST_DESCRIBE_ENUM(LogDiagnosticDataTypes,

    Invalid,
    Manager,
    PreOS,
    OS,
    OEM,
    CPER,
    CPERSection,
);

NLOHMANN_JSON_SERIALIZE_ENUM(OriginatorTypes, {
    {OriginatorTypes::Invalid, "Invalid"},
    {OriginatorTypes::Client, "Client"},
    {OriginatorTypes::Internal, "Internal"},
    {OriginatorTypes::SupportingService, "SupportingService"},
});

BOOST_DESCRIBE_ENUM(OriginatorTypes,

    Invalid,
    Client,
    Internal,
    SupportingService,
);

NLOHMANN_JSON_SERIALIZE_ENUM(CXLEntryType, {
    {CXLEntryType::Invalid, "Invalid"},
    {CXLEntryType::DynamicCapacity, "DynamicCapacity"},
    {CXLEntryType::Informational, "Informational"},
    {CXLEntryType::Warning, "Warning"},
    {CXLEntryType::Failure, "Failure"},
    {CXLEntryType::Fatal, "Fatal"},
});

BOOST_DESCRIBE_ENUM(CXLEntryType,

    Invalid,
    DynamicCapacity,
    Informational,
    Warning,
    Failure,
    Fatal,
);

}
// clang-format on
