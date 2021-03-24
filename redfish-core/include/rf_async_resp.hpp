#pragma once

#include "http_response.hpp"

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

    ~AsyncResp()
    {
        res.end();
    }

    crow::Response& res;
};

} // namespace redfish