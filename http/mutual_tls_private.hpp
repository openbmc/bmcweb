// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "ossl_wrappers.hpp"

#include <string>
#include <string_view>

std::string getUPNFromCert(OpenSSLX509& peerCert, std::string_view hostname);

bool isUPNMatch(std::string_view upn, std::string_view hostname);
