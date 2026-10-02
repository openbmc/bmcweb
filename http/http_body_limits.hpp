// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include "bmcweb_config.h"

#include <cstdint>

namespace crow
{

// request body limit size set by the BMCWEB_HTTP_BODY_LIMIT option
constexpr uint64_t httpReqBodyLimit = 1024UL * 1024UL * BMCWEB_HTTP_BODY_LIMIT;

constexpr uint64_t loggedOutPostBodyLimit = 4096U;

} // namespace crow
