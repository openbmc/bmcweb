#pragma once
#include "http_request.hpp"
#include "http_response.hpp"
#include "logging.hpp"
#include "routing/baserule.hpp"
#include "routing/ruleparametertraits.hpp"

#include <boost/beast/http/status.hpp>
#include <nlohmann/json.hpp>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
namespace redfish
{
class OemBaseRule : public crow::BaseRule
{
  public:
    explicit OemBaseRule(const std::string& thisRule) : BaseRule(thisRule) {}
    ~OemBaseRule() override = default;
    OemBaseRule(const OemBaseRule&) = delete;
    OemBaseRule(OemBaseRule&&) = delete;
    OemBaseRule& operator=(const OemBaseRule&) = delete;
    OemBaseRule& operator=(const OemBaseRule&&) = delete;

    void handle(const crow::Request& /*req*/,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const std::vector<std::string>& /*params*/) override
    {
        asyncResp->res.result(boost::beast::http::status::not_found);
    }

    virtual void handle(const crow::Request& /*req*/,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::vector<std::string>& /*params*/,
                        const nlohmann::json& /*payload*/)
    {
        asyncResp->res.result(boost::beast::http::status::not_found);
    }
};

template <typename... Args>
class OemRule :
    public OemBaseRule,
    public crow::RuleParameterTraits<OemRule<Args...>>
{
  public:
    using self_t = OemRule<Args...>;

    explicit OemRule(const std::string& ruleIn) : OemBaseRule(ruleIn) {}

    void validate() override
    {
        if (!getHandler && !patchHandler)
        {
            throw std::runtime_error(
                "no OEM fragment handler for the rule {}" + rule);
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

    void handle(const crow::Request& req,
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

    void handle(const crow::Request& req,
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
} // namespace redfish
