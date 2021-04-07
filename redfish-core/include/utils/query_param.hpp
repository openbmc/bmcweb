
#pragma once
#include <systemd/sd-journal.h>

#include <app.hpp>
#include <error_messages.hpp>

#include <string_view>
namespace redfish
{
namespace query_param
{
void excuteQueryParamAll(crow::App& app, const crow::Request& req,
                         const std::shared_ptr<bmcweb::AsyncResp>& aResp)
{
    boost::urls::url_view::params_type::iterator itOnly =
        req.urlParams.find("only");
    if (itOnly == req.urlParams.end())
    {
        return;
    }
    if (req.urlParams.size() > 1)
    {
        redfish::messages::queryCombinationInvalid(aResp->res);
        return;
    }
    nlohmann::json::iterator itMemberCount =
        aResp->res.jsonValue.find("Members@odata.count");
    if (itMemberCount == aResp->res.jsonValue.end())
    {
        BMCWEB_LOG_DEBUG << "Can't find member count.";
        return;
    }
    nlohmann::json::iterator itMembersT = aResp->res.jsonValue.find("Members");
    if (itMembersT == aResp->res.jsonValue.end())
    {
        BMCWEB_LOG_DEBUG << "Can't find member.";
        return;
    }
    if (itMemberCount.value() != 1 || itMembersT.value().size() != 1)
    {
        BMCWEB_LOG_DEBUG << "Members contains more than one element, returning "
                            "full collection.";
        return;
    }
    nlohmann::json::iterator itUrl = (*itMembersT)[0].find("@odata.id");
    if (itUrl != (*itMembersT)[0].end())
    {
        const std::string url = itUrl.value();
        aResp->res.jsonValue.clear();
        crow::Request reqS = req;
        reqS.url = url;
        reqS.session = nullptr;
        app.handle(reqS, aResp);
    }
    return;
}
} // namespace query_param
} // namespace redfish
