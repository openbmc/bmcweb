#pragma once
#include "app.hpp"

#include <systemd/sd-journal.h>

#include <string_view>
#include <vector>
namespace redfish
{
namespace query_param
{
struct SParam
{
    const std::string key;
    bool existValue;
};
std::vector<SParam> supportedParams = {{"only", false}};
bool checkparameters(const crow::Request& req,
                     const std::shared_ptr<bmcweb::AsyncResp>& aResp)
{
    if (req.urlParams.find("only") != req.urlParams.end() &&
        req.urlParams.size() > 1)
    {
        aResp->res.jsonValue.clear();
        messages::queryCombinationInvalid(aResp->res);
        return false;
    }
    for (auto it : req.urlParams)
    {
        bool isSupported = false;
        for (auto itSupport : supportedParams)
        {
            if (it->encoded_key() == itSupport.key)
            {
                if ((itSupport.existValue && it->encoded_value().empty()) ||
                    (!itSupport.existValue && !it->encoded_value().empty()))
                {
                    aResp->res.jsonValue.clear();
                    messages::queryParameterValueFormatError(
                        aResp->res, std::string(it->encoded_value()),
                        std::string(it->encoded_key()));
                    return false;
                }
                isSupported = true;
                break;
            }
        }
        if (!isSupported)
        {
            aResp->res.jsonValue.clear();
            messages::actionParameterNotSupported(
                aResp->res, std::string(it->encoded_key()),
                std::string(req.url));
            return false;
        }
    }
    return true;
}
void excuteQueryParamAll(crow::App& app, const crow::Request& req,
                         const std::shared_ptr<bmcweb::AsyncResp>& aResp)
{
    if (req.urlParams.empty())
    {
        return;
    }
    if (!checkparameters(req, aResp))
    {
        return;
    }

    nlohmann::json::iterator itMemberCount =
        aResp->res.jsonValue.find("Members@odata.count");
    if (itMemberCount == aResp->res.jsonValue.end())
    {
        BMCWEB_LOG_DEBUG << "Can't find member count.";
        return;
    }
    nlohmann::json::iterator itMembers = aResp->res.jsonValue.find("Members");
    if (itMembers == aResp->res.jsonValue.end())
    {
        BMCWEB_LOG_DEBUG << "Can't find member.";
        return;
    }
    const int64_t* memberCount = itMemberCount->get_ptr<const int64_t*>();
    if (!memberCount)
    {
        BMCWEB_LOG_DEBUG << "Can't get member count";
        return;
    }
    if (*memberCount != 1 || (*itMembers).begin() + 1 != (*itMembers).end())
    {
        BMCWEB_LOG_DEBUG << "Members contains more than one element,"
                            "returning full collection.";
        return;
    }
    nlohmann::json::iterator itUrl = (*itMembers)[0].find("@odata.id");
    if (itUrl != (*itMembers)[0].end())
    {
        const std::string* pUrl = itUrl->get_ptr<const std::string*>();
        if (pUrl == nullptr)
        {
            BMCWEB_LOG_DEBUG << "Can't get url";
            return;
        }
        const std::string url = *pUrl;
        aResp->res.jsonValue.clear();
        crow::Request reqS = req;
        reqS.url = url;
        app.handle(reqS, aResp);
    }
}
} // namespace query_param
} // namespace redfish
