// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "sessions.hpp"

#include <boost/asio/ip/address.hpp>
#include <boost/asio/ssl/verify_context.hpp>

#include <memory>

std::shared_ptr<persistent_data::UserSession> verifyMtlsUser(
    const boost::asio::ip::address& clientIp,
    boost::asio::ssl::verify_context& ctx);
