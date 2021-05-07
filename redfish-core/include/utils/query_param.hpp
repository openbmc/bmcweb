#pragma once
#include "async_resp.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"

#include <systemd/sd-journal.h>

#include <app.hpp>

#include <string_view>
#include <vector>
namespace redfish
{
namespace query_param
{
class ProcessParam
{
  public:
    ProcessParam(App& appIn, crow::Response& resIn) : app(appIn), res(resIn)
    {}
    ~ProcessParam() = default;

    bool processAllParam(const crow::Request& req)
    {
        if (res.resultInt() < 200 || res.resultInt() >= 400)
        {
            return false;
        }
        if (!checkParameters(req))
        {
            return false;
        }
        if (isOnly)
        {
            if (processOnly(req))
            {
                return true;
            }
            return false;
        }
        return false;
    }

  protected:
    bool checkParameters(const crow::Request& req)
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

    bool processOnly(const crow::Request& req)
    {
        auto itMembers = res.jsonValue.find("Members");
        if (itMembers == res.jsonValue.end())
        {
            res.jsonValue.clear();
            messages::actionParameterNotSupported(res, "only", req.url);
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
        newReq.emplace(req.req);
        newReq->session = req.session;
        newReq->setTarget(url);
        res.jsonValue.clear();
        auto asyncResp = std::make_shared<bmcweb::AsyncResp>(res);
        app.handle(*newReq, asyncResp);
        return true;
    }

  private:
    App& app;
    crow::Response& res;
    std::optional<crow::Request> newReq;
    bool isOnly = false;
};
} // namespace query_param
} // namespace redfish
