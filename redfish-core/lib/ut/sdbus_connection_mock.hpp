#pragma once

#include <string>

#include "gmock/gmock.h"

using handlerEcDumpId_t = std::function<void(const boost::system::error_code ec,
                                             const uint32_t& dumpId)>&&;
using handlerEcResp_t =
    std::function<void(const boost::system::error_code ec,
                       dbus::utility::ManagedObjectType& resp)>&&;

class MockSdbusConnection
{
  public:
    MOCK_METHOD(void, async_method_call,
                (handlerEcResp_t handler, const std::string& service,
                 const std::string& objpath, const std::string& interf,
                 const std::string& method),
                ());

    MOCK_METHOD(void, async_method_call,
                (handlerEcDumpId_t handler, const std::string& service,
                 const std::string& objpath, const std::string& interf,
                 const std::string& method),
                ());
};
