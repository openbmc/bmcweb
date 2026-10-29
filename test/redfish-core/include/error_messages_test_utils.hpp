// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#pragma once

#include <nlohmann/json.hpp>

namespace redfish
{

inline nlohmann::json internalErrorJson()
{
    using nlohmann::literals::operator""_json;
    return R"({
        "error": {
            "@Message.ExtendedInfo": [
                {
                    "@odata.type": "#Message.v1_1_1.Message",
                    "Message": "The request failed due to an internal service error.  The service is still operational.",
                    "MessageArgs": [],
                    "MessageId": "Base.1.19.InternalError",
                    "MessageSeverity": "Critical",
                    "Resolution": "Resubmit the request.  If the problem persists, consider resetting the service."
                }
            ],
            "code": "Base.1.19.InternalError",
            "message": "The request failed due to an internal service error.  The service is still operational."
        }
    })"_json;
}

} // namespace redfish
