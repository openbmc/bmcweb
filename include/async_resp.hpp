#pragma once

#include "http_response.hpp"

#include <functional>

namespace bmcweb
{

/**
 * AsyncResp
 * Gathers data needed for response processing after async calls are done
 */

class AsyncResp
{
  public:
    AsyncResp() = default;

    AsyncResp(const AsyncResp&) = delete;
    AsyncResp(AsyncResp&&) = delete;

    ~AsyncResp()
    {
        res.end();
    }

    crow::Response res;
};

} // namespace bmcweb
