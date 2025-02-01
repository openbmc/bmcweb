// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "error_message_utils.hpp"
#include "http_response.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>

namespace redfish
{
// Propagates the worst error code to the final response.
// The order of error code is (from high to low)
// 500 Internal Server Error
// 511 Network Authentication Required
// 510 Not Extended
// 508 Loop Detected
// 507 Insufficient Storage
// 506 Variant Also Negotiates
// 505 HTTP Version Not Supported
// 504 Gateway Timeout
// 503 Service Unavailable
// 502 Bad Gateway
// 501 Not Implemented
// 401 Unauthorized
// 451 - 409 Error codes (not listed explicitly)
// 408 Request Timeout
// 407 Proxy Authentication Required
// 406 Not Acceptable
// 405 Method Not Allowed
// 404 Not Found
// 403 Forbidden
// 402 Payment Required
// 400 Bad Request
inline unsigned propogateErrorCode(unsigned finalCode, unsigned subResponseCode)
{
    // We keep a explicit list for error codes that this project often uses
    // Higher priority codes are in lower indexes
    constexpr std::array<unsigned, 13> orderedCodes = {
        500, 507, 503, 502, 501, 401, 412, 409, 406, 405, 404, 403, 400};
    size_t finalCodeIndex = std::numeric_limits<size_t>::max();
    size_t subResponseCodeIndex = std::numeric_limits<size_t>::max();
    for (size_t i = 0; i < orderedCodes.size(); ++i)
    {
        if (orderedCodes[i] == finalCode)
        {
            finalCodeIndex = i;
        }
        if (orderedCodes[i] == subResponseCode)
        {
            subResponseCodeIndex = i;
        }
    }
    if (finalCodeIndex != std::numeric_limits<size_t>::max() &&
        subResponseCodeIndex != std::numeric_limits<size_t>::max())
    {
        return finalCodeIndex <= subResponseCodeIndex
                   ? finalCode
                   : subResponseCode;
    }
    if (subResponseCode == 500 || finalCode == 500)
    {
        return 500;
    }
    if (subResponseCode > 500 || finalCode > 500)
    {
        return std::max(finalCode, subResponseCode);
    }
    if (subResponseCode == 401)
    {
        return subResponseCode;
    }
    return std::max(finalCode, subResponseCode);
}

// Propagates all error messages into |finalResponse|
inline void propogateError(crow::Response& finalResponse,
                           crow::Response& subResponse)
{
    // no errors
    if (subResponse.resultInt() >= 200 && subResponse.resultInt() < 400)
    {
        return;
    }
    messages::moveErrorsToErrorJson(finalResponse.jsonValue,
                                    subResponse.jsonValue);
    finalResponse.result(
        propogateErrorCode(finalResponse.resultInt(), subResponse.resultInt()));
}

} // namespace redfish
