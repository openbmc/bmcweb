// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

// clang-format off

namespace certificate_enrollment
{
enum class EnrollmentProtocolType{
    Invalid,
    ACME,
    SCEP,
    OEM,
};

enum class LastOperationType{
    Invalid,
    Renew,
    UpdateAcmeEmail,
};

enum class OperationStatus{
    Invalid,
    Success,
    Failed,
    InProgress,
    Unknown,
};

enum class ACMEChallengeType{
    Invalid,
    Http01,
    Dns01,
};

NLOHMANN_JSON_SERIALIZE_ENUM(EnrollmentProtocolType, {
    {EnrollmentProtocolType::Invalid, "Invalid"},
    {EnrollmentProtocolType::ACME, "ACME"},
    {EnrollmentProtocolType::SCEP, "SCEP"},
    {EnrollmentProtocolType::OEM, "OEM"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(LastOperationType, {
    {LastOperationType::Invalid, "Invalid"},
    {LastOperationType::Renew, "Renew"},
    {LastOperationType::UpdateAcmeEmail, "UpdateAcmeEmail"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(OperationStatus, {
    {OperationStatus::Invalid, "Invalid"},
    {OperationStatus::Success, "Success"},
    {OperationStatus::Failed, "Failed"},
    {OperationStatus::InProgress, "InProgress"},
    {OperationStatus::Unknown, "Unknown"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ACMEChallengeType, {
    {ACMEChallengeType::Invalid, "Invalid"},
    {ACMEChallengeType::Http01, "Http01"},
    {ACMEChallengeType::Dns01, "Dns01"},
});

// clang-format on
}
