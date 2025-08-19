// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include <openssl/x509.h>

#include <string>
#include <string_view>

std::string getUPNFromCert(X509* peerCert, std::string_view hostname);

bool isUPNMatch(std::string_view upn, std::string_view hostname);
