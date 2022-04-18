#pragma once
#include <sdbusplus/asio/connection.hpp>

namespace crow
{
namespace connections
{
#ifndef MOCK_CONNECTION
static std::shared_ptr<sdbusplus::asio::connection> systemBus;
#else
static std::shared_ptr<mockConnection> systemBus;
#endif

} // namespace connections
} // namespace crow
