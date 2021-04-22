#pragma once
#include "app.hpp"

#include <systemd/sd-journal.h>

#include <string_view>
#include <vector>
namespace redfish
{
namespace query_param
{
bool checkParameters(const crow::Request& req,
                     const std::shared_ptr<bmcweb::AsyncResp>& aResp)
{
    bool isOnly = false;
    if (req.urlParams.empty())
    {
        return false;
    }
    auto itOnly = req.urlParams.find("only");
    if (itOnly != req.urlParams.end())
    {
        if (itOnly->encoded_value().empty())
        {
            isOnly = true;
        }
        else
        {
            aResp->res.jsonValue.clear();
            messages::queryParameterValueFormatError(
                aResp->res, itOnly->encoded_value(), itOnly->encoded_key());
            return false;
        }
    }
    for (auto it : req.urlParams)
    {
        if (it->encoded_key() == "only")
        {
            continue;
        }
        if (it->encoded_key() == "$expand")
        {
            if (isOnly)
            {
                aResp->res.jsonValue.clear();
                messages::queryCombinationInvalid(aResp->res);
                return false;
            }
        }
        else
        {
            aResp->res.jsonValue.clear();
            messages::actionParameterNotSupported(aResp->res, it->encoded_key(),
                                                  req.url);
            return false;
        }
    }
    return true;
}
void porcessOnly(crow::App& app, const crow::Request& req,
                 const std::shared_ptr<bmcweb::AsyncResp>& aResp)
{
    nlohmann::json::iterator itMembers = aResp->res.jsonValue.find("Members");
    if (itMembers == aResp->res.jsonValue.end())
    {
        BMCWEB_LOG_DEBUG << "Can't find member.";
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
        BMCWEB_LOG_DEBUG << "Can't get odata.id";
        return;
    }
    const std::string url = itUrl->get<const std::string>();
    crow::Request reqS = req;
    reqS.setUrl(url);
    aResp->res.jsonValue.clear();
    app.handle(reqS, aResp);
}

void processAllParam(crow::App& app, const crow::Request& req,
                     const std::shared_ptr<bmcweb::AsyncResp>& aResp)
{
    if (!checkParameters(req, aResp))
    {
        return;
    }
    if (req.urlParams.find("only") != req.urlParams.end())
    {
        porcessOnly(app, req, aResp);
        return;
    }
}
} // namespace query_param
} // namespace redfish
