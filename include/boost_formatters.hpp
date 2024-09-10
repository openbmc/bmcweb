// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include <boost/system/error_code.hpp>
#include <boost/url/grammar.hpp>
#include <boost/url/url_view_base.hpp>

#include <format>

// NOLINTBEGIN(readability-convert-member-functions-to-static, cert-dcl58-cpp)
template <std::derived_from<boost::system::error_code> ErrorCodeType>
struct std::formatter<ErrorCodeType>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(const ErrorCodeType& ec, auto& ctx) const
    {
        return std::format_to(ctx.out(), "{}", ec.what());
    }
};

template <std::derived_from<boost::urls::grammar::string_view_base> StringView>
struct std::formatter<StringView>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }
    auto format(const StringView& msg, auto& ctx) const
    {
        return std::format_to(ctx.out(), "{}",
                              std::string_view(msg.data(), msg.size()));
    }
};

template <std::derived_from<boost::urls::url_view_base> UrlBase>
struct std::formatter<UrlBase>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }
    auto format(const UrlBase& msg, auto& ctx) const
    {
        return std::format_to(ctx.out(), "{}", std::string_view(msg.buffer()));
    }
};

template <std::derived_from<boost::core::string_view> StringView>
struct std::formatter<StringView>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }
    auto format(const StringView& msg, auto& ctx) const
    {
        return std::format_to(ctx.out(), "{}", std::string_view(msg));
    }
};
// NOLINTEND(readability-convert-member-functions-to-static, cert-dcl58-cpp)
