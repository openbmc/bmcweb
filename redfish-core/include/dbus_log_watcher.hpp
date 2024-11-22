#pragma once

#include <sdbusplus/message.hpp>
namespace redfish
{

void getReadingsForReport(sdbusplus::message_t& msg);

} // namespace redfish
