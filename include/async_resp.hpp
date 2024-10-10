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
    explicit AsyncResp(bmcweb::Response&& resIn) : res(std::move(resIn)) {}

    AsyncResp(const AsyncResp&) = delete;
    AsyncResp(AsyncResp&&) = delete;
    AsyncResp& operator=(const AsyncResp&) = delete;
    AsyncResp& operator=(AsyncResp&&) = delete;

    ~AsyncResp()
    {
        res.end();
    }

    bmcweb::Response res;
};

} // namespace bmcweb
