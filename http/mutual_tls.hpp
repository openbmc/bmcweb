// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "sessions.hpp"

#include <openssl/crypto.h>

#include <boost/asio/ip/address.hpp>
#include <boost/asio/ssl/verify_context.hpp>

#include <memory>
#include <string>
#include <string_view>

std::string getCommonNameFromCert(X509* cert);

std::string getUPNFromCert(X509* peerCert, std::string_view hostname);

std::string getMetaUserNameFromCert(X509* cert);

std::string getUsernameFromCert(X509* cert);

bool isUPNMatch(std::string_view upn, std::string_view hostname);

std::shared_ptr<persistent_data::UserSession> verifyMtlsUser(
    const boost::asio::ip::address& clientIp,
    boost::asio::ssl::verify_context& ctx);
