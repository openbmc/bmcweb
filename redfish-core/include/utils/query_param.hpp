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
                isParsed = false;
                return true;
            }
            return false;
        }
        if (isExpand)
        {
            if (processExpand(req))
            {
                return true;
            }
        }
        isParsed = false;
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
                res.jsonValue.clear();
                messages::queryCombinationInvalid(res);
                return false;
            }
            if (!it->value().empty())
            {
                res.jsonValue.clear();
                messages::queryParameterValueFormatError(res, it->value(),
                                                         it->key());
                return false;
            }
            isOnly = true;
            return true;
        }
        isExpand = false;
        for (auto ite : req.urlParams)
        {
            if (ite->key() == "$expand")
            {
                std::string value = std::string(ite->value());
                if (value == "*" || value == "." || value == "~")
                {
                    isExpand = true;
                    expandType = std::move(value);
                    expandLevel = 1;
                }
                else if (value.size() >= 12 &&
                         value.substr(1, 9) == "($levels=" &&
                         (value[0] == '*' || value[0] == '.' ||
                          value[0] == '~') &&
                         value[value.size() - 1] == ')')
                {
                    isExpand = true;
                    expandType = value[0];
                    expandLevel =
                        strtoul(value.substr(10, value.size() - 2).c_str(),
                                nullptr, 10);
                    expandLevel =
                        expandLevel < maxLevel ? expandLevel : maxLevel;
                }
                else
                {
                    res.jsonValue.clear();
                    messages::queryParameterValueFormatError(res, ite->value(),
                                                             ite->key());
                    return false;
                }
            }
            continue;
        }
        return true;
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

    bool processExpand(const crow::Request& req)
    {
        if (pendingUrlVec.size() == 0)
        {
            if (res.jsonValue.find("isExpand") == res.jsonValue.end())
            {
                res.jsonValue["isExpand"] = expandLevel + 1;
            }
            recursiveHyperlinks(res.jsonValue);
            if (pendingUrlVec.size() == 0)
            {
                deleteExpand(res.jsonValue);
                return false;
            }
            jsonValue = res.jsonValue;
            newReq.emplace(req.req);
            newReq->session = req.session;
            newReq->setTarget(pendingUrlVec[0] + "?$expand=" + expandType +
                              "($levels=" + std::to_string(expandLevel) + ")");
            res.jsonValue.clear();
            auto asyncResp = std::make_shared<bmcweb::AsyncResp>(res);
            app.handle(*newReq, asyncResp);
            return true;
        }
        if (res.jsonValue.find("isExpand") == res.jsonValue.end())
        {
            res.jsonValue["isExpand"] = expandLevel;
        }
        insertJson(jsonValue, pendingUrlVec[0], res.jsonValue);
        pendingUrlVec.erase(pendingUrlVec.begin());
        if (pendingUrlVec.size() != 0)
        {
            newReq.emplace(req.req);
            newReq->session = req.session;
            newReq->setTarget(pendingUrlVec[0] + "?$expand=" + expandType +
                              "($levels=" + std::to_string(expandLevel) + ")");
            res.jsonValue.clear();
            auto asyncResp = std::make_shared<bmcweb::AsyncResp>(res);
            app.handle(*newReq, asyncResp);
            return true;
        }
        expandLevel -= 1;
        if (expandLevel == 0)
        {
            res.jsonValue = jsonValue;
            jsonValue.clear();
            deleteExpand(res.jsonValue);
            return false;
        }
        recursiveHyperlinks(jsonValue);
        if (pendingUrlVec.size() == 0)
        {
            deleteExpand(res.jsonValue);
            return false;
        }
        newReq.emplace(req.req);
        newReq->session = req.session;
        newReq->setTarget(pendingUrlVec[0] + "?$expand=" + expandType +
                          "($levels=" + std::to_string(expandLevel) + ")");
        res.jsonValue.clear();
        auto asyncResp = std::make_shared<bmcweb::AsyncResp>(res);
        app.handle(*newReq, asyncResp);
        return true;
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

                if (url.find('#') == std::string::npos)
                {
                    pendingUrlVec.push_back(url);
                }
            }
            else if (expandType == "*" || expandType == ".")
            {
                it = j.find("@Redfish.Settings");
                if (it != j.end())
                {
                    std::string url = it->get<std::string>();
                    if (url.find('#') == std::string::npos)
                    {
                        pendingUrlVec.push_back(url);
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
                            pendingUrlVec.push_back(url);
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
                                pendingUrlVec.push_back(url);
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
                for (auto& itArray : *it)
                {
                    recursiveHyperlinks(itArray);
                }
            }
        }
    }

    bool insertJson(nlohmann::json& base, const std::string& pos,
                    const nlohmann::json& data)
    {
        auto itExpand = base.find("isExpand");
        if (itExpand != base.end())
        {
            if (itExpand->get<unsigned int>() <= expandLevel)
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
                for (auto& itArray : *it)
                {
                    if (insertJson(itArray, pos, data))
                    {
                        return true;
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
        for (auto& it : j)
        {
            if (it.is_structured())
            {
                deleteExpand(it);
            }
        }
    }

  private:
    App& app;
    crow::Response& res;
    std::optional<crow::Request> newReq;
    bool isParsed = false;
    bool isOnly = false;
    bool isExpand = false;
    uint64_t expandLevel;
    const uint64_t maxLevel = 2;
    std::string expandType;
    std::vector<std::string> pendingUrlVec;
    nlohmann::json jsonValue;
};
} // namespace query_param
} // namespace redfish
