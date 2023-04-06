#pragma once
#include "baserule.hpp"
#include "dynamicrule.hpp"
#include "ruleparametertraits.hpp"

#include <boost/beast/http/verb.hpp>

#include <memory>
#include <string>
#include <vector>

namespace crow
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
            black_magic::CallHelper<
                Func, black_magic::S<crow::Request,
                                     std::shared_ptr<bmcweb::AsyncResp>&,
                                     Args...>>::value,
            "Handler type is mismatched with URL parameters");
        static_assert(
            std::is_same<
                void,
                decltype(f(std::declval<crow::Request>(),
                           std::declval<std::shared_ptr<bmcweb::AsyncResp>&>(),
                           std::declval<Args>()...))>::value,
            "Handler function with response argument should have void return type");

        handler = std::forward<Func>(f);
    }

    void handle(const Request& req,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const std::vector<std::string>& params) override
    {
        detail::routing_handler_call_helper::Call<
            detail::routing_handler_call_helper::CallParams<decltype(handler)>,
            0, black_magic::S<Args...>, black_magic::S<>>()(
            detail::routing_handler_call_helper::CallParams<decltype(handler)>{
                handler, params, req, asyncResp});
    }

  private:
    std::function<void(const crow::Request&,
                       const std::shared_ptr<bmcweb::AsyncResp>&, Args...)>
        handler;
};
} // namespace crow
