#pragma once
#include "error_messages.hpp"
#include "http_request.hpp"

#include <systemd/sd-journal.h>

#include <string_view>
#include <vector>
namespace redfish
{
namespace query_param
{
template <typename Handler>
class ProcessParam
{
  public:
    ProcessParam(Handler* handlerIn) : handler(handlerIn)
    {}
    ~ProcessParam()
    {}

    bool processAllParam(crow::Request& req, crow::Response& res)
    {
        if (!checkParameters(req, res))
        {
            return true;
        }
        auto it = req.urlParams.find("only");
        if (it != req.urlParams.end())
        {
            porcessOnly(req, res);
            return false;
        }
        return true;
    }

  protected:
    bool checkParameters(crow::Request& req, crow::Response& res)
    {
        if (req.urlParams.empty())
        {
            return false;
        }
        auto it = req.urlParams.find("only");
        if (it != req.urlParams.end())
        {
            if (req.urlParams.size() != 1)
            {
                res.jsonValue.clear();
                messages::queryCombinationInvalid(res);
                return false;
            }
            if (!it->encoded_value().empty())
            {
                res.jsonValue.clear();
                messages::queryParameterValueFormatError(
                    res, it->encoded_value(), it->encoded_key());
                return false;
            }
        }
        return true;
    }

    void porcessOnly(crow::Request& req, crow::Response& res)
    {
        nlohmann::json::iterator itMembers = res.jsonValue.find("Members");
        if (itMembers == res.jsonValue.end())
        {
            BMCWEB_LOG_DEBUG << "No found Members.";
            return;
        }

        nlohmann::json::iterator itMemBegin = itMembers->begin();
        if (itMemBegin == itMembers->end())
        {
            BMCWEB_LOG_DEBUG << "Members contains no element,"
                                "returning full collection.";
            return;
        }
        if (itMembers->size() > 1)
        {
            BMCWEB_LOG_DEBUG << "Members contains more than one element,"
                                "returning full collection.";
            return;
        }
        nlohmann::json::iterator itUrl = itMemBegin->find("@odata.id");
        if (itUrl == itMemBegin->end())
        {
            BMCWEB_LOG_DEBUG << "No found odata.id";
            return;
        }
        const std::string url = itUrl->get<const std::string>();
        req.setUrl(url);
        res.setCompleted(false);
        res.jsonValue.clear();
        auto asyncResp = std::make_shared<bmcweb::AsyncResp>(res);
        handler->handle(req, asyncResp);
    }

  private:
    Handler* handler;
};
} // namespace query_param
} // namespace redfish
