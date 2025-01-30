// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include <boost/asio/io_context.hpp>

inline boost::asio::io_context& getIoContext()
{
    static boost::asio::io_context io;
    return io;
}
