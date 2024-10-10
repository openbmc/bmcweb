#pragma once
#include "baserule.hpp"
#include "dynamicrule.hpp"
#include "ruleparametertraits.hpp"

#include <boost/beast/http/verb.hpp>

#include <memory>
#include <string>
#include <vector>

namespace bmcweb
{
template <typename... Args>
class TaggedRule :
    public BaseRule,
    public RuleParameterTraits<TaggedRule<Args...>>
{
  public:
    using self_t = TaggedRule<Args...>;

    explicit TaggedRule(const std::string& ruleIn) : BaseRule(ruleIn) {}

    void validate() override
    {
        if (!handler)
        {
            throw std::runtime_error("no handler for url " + rule);
        }
    }

    template <typename Func>
    void operator()(Func&& f)
    {
        static_assert(
            std::is_invocable_v<Func, Request,
                                std::shared_ptr<bmcweb::AsyncResp>&, Args...>,
            "Handler type is mismatched with URL parameters");
        static_assert(
            std::is_same_v<
                void, std::invoke_result_t<Func, Request,
                                           std::shared_ptr<bmcweb::AsyncResp>&,
                                           Args...>>,
            "Handler function with response argument should have void return type");

        handler = std::forward<Func>(f);
    }

    void handle(const Request& req,
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
    std::function<void(const Request&,
                       const std::shared_ptr<bmcweb::AsyncResp>&, Args...)>
        handler;
};
} // namespace bmcweb

namespace crow = bmcweb;
