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
class RfOemBaseRule : public BaseRule
{
  public:
    explicit RfOemBaseRule(const std::string& thisRule) : BaseRule(thisRule) {}
    ~RfOemBaseRule() override = default;
    RfOemBaseRule(const RfOemBaseRule&) = delete;
    RfOemBaseRule(RfOemBaseRule&&) = delete;
    RfOemBaseRule& operator=(const RfOemBaseRule&) = delete;
    RfOemBaseRule& operator=(const RfOemBaseRule&&) = delete;

    void handle(const Request& /*req*/,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const std::vector<std::string>& /*params*/) override
    {
        asyncResp->res.result(boost::beast::http::status::not_found);
    }

    virtual void handle(Request& /*req*/,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::vector<std::string>& /*params*/,
                        const nlohmann::json& /*payload*/)
    {
        asyncResp->res.result(boost::beast::http::status::not_found);
    }
};

template <typename... Args>
class RfOemRule :
    public RfOemBaseRule,
    public RuleParameterTraits<RfOemRule<Args...>>
{
  public:
    using self_t = RfOemRule<Args...>;

    explicit RfOemRule(const std::string& ruleIn) : RfOemBaseRule(ruleIn) {}

    void validate() override
    {
        if (!getHandler && !patchHandler)
        {
            throw std::runtime_error("no handler for url " + rule);
        }
    }

    void setGetHandler(
        std::function<void(const crow::Request&,
                           const std::shared_ptr<bmcweb::AsyncResp>&, Args...)>
            newHandler)
    {
        getHandler = std::move(newHandler);
    }

    void setPatchHandler(
        std::function<void(const crow::Request&,
                           const std::shared_ptr<bmcweb::AsyncResp>&,
                           const nlohmann::json::object_t& payload, Args...)>
            newHandler)
    {
        patchHandler = std::move(newHandler);
    }

    void handle(const Request& req,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const std::vector<std::string>& params) override
    {
        if constexpr (sizeof...(Args) == 0)
        {
            getHandler(req, asyncResp);
        }
        else if constexpr (sizeof...(Args) == 1)
        {
            getHandler(req, asyncResp, params[0]);
        }
        else if constexpr (sizeof...(Args) == 2)
        {
            getHandler(req, asyncResp, params[0], params[1]);
        }
        else if constexpr (sizeof...(Args) == 3)
        {
            getHandler(req, asyncResp, params[0], params[1], params[2]);
        }
        else if constexpr (sizeof...(Args) == 4)
        {
            getHandler(req, asyncResp, params[0], params[1], params[2],
                       params[3]);
        }
        else if constexpr (sizeof...(Args) == 5)
        {
            getHandler(req, asyncResp, params[0], params[1], params[2],
                       params[3], params[4]);
        }
        static_assert(sizeof...(Args) <= 5, "More args than are supported");
    }

    void handle(Request& req,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const std::vector<std::string>& params,
                const nlohmann::json& payload) override
    {
        BMCWEB_LOG_DEBUG("OEM Patch Fragment JSON {}", payload.dump(4));

        if constexpr (sizeof...(Args) == 0)
        {
            patchHandler(req, asyncResp, payload);
        }
        else if constexpr (sizeof...(Args) == 1)
        {
            patchHandler(req, asyncResp, payload, params[0]);
        }
        else if constexpr (sizeof...(Args) == 2)
        {
            patchHandler(req, asyncResp, payload, params[0], params[1]);
        }
        else if constexpr (sizeof...(Args) == 3)
        {
            patchHandler(req, asyncResp, payload, params[0], params[1],
                         params[2]);
        }
        else if constexpr (sizeof...(Args) == 4)
        {
            patchHandler(req, asyncResp, payload, params[0], params[1],
                         params[2], params[3]);
        }
        else if constexpr (sizeof...(Args) == 5)
        {
            patchHandler(req, asyncResp, payload, params[0], params[1],
                         params[2], params[3], params[4]);
        }
        static_assert(sizeof...(Args) <= 5, "More args than are supported");
    }

  private:
    std::function<void(const crow::Request&,
                       const std::shared_ptr<bmcweb::AsyncResp>&, Args...)>
        getHandler;
    std::function<void(const crow::Request&,
                       const std::shared_ptr<bmcweb::AsyncResp>&,
                       const nlohmann::json::object_t& payload, Args...)>
        patchHandler;
};
} // namespace crow
