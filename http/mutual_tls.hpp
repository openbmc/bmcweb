// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "persistent_data.hpp"

#include <boost/asio/ip/address.hpp>
#include <boost/asio/ssl/verify_context.hpp>

#include <memory>
#include <string_view>

std::string getUsernameFromCommonName(std::string_view commonName);

std::shared_ptr<persistent_data::UserSession>
    verifyMtlsUser(const boost::asio::ip::address& clientIp,
                   boost::asio::ssl::verify_context& ctx);
