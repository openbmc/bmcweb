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
    AsyncResp(crow::Response& response) : res(response)
    {}

    AsyncResp(crow::Response& response, std::function<void()>&& function) :
        res(response), func(std::move(function))
    {}

    AsyncResp(const AsyncResp&) = delete;
    AsyncResp(AsyncResp&&) = delete;

    ~AsyncResp()
    {
        if (func)
        {
            func();
        }

        res.end();
    }

    crow::Response& res;
    std::function<void()> func;
};

} // namespace bmcweb
