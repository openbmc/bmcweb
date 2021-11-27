#pragma once

#include "http_response_class_decl.hpp"

namespace redfish
{

/**
 * AsyncResp
 * Gathers data needed for response processing after async calls are done
 */
class AsyncResp
{
  public:
    AsyncResp(crow::Response& response) : res(response)
    {}

    AsyncResp(const AsyncResp&) = delete;
    AsyncResp(AsyncResp&&) = delete;

    ~AsyncResp()
    {
        res.end();
    }

    crow::Response& res;
};

} // namespace redfish
