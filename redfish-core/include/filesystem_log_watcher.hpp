#pragma once

#include <boost/asio/io_context.hpp>

namespace redfish
{
int startEventLogMonitor(boost::asio::io_context& ioc);
void stopEventLogMonitor();
} // namespace redfish
