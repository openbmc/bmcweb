// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include <nlohmann/json.hpp>

#include <format>

// Clang-tidy would rather these be static, but using static causes the template
// specialization to not function.  Ignore the warning.
// NOLINTBEGIN(readability-convert-member-functions-to-static, cert-dcl58-cpp)

template <>
struct std::formatter<nlohmann::json::json_pointer>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }
    auto format(const nlohmann::json::json_pointer& ptr, auto& ctx) const
    {
        return std::format_to(ctx.out(), "{}", ptr.to_string());
    }
};

template <>
struct std::formatter<nlohmann::json>
{
    static constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }
    auto format(const nlohmann::json& json, auto& ctx) const
    {
        return std::format_to(
            ctx.out(), "{}",
            json.dump(-1, ' ', false,
                      nlohmann::json::error_handler_t::replace));
    }
};
// NOLINTEND(readability-convert-member-functions-to-static, cert-dcl58-cpp)
