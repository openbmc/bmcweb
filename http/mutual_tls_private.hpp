// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include <openssl/crypto.h>

#include <string>
#include <string_view>

std::string getCommonNameFromCert(X509* cert);

std::string getUPNFromCert(X509* peerCert, std::string_view hostname);

std::string getMetaUserNameFromCert(X509* cert);

std::string getUsernameFromCert(X509* cert);

bool isUPNMatch(std::string_view upn, std::string_view hostname);
