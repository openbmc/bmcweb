#pragma once
#include "async_resp.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"

#include <systemd/sd-journal.h>

#include <app.hpp>

#include <charconv>
#include <stack>
#include <string_view>
#include <vector>

#define PARAM_NOACTION 0
#define PARAM_ERASE 1
#define PARAM_FORMATERROR 2
namespace redfish
{
namespace query_param
{
class ProcessParam
{
    enum class FilterType
    {
        Grouping,
        Compare,
        Logical,
        String,
        Boolean,
        Integer,
        Double
    };
    struct FilterValue
    {
        FilterType type;
        std::string s;
        bool b;
        int n;
        double d;
    };

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
        if (isSelect)
        {
            processSelect();
        }
        if (isSkip)
        {
            processSkip(req);
        }
        if (isTop)
        {
            processTop(req);
        }
        if (isFilter)
        {
            processFilter(req);
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
        isSelect = false;
        isSkip = false;
        isTop = false;
        isFilter = false;
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
            else if (ite->key() == "$select")
            {
                isSelect = true;
                selectPropertyVec.clear();
                std::string value = std::string(ite->value());
                auto result = stringSplit(value, ",");
                for (auto& itResult : result)
                {
                    auto v = stringSplit(itResult, "/");
                    if (v.empty())
                    {
                        continue;
                    }
                    selectPropertyVec.push_back(std::move(v));
                }
            }
            else if (ite->key() == "$skip")
            {
                skipValue = strtoul(ite->value().c_str(), nullptr, 10);
                if (skipValue > 0)
                {
                    isSkip = true;
                }
            }
            else if (ite->key() == "$top")
            {
                topValue = strtoul(ite->value().c_str(), nullptr, 10);
                if (topValue > 0)
                {
                    isTop = true;
                }
            }
            else if (ite->key() == "$filter")
            {
                std::string value = ite->value();
                filterValue = value;
                if (value.empty())
                {
                    res.jsonValue.clear();
                    messages::queryParameterValueFormatError(res, it->value(),
                                                             it->key());
                    return false;
                }

                value.erase(0, value.find_first_not_of(' '));
                value.erase(value.find_last_not_of(' ') + 1);

                std::string result = "";
                for (size_t i = 0; i < value.length(); i++)
                {
                    if (value[i] != ' ')
                    {
                        if (value[i] == '(' || value[i] == ')')
                        {
                            if (i > 0 && value[i - 1] != ' ')
                            {
                                result.append(1, ' ');
                                result.append(1, value[i]);
                            }
                            else if (i < value.length() - 1 &&
                                     value[i + 1] != ' ')
                            {
                                result.append(1, value[i]);
                                result.append(1, ' ');
                            }
                            else
                            {
                                result.append(1, value[i]);
                            }
                        }
                        else
                        {
                            result.append(1, value[i]);
                        }
                    }
                    else if (value[i + 1] != ' ')
                    {
                        result.append(1, value[i]);
                    }
                }
                filterVec = stringSplit(result, " ");
                isFilter = true;
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

    void processSelect()
    {
        jsonValue = nlohmann::json::object();
        for (auto& it : selectPropertyVec)
        {
            recursiveSelect(it, res.jsonValue, jsonValue);
        }
        res.jsonValue = jsonValue;
    }

    void processSkip(const crow::Request& req)
    {
        auto it = res.jsonValue.find("Members");
        if (it == res.jsonValue.end())
        {
            res.jsonValue.clear();
            messages::actionParameterNotSupported(res, "$skip", req.url);
            return;
        }

        if (it->size() <= skipValue)
        {
            it->clear();
            return;
        }
        it->erase(it->begin(), it->begin() + int(skipValue));
    }

    void processTop(const crow::Request& req)
    {
        auto it = res.jsonValue.find("Members");
        if (it == res.jsonValue.end())
        {
            res.jsonValue.clear();
            messages::actionParameterNotSupported(res, "$top", req.url);
            return;
        }
        if (it->size() <= topValue)
        {
            return;
        }
        it->erase(it->begin() + int(topValue), it->end());
    }

    void processFilter(const crow::Request& req)
    {
        auto itMembers = res.jsonValue.find("Members");
        if (itMembers == res.jsonValue.end())
        {
            res.jsonValue.clear();
            messages::actionParameterNotSupported(res, "$filter", req.url);
            return;
        }
        for (auto it = itMembers->begin(); it != itMembers->end();)
        {
            auto ret = parseFilterParameters(*it);
            if (ret == PARAM_ERASE)
            {
                it = itMembers->erase(it);
                continue;
            }
            if (ret == PARAM_FORMATERROR)
            {
                res.jsonValue.clear();
                messages::queryParameterValueFormatError(res, filterValue,
                                                         "$filter");
                return;
            }
            it++;
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

    std::vector<std::string> stringSplit(const std::string& s,
                                         const std::string& delim)
    {
        std::vector<std::string> elems;
        size_t pos = 0;
        size_t len = s.length();
        size_t delimLen = delim.length();
        if (delimLen == 0)
        {
            return elems;
        }
        while (pos < len)
        {
            size_t findPos = s.find(delim, pos);
            if (findPos == std::string::npos)
            {
                elems.push_back(s.substr(pos, len - pos));
                break;
            }
            elems.push_back(s.substr(pos, findPos - pos));
            pos = findPos + delimLen;
        }
        return elems;
    }

    void recursiveSelect(std::vector<std::string> v, nlohmann::json& src,
                         nlohmann::json& dst)
    {
        if (v.empty())
        {
            return;
        }
        if (src.is_array())
        {
            for (auto it : src)
            {
                dst.push_back(nlohmann::json());
                recursiveSelect(v, it, dst.back());
            }
            return;
        }
        std::string key = v[0];
        auto it = src.find(key);
        if (v.size() == 1)
        {
            dst.insert(it, ++it);
        }
        else
        {
            v.erase(v.begin());
            if (it->is_array())
            {
                dst[key] = nlohmann::json::array();
            }
            else
            {
                dst[key] = nlohmann::json::object();
            }
            recursiveSelect(v, *it, dst[key]);
        }
        it = src.find("@odata.id");
        if (it != src.end())
        {
            dst.insert(it, ++it);
        }
        it = src.find("@odata.type");
        if (it != src.end())
        {
            dst.insert(it, ++it);
        }
        it = src.find("@odata.context");
        if (it != src.end())
        {
            dst.insert(it, ++it);
        }
        it = src.find("@odata.etag");
        if (it != src.end())
        {
            dst.insert(it, ++it);
        }
    }

    int8_t parseFilterParameters(nlohmann::json& j)
    {
        std::vector<FilterValue> filterValueVec;
        for (auto& it : filterVec)
        {
            if (it == "(" || it == ")")
            {
                FilterValue fv;
                fv.type = FilterType::Grouping;
                fv.s = it;
                filterValueVec.push_back(std::move(fv));
            }
            else if (it == "eq" || it == "ne" || it == "ge" || it == "gt" ||
                     it == "le" || it == "lt")
            {
                FilterValue fv;
                fv.type = FilterType::Compare;
                fv.s = it;
                filterValueVec.push_back(std::move(fv));
            }
            else if (it == "and" || it == "not" || it == "or")
            {
                FilterValue fv;
                fv.type = FilterType::Logical;
                fv.s = it;
                filterValueVec.push_back(std::move(fv));
            }
            else if (it == "true" || it == "false")
            {
                FilterValue fv;
                fv.type = FilterType::Boolean;
                fv.b = it == "true" ? true : false;
                filterValueVec.push_back(std::move(fv));
            }
            else if (it[0] == '\'' && it[it.size() - 1] == '\'' &&
                     it.size() >= 2)
            {
                FilterValue fv;
                fv.type = FilterType::String;
                fv.s = it.substr(1, it.size() - 2);
                filterValueVec.push_back(std::move(fv));
            }
            else if (isNumber(it))
            {
                if (it.find('.') == std::string::npos)
                {
                    int value;
                    auto result = std::from_chars(it.data(),
                                                  it.data() + it.size(), value);
                    if (result.ec != std::errc())
                    {
                    }
                    else
                    {
                        FilterValue fv;
                        fv.type = FilterType::Integer;
                        fv.n = value;
                        filterValueVec.push_back(std::move(fv));
                    }
                }
                else
                {
                    char* end;
                    FilterValue fv;
                    fv.d = std::strtod(it.data(), &end);
                    fv.type = FilterType::Double;
                    filterValueVec.push_back(std::move(fv));
                }
            }
            else
            {
                // get value from json
                auto v = stringSplit(it, "/");
                if (!getJsonValue(v, j, filterValueVec))
                {
                    return PARAM_FORMATERROR;
                }
            }
        }
        // change to RPN
        std::stack<FilterValue> stackOpt;
        std::stack<FilterValue> stackRPN;
        for (auto& it : filterValueVec)
        {
            if (it.type == FilterType::String ||
                it.type == FilterType::Boolean ||
                it.type == FilterType::Integer || it.type == FilterType::Double)
            {
                stackRPN.push(it);
            }
            else if (it.type == FilterType::Grouping)
            {
                if (it.s == "(")
                {
                    stackOpt.push(it);
                }
                else
                {
                    while (!stackOpt.empty())
                    {
                        FilterValue fv = stackOpt.top();
                        if (fv.type != FilterType::Grouping)
                        {
                            stackRPN.push(fv);
                            stackOpt.pop();
                        }
                        else if (fv.s == "(")
                        {
                            stackOpt.pop();
                            break;
                        }
                    }
                }
            }
            else
            {
                while (true)
                {
                    if (stackOpt.empty())
                    {
                        stackOpt.push(it);
                        break;
                    }
                    FilterValue fv = stackOpt.top();
                    if (fv.s == "(")
                    {
                        stackOpt.push(it);
                        break;
                    }
                    if (getOperatorPrecedence(it.s) >
                        getOperatorPrecedence(fv.s))
                    {
                        stackOpt.push(it);
                        break;
                    }
                    stackRPN.push(fv);
                    stackOpt.pop();
                }
            }
        }
        while (!stackOpt.empty())
        {
            FilterValue fv = stackOpt.top();
            stackRPN.push(fv);
            stackOpt.pop();
        }
        filterValueVec.clear();
        while (!stackRPN.empty())
        {
            stackOpt.push(stackRPN.top());
            stackRPN.pop();
        }
        // use RPN to calc
        while (!stackOpt.empty())
        {
            FilterValue fv = stackOpt.top();
            stackOpt.pop();
            if (fv.type == FilterType::Double ||
                fv.type == FilterType::Integer ||
                fv.type == FilterType::Boolean || fv.type == FilterType::String)
            {
                stackRPN.push(fv);
            }
            else if (fv.s == "and" && stackRPN.size() >= 2)
            {
                FilterValue fv1 = stackRPN.top();
                stackRPN.pop();
                FilterValue fv2 = stackRPN.top();
                stackRPN.pop();
                if (fv1.type == FilterType::Boolean &&
                    fv2.type == FilterType::Boolean)
                {
                    FilterValue fv;
                    fv.type = FilterType::Boolean;
                    fv.b = fv1.b && fv2.b;
                    stackRPN.push(fv);
                }
            }
            else if (fv.s == "or" && stackRPN.size() >= 2)
            {
                FilterValue fv1 = stackRPN.top();
                stackRPN.pop();
                FilterValue fv2 = stackRPN.top();
                stackRPN.pop();
                if (fv1.type == FilterType::Boolean &&
                    fv2.type == FilterType::Boolean)
                {
                    FilterValue fv;
                    fv.type = FilterType::Boolean;
                    fv.b = fv1.b || fv2.b;
                    stackRPN.push(fv);
                }
            }
            else if (fv.s == "not" && stackRPN.size() >= 1)
            {
                FilterValue fv1 = stackRPN.top();
                stackRPN.pop();
                if (fv1.type == FilterType::Boolean)
                {
                    FilterValue fv;
                    fv.type = FilterType::Boolean;
                    fv.b = !fv1.b;
                    stackRPN.push(fv);
                }
            }
            else if (fv.s == "eq" && stackRPN.size() >= 2)
            {
                FilterValue fv1 = stackRPN.top();
                stackRPN.pop();
                FilterValue fv2 = stackRPN.top();
                stackRPN.pop();
                if (fv1.type == fv2.type)
                {
                    FilterValue fv;
                    fv.type = FilterType::Boolean;
                    if (fv1.type == FilterType::Integer)
                    {
                        fv.b = fv1.n == fv2.n;
                    }
                    else if (fv1.type == FilterType::Boolean)
                    {
                        fv.b = fv1.b == fv2.b;
                    }
                    else if (fv1.type == FilterType::String)
                    {
                        fv.b = fv1.s == fv2.s;
                    }
                    stackRPN.push(fv);
                }
            }
            else if (fv.s == "ne" && stackRPN.size() >= 2)
            {
                FilterValue fv1 = stackRPN.top();
                stackRPN.pop();
                FilterValue fv2 = stackRPN.top();
                stackRPN.pop();
                if (fv1.type == fv2.type)
                {
                    FilterValue fv;
                    fv.type = FilterType::Boolean;
                    if (fv1.type == FilterType::Integer)
                    {
                        fv.b = fv1.n != fv2.n;
                    }
                    else if (fv1.type == FilterType::Boolean)
                    {
                        fv.b = fv1.b != fv2.b;
                    }
                    else if (fv1.type == FilterType::String)
                    {
                        fv.b = fv1.s != fv2.s;
                    }
                    stackRPN.push(fv);
                }
            }
            else if (fv.s == "ge" && stackRPN.size() >= 2)
            {
                FilterValue fv1 = stackRPN.top();
                stackRPN.pop();
                FilterValue fv2 = stackRPN.top();
                stackRPN.pop();
                if ((fv1.type == FilterType::Integer ||
                     fv1.type == FilterType::Double) &&
                    (fv2.type == FilterType::Integer ||
                     fv2.type == FilterType::Double))
                {
                    FilterValue fv;
                    fv.type = FilterType::Boolean;
                    if (fv1.type == FilterType::Integer &&
                        fv2.type == FilterType::Integer)
                    {
                        fv.b = fv1.n >= fv2.n;
                    }
                    else if (fv1.type == FilterType::Double &&
                             fv2.type == FilterType::Double)
                    {
                        fv.b = fv1.d >= fv2.d;
                    }
                    else if (fv1.type == FilterType::Double &&
                             fv2.type == FilterType::Integer)
                    {
                        fv.b = fv1.d >= fv2.n;
                    }
                    else if (fv1.type == FilterType::Integer &&
                             fv2.type == FilterType::Double)
                    {
                        fv.b = fv1.n >= fv2.d;
                    }
                    stackRPN.push(fv);
                }
            }
            else if (fv.s == "gt" && stackRPN.size() >= 2)
            {
                FilterValue fv1 = stackRPN.top();
                stackRPN.pop();
                FilterValue fv2 = stackRPN.top();
                stackRPN.pop();
                if ((fv1.type == FilterType::Integer ||
                     fv1.type == FilterType::Double) &&
                    (fv2.type == FilterType::Integer ||
                     fv2.type == FilterType::Double))
                {
                    FilterValue fv;
                    fv.type = FilterType::Boolean;
                    if (fv1.type == FilterType::Integer &&
                        fv2.type == FilterType::Integer)
                    {
                        fv.b = fv1.n > fv2.n;
                    }
                    else if (fv1.type == FilterType::Double &&
                             fv2.type == FilterType::Double)
                    {
                        fv.b = fv1.d > fv2.d;
                    }
                    else if (fv1.type == FilterType::Double &&
                             fv2.type == FilterType::Integer)
                    {
                        fv.b = fv1.d > fv2.n;
                    }
                    else if (fv1.type == FilterType::Integer &&
                             fv2.type == FilterType::Double)
                    {
                        fv.b = fv1.n > fv2.d;
                    }
                    stackRPN.push(fv);
                }
            }
            else if (fv.s == "le" && stackRPN.size() >= 2)
            {
                FilterValue fv1 = stackRPN.top();
                stackRPN.pop();
                FilterValue fv2 = stackRPN.top();
                stackRPN.pop();
                if ((fv1.type == FilterType::Integer ||
                     fv1.type == FilterType::Double) &&
                    (fv2.type == FilterType::Integer ||
                     fv2.type == FilterType::Double))
                {
                    FilterValue fv;
                    fv.type = FilterType::Boolean;
                    if (fv1.type == FilterType::Integer &&
                        fv2.type == FilterType::Integer)
                    {
                        fv.b = fv1.n <= fv2.n;
                    }
                    else if (fv1.type == FilterType::Double &&
                             fv2.type == FilterType::Double)
                    {
                        fv.b = fv1.d <= fv2.d;
                    }
                    else if (fv1.type == FilterType::Double &&
                             fv2.type == FilterType::Integer)
                    {
                        fv.b = fv1.d <= fv2.n;
                    }
                    else if (fv1.type == FilterType::Integer &&
                             fv2.type == FilterType::Double)
                    {
                        fv.b = fv1.n <= fv2.d;
                    }
                    stackRPN.push(fv);
                }
            }
            else if (fv.s == "lt" && stackRPN.size() >= 2)
            {
                FilterValue fv1 = stackRPN.top();
                stackRPN.pop();
                FilterValue fv2 = stackRPN.top();
                stackRPN.pop();
                if ((fv1.type == FilterType::Integer ||
                     fv1.type == FilterType::Double) &&
                    (fv2.type == FilterType::Integer ||
                     fv2.type == FilterType::Double))
                {
                    FilterValue fv;
                    fv.type = FilterType::Boolean;
                    if (fv1.type == FilterType::Integer &&
                        fv2.type == FilterType::Integer)
                    {
                        fv.b = fv1.n < fv2.n;
                    }
                    else if (fv1.type == FilterType::Double &&
                             fv2.type == FilterType::Double)
                    {
                        fv.b = fv1.d < fv2.d;
                    }
                    else if (fv1.type == FilterType::Double &&
                             fv2.type == FilterType::Integer)
                    {
                        fv.b = fv1.d < fv2.n;
                    }
                    else if (fv1.type == FilterType::Integer &&
                             fv2.type == FilterType::Double)
                    {
                        fv.b = fv1.n < fv2.d;
                    }
                    stackRPN.push(fv);
                }
            }
            else
            {
                return PARAM_FORMATERROR;
            }
        }
        if (stackRPN.size() == 1)
        {
            FilterValue fv = stackRPN.top();
            if (fv.type == FilterType::Boolean)
            {
                if (fv.b)
                {
                    return PARAM_NOACTION;
                }
                return PARAM_ERASE;
            }
        }
        return PARAM_FORMATERROR;
    }

    bool getJsonValue(std::vector<std::string>& v, nlohmann::json& j,
                      std::vector<FilterValue>& dstV)
    {
        if (v.empty())
        {
            return true;
        }
        std::string key = v[0];
        auto it = j.find(key);
        if (it == j.end())
        {
            return false;
        }
        v.erase(v.begin());
        if (v.empty())
        {
            if (it->is_string())
            {
                FilterValue fv;
                fv.type = FilterType::String;
                fv.s = it->get<std::string>();
                dstV.push_back(std::move(fv));
                return true;
            }
            if (it->is_boolean())
            {
                FilterValue fv;
                fv.type = FilterType::Boolean;
                fv.b = it->get<bool>();
                dstV.push_back(std::move(fv));
                return true;
            }
            if (it->is_number_float())
            {
                FilterValue fv;
                fv.type = FilterType::Double;
                fv.d = it->get<float>();
                dstV.push_back(std::move(fv));
                return true;
            }
            if (it->is_number())
            {
                FilterValue fv;
                fv.type = FilterType::Integer;
                fv.n = it->get<int>();
                dstV.push_back(std::move(fv));
                return true;
            }
            return false;
        }
        return getJsonValue(v, *it, dstV);
    }

    uint8_t getOperatorPrecedence(const std::string& opt)
    {
        if (opt == "not")
        {
            return 5;
        }
        if (opt == "gt" || opt == "ge" || opt == "lt" || opt == "le")
        {
            return 4;
        }
        if (opt == "eq" || opt == "ne")
        {
            return 3;
        }
        if (opt == "and")
        {
            return 2;
        }
        if (opt == "or")
        {
            return 1;
        }
        return 0;
    }

    bool isNumber(const std::string& str)
    {
        if (str.empty())
        {
            return false;
        }
        for (size_t i = 0; i < str.size(); i++)
        {
            if (i == 0 && str[i] == '-')
            {
                continue;
            }
            if (i == 0 && str[i] == '.')
            {
                return false;
            }
            if ((str[i] < '0' || str[i] > '9') && str[i] != '.')
            {
                return false;
            }
        }
        return true;
    }

  private:
    App& app;
    crow::Response& res;
    std::optional<crow::Request> newReq;
    bool isParsed = false;
    bool isOnly = false;
    bool isExpand = false;
    bool isSelect = false;
    bool isSkip = false;
    bool isTop = false;
    bool isFilter = false;
    uint64_t expandLevel;
    const uint64_t maxLevel = 2;
    uint64_t skipValue = 0;
    uint64_t topValue = 0;
    std::string expandType;
    std::string filterValue;
    std::vector<std::string> pendingUrlVec;
    std::vector<std::vector<std::string>> selectPropertyVec;
    std::vector<std::string> filterVec;
    nlohmann::json jsonValue;
};
} // namespace query_param
} // namespace redfish
