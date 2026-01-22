// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

// clang-format off

namespace outbound_connection
{
enum class OutboundConnectionRetryPolicyType{
    Invalid,
    None,
    RetryForever,
    RetryCount,
};

enum class AuthenticationType{
    Invalid,
    MTLS,
    JWT,
    None,
    OEM,
};

NLOHMANN_JSON_SERIALIZE_ENUM(OutboundConnectionRetryPolicyType, {
    {OutboundConnectionRetryPolicyType::Invalid, "Invalid"},
    {OutboundConnectionRetryPolicyType::None, "None"},
    {OutboundConnectionRetryPolicyType::RetryForever, "RetryForever"},
    {OutboundConnectionRetryPolicyType::RetryCount, "RetryCount"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(AuthenticationType, {
    {AuthenticationType::Invalid, "Invalid"},
    {AuthenticationType::MTLS, "MTLS"},
    {AuthenticationType::JWT, "JWT"},
    {AuthenticationType::None, "None"},
    {AuthenticationType::OEM, "OEM"},
});

// clang-format on
}
