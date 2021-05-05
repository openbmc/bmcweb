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
            return true;
        }
        auto it = req.urlParams.find("only");
        if (it != req.urlParams.end())
        {
            porcessOnly(req, res);
            return false;
        }
        it = req.urlParams.find("$expand");
        if (it != req.urlParams.end())
        {
            if (porcessExpand(req, res))
            {
                return false;
            }
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
        for (auto ite : req.urlParams)
        {
            if (ite->encoded_key() == "$expand")
            {
                std::string value = std::string(ite->encoded_value());
                if (value.size() == 1)
                {
                    expandType = std::move(value);
                    expandLevel = 1;
                }
                else if (value.size() >= 12 &&
                         value.substr(1, 9) == "($levels=" &&
                         value[value.size() - 1] == ')')
                {
                    expandType = value[0];
                    expandLevel =
                        atoi(value.substr(10, value.size() - 2).c_str());
                    expandLevel =
                        expandLevel < maxLevel ? expandLevel : maxLevel;
                }
                else
                {
                    res.jsonValue.clear();
                    messages::queryParameterValueFormatError(
                        res, ite->encoded_value(), ite->encoded_key());
                    return false;
                }
            }
            else
            {
                res.jsonValue.clear();
                messages::actionParameterNotSupported(res, ite->encoded_key(),
                                                      req.url);
                return false;
            }
        }
        return true;
    }

  protected:
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

    bool porcessExpand(crow::Request& req, crow::Response& res)
    {
        std::string url;
        if (pendingUrl.size() == 0)
        {
            if (res.jsonValue.find("isExpand") == res.jsonValue.end())
            {
                res.jsonValue["isExpand"] = expandLevel + 1;
            }
            recursiveHyperlinks(res.jsonValue);
            if (pendingUrl.size() == 0)
            {
                deleteExpand(res.jsonValue);
                return false;
            }
            jsonValue = res.jsonValue;

            res.jsonValue.clear();
            req.setUrl(pendingUrl[0] + "?$expand=" + expandType +
                       "($levels=" + std::to_string(expandLevel) + ")");
            res.setCompleted(false);
            auto asyncResp = std::make_shared<bmcweb::AsyncResp>(res);
            handler->handle(req, asyncResp);
            return true;
        }
        if (res.jsonValue.find("isExpand") == res.jsonValue.end())
        {
            res.jsonValue["isExpand"] = expandLevel;
        }
        insertJson(jsonValue, pendingUrl[0], res.jsonValue);
        pendingUrl.erase(pendingUrl.begin());

        if (pendingUrl.size() != 0)
        {
            res.jsonValue.clear();
            req.setUrl(pendingUrl[0] + "?$expand=" + expandType +
                       "($levels=" + std::to_string(expandLevel) + ")");
            res.setCompleted(false);
            auto asyncResp = std::make_shared<bmcweb::AsyncResp>(res);
            handler->handle(req, asyncResp);
            return true;
        }
        else
        {
            expandLevel -= 1;
            if (expandLevel == 0)
            {
                res.jsonValue = jsonValue;
                jsonValue.clear();
                deleteExpand(res.jsonValue);
                return false;
            }
            recursiveHyperlinks(jsonValue);
            if (pendingUrl.size() == 0)
            {
                deleteExpand(res.jsonValue);
                return false;
            }
            res.jsonValue.clear();
            req.setUrl(pendingUrl[0] + "?$expand=" + expandType +
                       "($levels=" + std::to_string(expandLevel) + ")");
            res.setCompleted(false);
            auto asyncResp = std::make_shared<bmcweb::AsyncResp>(res);
            handler->handle(req, asyncResp);
            return true;
        }
    }

    void recursiveHyperlinks(nlohmann::json& j)
    {
        auto itExpand = j.find("isExpand");
        if (itExpand == j.end())
        {
            auto it = j.find("@odata.id");
            if (it != j.end())
            {
                std::string url = it->get<std::string>();

                if (url.find("#") == std::string::npos)
                {
                    pendingUrl.push_back(url);
                }
            }
            else if (expandType == "*" || expandType == ".")
            {
                it = j.find("@Redfish.Settings");
                if (it != j.end())
                {
                    std::string url = it->get<std::string>();
                    if (url.find("#") == std::string::npos)
                    {
                        pendingUrl.push_back(url);
                    }
                }
                else
                {
                    it = j.find("@Redfish.ActionInfo");
                    if (it != j.end())
                    {
                        std::string url = it->get<std::string>();
                        if (url.find("#") == std::string::npos)
                        {
                            pendingUrl.push_back(url);
                        }
                    }
                    else
                    {
                        it = j.find("@Redfish.CollectionCapabilities");
                        if (it != j.end())
                        {
                            std::string url = it->get<std::string>();
                            if (url.find("#") == std::string::npos)
                            {
                                pendingUrl.push_back(url);
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
                if (expandType == "." && it.key() == "Links")
                {
                    continue;
                }
                recursiveHyperlinks(*it);
            }
            else if (it->is_array())
            {
                for (auto itArray = it->begin(); itArray != it->end();
                     ++itArray)
                {
                    if (itArray->is_object())
                    {
                        recursiveHyperlinks(*itArray);
                    }
                }
            }
        }
    }

    bool insertJson(nlohmann::json& base, const std::string pos,
                    nlohmann::json data)
    {
        auto itExpand = base.find("isExpand");
        if (itExpand != base.end())
        {
            if (itExpand->get<int>() <= expandLevel)
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
            else if (expandType == "*" || expandType == ".")
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
                if (expandType == "." && it.key() == "Links")
                {
                    continue;
                }
                if (insertJson(*it, pos, data))
                {
                    return true;
                }
            }
            else if (it->is_array())
            {
                for (auto itArray = it->begin(); itArray != it->end();
                     ++itArray)
                {
                    if (itArray->is_object())
                    {
                        if (insertJson(*itArray, pos, data))
                        {
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }
    void deleteExpand(nlohmann::json& j)
    {
        auto it = j.find("isExpand");
        if (it != j.end())
        {
            j.erase(it);
        }
        for (auto it = j.begin(); it != j.end(); ++it)
        {
            if (it->is_structured())
            {
                deleteExpand(*it);
            }
        }
    }

  private:
    Handler* handler;
    nlohmann::json jsonValue;
    int expandLevel;
    int maxLevel = 2;
    std::string expandType;
    std::vector<std::string> pendingUrl;
};
} // namespace query_param
} // namespace redfish
