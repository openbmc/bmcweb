// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "sessions.hpp"

#include <boost/asio/ip/address.hpp>

#include <memory>

struct ssl_st;
using SSL = ssl_st;

std::shared_ptr<persistent_data::UserSession> verifyMtlsUser(
    const boost::asio::ip::address& clientIp, SSL* ssl);
