// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <span>
#include <string>
#include <vector>

inline std::string bytesToHexString(const std::span<const uint8_t>& bytes)
{
    std::string rc;
    rc.reserve(bytes.size() * 2);
    for (uint8_t byte : bytes)
    {
        rc += std::format("{:02X}", byte);
    }
    return rc;
}

// Returns nibble.
inline uint8_t hexCharToNibble(char ch)
{
    uint8_t rc = 16;
    if (ch >= '0' && ch <= '9')
    {
        rc = static_cast<uint8_t>(ch) - '0';
    }
    else if (ch >= 'A' && ch <= 'F')
    {
        rc = static_cast<uint8_t>(ch - 'A') + 10U;
    }
    else if (ch >= 'a' && ch <= 'f')
    {
        rc = static_cast<uint8_t>(ch - 'a') + 10U;
    }

    return rc;
}

// Returns empty vector in case of malformed hex-string.
inline std::vector<uint8_t> hexStringToBytes(const std::string& str)
{
    std::vector<uint8_t> rc(str.size() / 2, 0);
    for (size_t i = 0; i < str.length(); i += 2)
    {
        uint8_t hi = hexCharToNibble(str[i]);
        if (i == str.length() - 1)
        {
            rc.clear();
            break;
        }
        uint8_t lo = hexCharToNibble(str[i + 1]);
        if (lo == 16 || hi == 16)
        {
            rc.clear();
            break;
        }

        rc[i / 2] = static_cast<uint8_t>(hi << 4) | lo;
    }
    return rc;
}
