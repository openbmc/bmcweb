
#pragma once
#include "node.hpp"

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
    int queryAccount = 0;
    boost::urls::url_view::params_type::iterator itSkip =
        req.urlParams.find("skip");
    boost::urls::url_view::params_type::iterator itOnly =
        req.urlParams.find("only");
    if (itSkip != req.urlParams.end())
    {
        BMCWEB_LOG_DEBUG << "req.urlParams[skip]？= " << req.urlParams["skip"];
        // Adding $skip here is only used to test the queryCombinationInvalid
        // response of $only.
        queryAccount += 1;
    }
    if (itOnly != req.urlParams.end())
    {
        queryAccount += 1;
        if (queryAccount > 1)
        {
            std::cerr << "too many paramters. counts = " << queryAccount
                      << std::endl;
            aResp->res.clear();
            messages::queryCombinationInvalid(aResp->res);
            return;
        }
        nlohmann::json::iterator itMemberCount =
            aResp->res.jsonValue.find("Members@odata.count");
        if (itMemberCount != aResp->res.jsonValue.end())
        {
            if (aResp->res.jsonValue["Members@odata.count"] == 1 &&
                aResp->res.jsonValue.find("Members") !=
                    aResp->res.jsonValue.end())
            {
                nlohmann::json& membersT = aResp->res.jsonValue["Members"];
                if (membersT.size() != 1)
                {
                    std::cerr << "Members' count isn't suit its description."
                              << std::endl;
                    return;
                }
                if (membersT[0].find("@odata.id") != membersT[0].end())
                {
                    std::string url = membersT[0]["@odata.id"];
                    aResp->res.clear();
                    auto reqS = std::make_shared<crow::Request>(req);
                    reqS->url = url;
                    app.handle(*reqS, aResp);
                }
            }
            else
            {
                std::cerr << "The member counts is "
                          << aResp->res.jsonValue["Members@odata.count"]
                          << " not meet the 'only' requirement." << std::endl;
            }
        }
        else
        {
            std::cerr << "can't find member count." << std::endl;
        }
    }
    return;
}
} // namespace query_param
} // namespace redfish
