#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "routing.hpp"

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
const uint64_t expandMaxLevel = 2;  // todo
const uint64_t expandMinLenth = 12; //*($levels=1)
struct Query
{
    bool isOnly = false;
    bool isExpand = false;
    bool isSelect = false;
    bool isSkip = false;
    bool isTop = false;
    bool isFilter = false;
    uint64_t expandLevel;
    std::string expandType;
    std::vector<std::string> pendingUrlVec;
    std::vector<std::vector<std::string>> selectPropertyVec;
    uint64_t skipValue;
    uint64_t topValue;
    std::string filterValue;
    std::vector<std::string> filterVec;
    nlohmann::json jsonValue;
};

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

void processAllParams(crow::App& app, Query& query,
                      crow::Response& intermediateResponse,
                      std::function<void(crow::Response&)>& completionHandler);

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
        else if (it.key() == "$select")
        {
            ret.isSelect = true;
            std::string value = std::string(it.value());
            auto result = stringSplit(value, ",");
            for (auto& itResult : result)
            {
                auto v = stringSplit(itResult, "/");
                if (v.empty())
                {
                    continue;
                }
                ret.selectPropertyVec.push_back(std::move(v));
            }
        }
        else if (it.key() == "$skip")
        {
            ret.skipValue = strtoul(it.value().c_str(), nullptr, 10);
            if (ret.skipValue > 0)
            {
                ret.isSkip = true;
            }
        }
        else if (it.key() == "$top")
        {
            ret.topValue = strtoul(it.value().c_str(), nullptr, 10);
            if (ret.topValue > 0)
            {
                ret.isTop = true;
            }
        }
        else if (it.key() == "$filter")
        {
            std::string value = it.value();
            if (value.empty())
            {
                messages::queryParameterValueFormatError(res, it.value(),
                                                         it.key());
                return std::nullopt;
            }

            ret.filterValue = value;
            value.erase(0, value.find_first_not_of(' '));
            value.erase(value.find_last_not_of(' ') + 1);

            std::string result{};
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
                        else if (i < value.length() - 1 && value[i + 1] != ' ')
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
            ret.filterVec = stringSplit(result, " ");
            ret.isFilter = true;
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

void recursiveSelect(std::vector<std::string> v, nlohmann::json& src,
                     nlohmann::json& dst)
{
    if (v.empty())
    {
        return;
    }
    if (src.is_array())
    {
        if (src.size() != dst.size())
        {
            return;
        }
        for (uint32_t i = 0; i < src.size(); i++)
        {
            recursiveSelect(v, src[i], dst[i]);
        }
        return;
    }
    std::string key = v[0];
    auto it = src.find(key);
    if (it != src.end())
    {
        if (v.size() == 1)
        {
            dst[key] = *it;
        }
        else
        {
            v.erase(v.begin());
            if (it->is_array())
            {
                if (dst.find(key) == dst.end())
                {
                    dst[key] = nlohmann::json::array();
                    for (uint32_t i = 0; i < it->size(); i++)
                    {
                        dst[key].push_back(nlohmann::json());
                    }
                }
            }
            else
            {
                if (dst.find(key) == dst.end())
                {
                    dst[key] = nlohmann::json::object();
                }
            }
            recursiveSelect(v, *it, dst[key]);
        }
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

inline bool getJsonValueForFilter(std::vector<std::string>& v,
                                  nlohmann::json& j,
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
    return getJsonValueForFilter(v, *it, dstV);
}

inline uint8_t getFilterOperatorPrecedence(const std::string& opt)
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

inline bool isNumber(const std::string& str)
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

inline int8_t parseFilterParameters(nlohmann::json& j, Query& query)
{
    std::vector<FilterValue> filterValueVec;
    for (auto& it : query.filterVec)
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
        else if (it[0] == '\'' && it[it.size() - 1] == '\'' && it.size() >= 2)
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
                auto result =
                    std::from_chars(it.data(), it.data() + it.size(), value);
                if (result.ec != std::errc())
                {
                    return PARAM_FORMATERROR;
                }
                FilterValue fv;
                fv.type = FilterType::Integer;
                fv.n = value;
                filterValueVec.push_back(std::move(fv));
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
            if (!getJsonValueForFilter(v, j, filterValueVec))
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
        if (it.type == FilterType::String || it.type == FilterType::Boolean ||
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
                if (getFilterOperatorPrecedence(it.s) >
                    getFilterOperatorPrecedence(fv.s))
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
        if (fv.type == FilterType::Double || fv.type == FilterType::Integer ||
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

inline void processSelect(crow::Response& res, Query& query)
{
    auto jsonValue = nlohmann::json::object();
    for (auto& it : query.selectPropertyVec)
    {
        recursiveSelect(it, res.jsonValue, jsonValue);
    }
    res.jsonValue = jsonValue;
}

inline void processSkip(crow::Response& res, Query& query)
{
    auto it = res.jsonValue.find("Members");
    if (it == res.jsonValue.end())
    {
        return;
    }

    if (it->size() <= query.skipValue)
    {
        it->clear();
        return;
    }
    it->erase(it->begin(), it->begin() + int(query.skipValue));
}

inline void processTop(crow::Response& res, Query& query)
{
    auto it = res.jsonValue.find("Members");
    if (it == res.jsonValue.end())
    {
        return;
    }
    if (it->size() <= query.topValue)
    {
        return;
    }
    it->erase(it->begin() + int(query.topValue), it->end());
}

inline void processFilter(crow::Response& res, Query& query)
{
    auto itMembers = res.jsonValue.find("Members");
    if (itMembers == res.jsonValue.end())
    {
        return;
    }
    for (auto it = itMembers->begin(); it != itMembers->end();)
    {
        auto ret = parseFilterParameters(*it, query);
        if (ret == PARAM_ERASE)
        {
            it = itMembers->erase(it);
            continue;
        }
        if (ret == PARAM_FORMATERROR)
        {
            messages::queryParameterValueFormatError(res, query.filterValue,
                                                     "$filter");
            return;
        }
        it++;
    }
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
    if (query.isSelect)
    {
        processSelect(intermediateResponse, query);
    }
    if (query.isSkip)
    {
        processSkip(intermediateResponse, query);
    }
    if (query.isTop)
    {
        processTop(intermediateResponse, query);
    }
    if (query.isFilter)
    {
        processFilter(intermediateResponse, query);
    }
    completionHandler(intermediateResponse);
}
} // namespace query_param
} // namespace redfish
