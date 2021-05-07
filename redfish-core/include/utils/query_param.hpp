#pragma once
#include "app.hpp"
#include "async_resp.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "routing.hpp"

#include <systemd/sd-journal.h>

#include <string_view>
#include <vector>

#define REDFISH_ROUTE(app, req, asyncResp)                                     \
    if ((asyncResp)->res.processParamHandler == nullptr)                       \
    {                                                                          \
        auto processParam =                                                    \
            std::make_shared<redfish::query_param::ProcessParam>(              \
                (app), (asyncResp)->res);                                      \
        (asyncResp)->res.processParamHandler = [req, processParam]() -> bool { \
            return processParam->processAllParam(req);                         \
        };                                                                     \
    }
namespace redfish
{
namespace query_param
{
class ProcessParam
{
  public:
    ProcessParam(crow::App& appIn, crow::Response& resIn) :
        app(appIn), res(resIn)
    {}
    ~ProcessParam() = default;

    bool processAllParam(const crow::Request& req)
    {
        if (res.resultInt() < 200 || res.resultInt() >= 400)
        {
            return false;
        }
        if (!isParsed)
        {
            if (!checkParameters(req))
            {
                return false;
            }
        }
        if (isOnly)
        {
            if (processOnly(req))
            {
                isOnly = false;
                return true;
            }
            return false;
        }
        return false;
    }

  private:
    bool checkParameters(const crow::Request& req)
    {
        isParsed = true;
        isOnly = false;
        auto it = req.urlParams.find("only");
        if (it != req.urlParams.end())
        {
            if (req.urlParams.size() != 1)
            {
                messages::queryCombinationInvalid(res);
                return false;
            }
            if (!it->value().empty())
            {
                messages::queryParameterValueFormatError(res, it->value(),
                                                         it->key());
                return false;
            }
            isOnly = true;
            return true;
        }
        return true;
    }

    bool processOnly(const crow::Request& req)
    {
        auto itMembers = res.jsonValue.find("Members");
        if (itMembers == res.jsonValue.end())
        {
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

    crow::App& app;
    crow::Response& res;
    std::optional<crow::Request> newReq;
    bool isParsed = false;
    bool isOnly = false;
};
} // namespace query_param
} // namespace redfish
