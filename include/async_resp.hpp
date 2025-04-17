// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "http_response.hpp"

namespace bmcweb
{

/**
This was a structure that forerly housed an RAII compatible response container.
The logic has since been moved into the Response object itself, but keep it
here for existing code that uses it.
*/

using AsyncResp = crow::Response;
} // namespace bmcweb
