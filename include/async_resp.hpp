#pragma once

#include "http_response_class_decl.hpp"

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
    AsyncResp(crow::Response& response);

    AsyncResp(crow::Response& response, std::function<void()>&& function);

    AsyncResp(const AsyncResp&) = delete;
    AsyncResp(AsyncResp&&) = delete;

    ~AsyncResp();

    crow::Response& res;
    std::function<void()> func;
};

} // namespace bmcweb
