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
    ~ProcessParam() = default;

    bool processAllParam(crow::Request& req, crow::Response& res)
    {
        if (!checkParameters(req, res))
        {
            return false;
        }
        if (isOnly)
        {
            if (processOnly(req, res))
            {
                return true;
            }
            return false;
        }
        return false;
    }

  protected:
    bool checkParameters(crow::Request& req, crow::Response& res)
    {
        isOnly = false;
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
            isOnly = true;
            return true;
        }
        return false;
    }

    void processOnly(crow::Request& req, crow::Response& res)
    {
        auto itMembers = res.jsonValue.find("Members");
        if (itMembers == res.jsonValue.end())
        {
            BMCWEB_LOG_DEBUG << "No found Members.";
            return false;
        }

        if (itMembers->size() != 1)
        {
            BMCWEB_LOG_DEBUG << "Members contains " << itMembers->size()
                             << " element, returning full collection.";
            return false;
        }
        auto itMemBegin = itMembers->begin();
        auto itUrl = itMemBegin->find("@odata.id");
        if (itUrl == itMemBegin->end())
        {
            BMCWEB_LOG_DEBUG << "No found odata.id";
            return false;
        }
        const std::string url = itUrl->get<const std::string>();
        req.setUrl(url);
        res.clearCompleted();
        res.jsonValue.clear();
        auto asyncResp = std::make_shared<bmcweb::AsyncResp>(res);
        handler->handle(req, asyncResp);
        return true;
    }

  private:
    Handler* handler;
    bool isOnly = false;
};
} // namespace query_param
} // namespace redfish
