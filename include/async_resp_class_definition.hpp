#pragma once
#include <functional>
#include "../http/http_response_class_definition.hpp"

namespace bmcweb
{

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
