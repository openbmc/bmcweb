#pragma once
#include "baserule.hpp"
#include "ruleparametertraits.hpp"
#include "websocket.hpp"

#include <boost/beast/http/verb.hpp>

#include <functional>
#include <limits>
#include <string>
#include <type_traits>

namespace bmcweb
{
namespace detail
{

template <typename Func, typename... ArgsWrapped>
struct Wrapped
{};

template <typename Func, typename... ArgsWrapped>
struct Wrapped<Func, std::tuple<ArgsWrapped...>>
{
    explicit Wrapped(Func f) : handler(std::move(f)) {}

    std::function<void(ArgsWrapped...)> handler;

    void operator()(const Request& req,
                    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                    const std::vector<std::string>& params)
    {
        if constexpr (sizeof...(ArgsWrapped) == 2)
        {
            handler(req, asyncResp);
        }
        else if constexpr (sizeof...(ArgsWrapped) == 3)
        {
            handler(req, asyncResp, params[0]);
        }
        else if constexpr (sizeof...(ArgsWrapped) == 4)
        {
            handler(req, asyncResp, params[0], params[1]);
        }
        else if constexpr (sizeof...(ArgsWrapped) == 5)
        {
            handler(req, asyncResp, params[0], params[1], params[2]);
        }
        else if constexpr (sizeof...(ArgsWrapped) == 6)
        {
            handler(req, asyncResp, params[0], params[1], params[2], params[3]);
        }
        else if constexpr (sizeof...(ArgsWrapped) == 7)
        {
            handler(req, asyncResp, params[0], params[1], params[2], params[3],
                    params[4]);
        }
    }
};
} // namespace detail

class DynamicRule : public BaseRule, public RuleParameterTraits<DynamicRule>
{
  public:
    explicit DynamicRule(const std::string& ruleIn) : BaseRule(ruleIn) {}

    void validate() override
    {
        if (!erasedHandler)
        {
            throw std::runtime_error("no handler for url " + rule);
        }
    }

    void handle(const Request& req,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const std::vector<std::string>& params) override
    {
        erasedHandler(req, asyncResp, params);
    }

    template <typename Func>
    void operator()(Func f)
    {
        using boost::callable_traits::args_t;
        erasedHandler = detail::Wrapped<Func, args_t<Func>>(std::move(f));
    }

  private:
    std::function<void(const Request&,
                       const std::shared_ptr<bmcweb::AsyncResp>&,
                       const std::vector<std::string>&)>
        erasedHandler;
};

} // namespace bmcweb

namespace crow = bmcweb;
