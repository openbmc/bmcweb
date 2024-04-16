#pragma once

#include "async_resp.hpp"
#include "http_request.hpp"
#include "privileges.hpp"
#include "verb.hpp"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <memory>
#include <string>

namespace crow
{
class BaseRule
{
  public:
    explicit BaseRule(const std::string& thisRule) : rule(thisRule) {}

    virtual ~BaseRule() = default;

    BaseRule(const BaseRule&) = delete;
    BaseRule(BaseRule&&) = delete;
    BaseRule& operator=(const BaseRule&) = delete;
    BaseRule& operator=(const BaseRule&&) = delete;

    virtual void validate() = 0;
    std::unique_ptr<BaseRule> upgrade()
    {
        if (ruleToUpgrade)
        {
            return std::move(ruleToUpgrade);
        }
        return {};
    }

    virtual void handle(const Request& /*req*/,
                        const std::shared_ptr<bmcweb::AsyncResp>&,
                        const std::vector<std::string>&) = 0;
    virtual void
        handleUpgrade(const Request& /*req*/,
                      const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      boost::asio::ip::tcp::socket&& /*adaptor*/)
    {
        asyncResp->res.result(boost::beast::http::status::not_found);
    }

    virtual void handleUpgrade(
        const Request& /*req*/,
        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
        boost::asio::ssl::stream<boost::asio::ip::tcp::socket>&& /*adaptor*/)
    {
        asyncResp->res.result(boost::beast::http::status::not_found);
    }

    size_t getMethods() const
    {
        return methodsBitfield;
    }

    bool checkPrivileges(const redfish::Privileges& userPrivileges)
    {
        // If there are no privileges assigned, assume no privileges
        // required
        if (privilegesSet.empty())
        {
            return true;
        }

        for (const redfish::Privileges& requiredPrivileges : privilegesSet)
        {
            if (userPrivileges.isSupersetOf(requiredPrivileges))
            {
                return true;
            }
        }
        return false;
    }

    size_t methodsBitfield{1 << static_cast<size_t>(HttpVerb::Get)};
    static_assert(std::numeric_limits<decltype(methodsBitfield)>::digits >
                      methodNotAllowedIndex,
                  "Not enough bits to store bitfield");

    std::vector<redfish::Privileges> privilegesSet;

    std::string rule;

    std::unique_ptr<BaseRule> ruleToUpgrade;

    friend class Router;
    template <typename T>
    friend struct RuleParameterTraits;
};

} // namespace crow
