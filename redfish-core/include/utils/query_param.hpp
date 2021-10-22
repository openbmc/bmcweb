#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "routing.hpp"

#include <string_view>
#include <vector>

namespace redfish
{
namespace query_param
{
const uint64_t expandMaxLevel = 2;  // todo
const uint64_t expandMinLenth = 12; //*($levels=1)
struct Query
{
    bool isOnly = false;
    bool isExpand = false;
    uint64_t expandLevel;
    std::string expandType;
    std::vector<std::string> pendingUrlVec;
    nlohmann::json jsonValue;
};

void processAllParams(crow::App& app, Query& query,
                      crow::Response& intermediateResponse,
                      std::function<void(crow::Response&)>& completionHandler);

inline std::optional<Query>
    parseParameters(const boost::urls::query_params_view& urlParams,
                    crow::Response& res)
{
    Query ret{};
    for (const boost::urls::query_params_view::value_type& it : urlParams)
    {
        if (it.key() == "only")
        {
            // Only cannot be combined with any other query
            if (urlParams.size() != 1)
            {
                messages::queryCombinationInvalid(res);
                return std::nullopt;
            }
            if (!it.value().empty())
            {
                messages::queryParameterValueFormatError(res, it.value(),
                                                         it.key());
                return std::nullopt;
            }
            ret.isOnly = true;
        }
        else if (it.key() == "$expand")
        {
            std::string value = std::string(it.value());
            if (value == "*" || value == "." || value == "~")
            {
                ret.isExpand = true;
                ret.expandType = std::move(value);
                ret.expandLevel = 1;
            }
            else if (value.size() >= expandMinLenth &&
                     value.substr(1, 9) == "($levels=" &&
                     (value[0] == '*' || value[0] == '.' || value[0] == '~') &&
                     value[value.size() - 1] == ')')
            {
                ret.isExpand = true;
                ret.expandType = value[0];
                ret.expandLevel = strtoul(
                    value.substr(10, value.size() - 2).c_str(), nullptr, 10);
                ret.expandLevel = ret.expandLevel < expandMaxLevel
                                      ? ret.expandLevel
                                      : expandMaxLevel;
            }
            else
            {
                messages::queryParameterValueFormatError(res, it.value(),
                                                         it.key());
                return std::nullopt;
            }
        }
    }
    return ret;
}

inline void recursiveToGetUrls(nlohmann::json& j, Query& query)
{
    auto itExpand = j.find("isExpand");
    if (itExpand == j.end())
    {
        auto it = j.find("@odata.id");
        if (it != j.end())
        {
            std::string url = it->get<std::string>();

            if (url.find('#') == std::string::npos)
            {
                query.pendingUrlVec.push_back(url);
            }
        }
        else if (query.expandType == "*" || query.expandType == ".")
        {
            it = j.find("@Redfish.Settings");
            if (it != j.end())
            {
                std::string url = it->get<std::string>();
                if (url.find('#') == std::string::npos)
                {
                    query.pendingUrlVec.push_back(url);
                }
            }
            else
            {
                it = j.find("@Redfish.ActionInfo");
                if (it != j.end())
                {
                    std::string url = it->get<std::string>();
                    if (url.find('#') == std::string::npos)
                    {
                        query.pendingUrlVec.push_back(url);
                    }
                }
                else
                {
                    it = j.find("@Redfish.CollectionCapabilities");
                    if (it != j.end())
                    {
                        std::string url = it->get<std::string>();
                        if (url.find('#') == std::string::npos)
                        {
                            query.pendingUrlVec.push_back(url);
                        }
                    }
                }
            }
        }
    }
    for (auto it = j.begin(); it != j.end(); ++it)
    {
        if (it->is_object())
        {
            if (query.expandType == "." && it.key() == "Links")
            {
                continue;
            }
            recursiveToGetUrls(*it, query);
        }
        else if (it->is_array())
        {
            for (auto& itArray : *it)
            {
                recursiveToGetUrls(itArray, query);
            }
        }
    }
}

inline bool insertJson(nlohmann::json& base, const std::string& pos,
                       const nlohmann::json& data, const Query& query)
{
    auto itExpand = base.find("isExpand");
    if (itExpand != base.end())
    {
        if (itExpand->get<unsigned int>() <= query.expandLevel)
        {
            return false;
        }
    }
    else
    {
        auto it = base.find("@odata.id");
        if (it != base.end())
        {
            if (it->get<std::string>() == pos)
            {
                base = data;
                return true;
            }
        }
        else if (query.expandType == "*" || query.expandType == ".")
        {
            it = base.find("@Redfish.Settings");
            if (it != base.end())
            {
                if (it->get<std::string>() == pos)
                {
                    base = data;
                    return true;
                }
            }
            else
            {
                it = base.find("@Redfish.ActionInfo");
                if (it != base.end())
                {
                    if (it->get<std::string>() == pos)
                    {
                        base = data;
                        return true;
                    }
                }
                else
                {
                    it = base.find("@Redfish.CollectionCapabilities");
                    if (it != base.end())
                    {
                        if (it->get<std::string>() == pos)
                        {
                            base = data;
                            return true;
                        }
                    }
                }
            }
        }
    }
    for (auto it = base.begin(); it != base.end(); ++it)
    {
        if (it->is_object())
        {
            if (query.expandType == "." && it.key() == "Links")
            {
                continue;
            }
            if (insertJson(*it, pos, data, query))
            {
                return true;
            }
        }
        else if (it->is_array())
        {
            for (auto& itArray : *it)
            {
                if (insertJson(itArray, pos, data, query))
                {
                    return true;
                }
            }
        }
    }
    return false;
}

inline void deleteExpand(nlohmann::json& j)
{
    auto it = j.find("isExpand");
    if (it != j.end())
    {
        j.erase(it);
    }
    for (auto& it : j)
    {
        if (it.is_structured())
        {
            deleteExpand(it);
        }
    }
}

inline bool processOnly(crow::App& app, crow::Response& res,
                        std::function<void(crow::Response&)>& completionHandler)
{
    BMCWEB_LOG_DEBUG << "Processing only query param";
    auto itMembers = res.jsonValue.find("Members");
    if (itMembers == res.jsonValue.end())
    {
        messages::queryNotSupportedOnResource(res);
        completionHandler(res);
        return false;
    }
    auto itMemBegin = itMembers->begin();
    if (itMemBegin == itMembers->end() || itMembers->size() != 1)
    {
        BMCWEB_LOG_DEBUG << "Members contains " << itMembers->size()
                         << " element, returning full collection.";
        completionHandler(res);
        return false;
    }
    auto itUrl = itMemBegin->find("@odata.id");
    if (itUrl == itMemBegin->end())
    {
        BMCWEB_LOG_DEBUG << "No found odata.id";
        messages::internalError(res);
        completionHandler(res);
        return false;
    }
    const std::string* url = itUrl->get_ptr<const std::string*>();
    if (!url)
    {
        BMCWEB_LOG_DEBUG << "@odata.id wasn't a string????";
        messages::internalError(res);
        completionHandler(res);
        return false;
    }
    // TODO(Ed) copy request headers?
    // newReq.session = req.session;
    std::error_code ec;
    crow::Request newReq({boost::beast::http::verb::get, *url, 11}, ec);
    if (ec)
    {
        messages::internalError(res);
        completionHandler(res);
        return false;
    }
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    BMCWEB_LOG_DEBUG << "setting completion handler on " << &asyncResp->res;
    asyncResp->res.setCompleteRequestHandler(std::move(completionHandler));
    asyncResp->res.setIsAliveHelper(res.releaseIsAliveHelper());
    app.handle(newReq, asyncResp);
    return true;
}

inline bool
    processExpand(crow::App& app, crow::Response& res,
                  std::function<void(crow::Response&)>& completionHandler,
                  Query& query)
{
    if (query.pendingUrlVec.size() == 0)
    {
        if (res.jsonValue.find("isExpand") == res.jsonValue.end())
        {
            res.jsonValue["isExpand"] = query.expandLevel + 1;
        }
        recursiveToGetUrls(res.jsonValue, query);
        if (query.pendingUrlVec.size() == 0)
        {
            deleteExpand(res.jsonValue);
            return false;
        }
        query.jsonValue = res.jsonValue;

        std::error_code ec;
        crow::Request newReq(
            {boost::beast::http::verb::get, query.pendingUrlVec[0], 11}, ec);
        if (ec)
        {
            messages::internalError(res);
            completionHandler(res);
            return true;
        }
        auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
        BMCWEB_LOG_DEBUG << "setting completion handler on " << &asyncResp->res;
        asyncResp->res.setCompleteRequestHandler(
            [&app, handler(std::move(completionHandler)),
             query](crow::Response& res) mutable {
                BMCWEB_LOG_DEBUG << "Starting query params handling";
                processAllParams(app, query, res, handler);
            });
        asyncResp->res.setIsAliveHelper(res.releaseIsAliveHelper());
        app.handle(newReq, asyncResp);
        return true;
    }
    if (res.jsonValue.find("isExpand") == res.jsonValue.end())
    {
        res.jsonValue["isExpand"] = query.expandLevel;
    }
    insertJson(query.jsonValue, query.pendingUrlVec[0], res.jsonValue, query);
    query.pendingUrlVec.erase(query.pendingUrlVec.begin());
    if (query.pendingUrlVec.size() != 0)
    {
        std::error_code ec;
        crow::Request newReq(
            {boost::beast::http::verb::get, query.pendingUrlVec[0], 11}, ec);
        if (ec)
        {
            messages::internalError(res);
            completionHandler(res);
            return true;
        }
        auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
        BMCWEB_LOG_DEBUG << "setting completion handler on " << &asyncResp->res;
        asyncResp->res.setCompleteRequestHandler(
            [&app, handler(std::move(completionHandler)),
             query](crow::Response& res) mutable {
                BMCWEB_LOG_DEBUG << "Starting query params handling";
                processAllParams(app, query, res, handler);
            });
        asyncResp->res.setIsAliveHelper(res.releaseIsAliveHelper());
        app.handle(newReq, asyncResp);
        return true;
    }
    query.expandLevel -= 1;
    if (query.expandLevel == 0)
    {
        res.jsonValue = query.jsonValue;
        deleteExpand(res.jsonValue);
        return false;
    }
    recursiveToGetUrls(query.jsonValue, query);
    if (query.pendingUrlVec.size() == 0)
    {
        deleteExpand(res.jsonValue);
        return false;
    }

    std::error_code ec;
    crow::Request newReq(
        {boost::beast::http::verb::get, query.pendingUrlVec[0], 11}, ec);
    if (ec)
    {
        messages::internalError(res);
        completionHandler(res);
        return true;
    }
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    BMCWEB_LOG_DEBUG << "setting completion handler on " << &asyncResp->res;
    asyncResp->res.setCompleteRequestHandler(
        [&app, handler(std::move(completionHandler)),
         query](crow::Response& res) mutable {
            BMCWEB_LOG_DEBUG << "Starting query params handling";
            processAllParams(app, query, res, handler);
        });
    asyncResp->res.setIsAliveHelper(res.releaseIsAliveHelper());
    app.handle(newReq, asyncResp);
    return true;
}

void processAllParams(crow::App& app, Query& query,
                      crow::Response& intermediateResponse,
                      std::function<void(crow::Response&)>& completionHandler)
{
    if (!completionHandler)
    {
        BMCWEB_LOG_DEBUG << "Function was invalid?";
        return;
    }
    BMCWEB_LOG_DEBUG << "Processing query params";
    // If the request failed, there's no reason to even try to run query
    // params.
    if (intermediateResponse.resultInt() < 200 ||
        intermediateResponse.resultInt() >= 400)
    {
        completionHandler(intermediateResponse);
        return;
    }
    if (query.isOnly)
    {
        processOnly(app, intermediateResponse, completionHandler);
        return;
    }
    if (query.isExpand)
    {
        if (processExpand(app, intermediateResponse, completionHandler, query))
        {
            return;
        }
    }
    completionHandler(intermediateResponse);
}
} // namespace query_param
} // namespace redfish
