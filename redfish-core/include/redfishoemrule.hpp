#pragma once
#include "async_resp.hpp"
#include "http_request.hpp"
#include "routing/baserule.hpp"

#include <nlohmann/json.hpp>

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace redfish
{
class OemBaseRule : public crow::BaseRule
{
  public:
    explicit OemBaseRule(std::string_view thisRule) : BaseRule(thisRule) {}
    ~OemBaseRule() override = default;
    OemBaseRule(const OemBaseRule&) = delete;
    OemBaseRule(OemBaseRule&&) = delete;
    OemBaseRule& operator=(const OemBaseRule&) = delete;
    OemBaseRule& operator=(const OemBaseRule&&) = delete;

    void handle(const crow::Request& /*req*/,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const std::vector<std::string>& /*params*/) override = 0;
};

template <typename... Args>
class OemRule : public OemBaseRule
{
  public:
    using self_t = OemRule<Args...>;

    explicit OemRule(std::string_view ruleIn) : OemBaseRule(ruleIn) {}

    void validate() override
    {
        if (!handler)
        {
            throw std::runtime_error(
                "no OEM fragment handler for the rule {}" + rule);
        }
    }

    template <typename Func>
    void operator()(Func&& f)
    {
        static_assert(
            std::is_invocable_v<Func, crow::Request,
                                std::shared_ptr<bmcweb::AsyncResp>&, Args...>,
            "Handler type is mismatched with URL parameters");
        static_assert(
            std::is_same_v<
                void, std::invoke_result_t<Func, crow::Request,
                                           std::shared_ptr<bmcweb::AsyncResp>&,
                                           Args...>>,
            "Handler function with response argument should have void return type");

        handler = std::forward<Func>(f);
    }

    void handle(const crow::Request& req,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const std::vector<std::string>& params) override
    {
        if constexpr (sizeof...(Args) == 0)
        {
            handler(req, asyncResp);
        }
        else if constexpr (sizeof...(Args) == 1)
        {
            handler(req, asyncResp, params[0]);
        }
        else if constexpr (sizeof...(Args) == 2)
        {
            handler(req, asyncResp, params[0], params[1]);
        }
        else if constexpr (sizeof...(Args) == 3)
        {
            handler(req, asyncResp, params[0], params[1], params[2]);
        }
        else if constexpr (sizeof...(Args) == 4)
        {
            handler(req, asyncResp, params[0], params[1], params[2], params[3]);
        }
        else if constexpr (sizeof...(Args) == 5)
        {
            handler(req, asyncResp, params[0], params[1], params[2], params[3],
                    params[4]);
        }
        static_assert(sizeof...(Args) <= 5, "More args than are supported");
    }

  private:
    std::function<void(const crow::Request&,
                       const std::shared_ptr<bmcweb::AsyncResp>&, Args...)>
        handler;
};
} // namespace redfish
