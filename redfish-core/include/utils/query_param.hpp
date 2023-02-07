#pragma once
#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "logging.hpp"
#include "str_utility.hpp"

#include <sys/types.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/beast/http/message.hpp> // IWYU pragma: keep
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/url/params_view.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <compare>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

// IWYU pragma: no_include <boost/url/impl/params_view.hpp>
// IWYU pragma: no_include <boost/beast/http/impl/message.hpp>
// IWYU pragma: no_include <boost/intrusive/detail/list_iterator.hpp>
// IWYU pragma: no_include <boost/algorithm/string/detail/classification.hpp>
// IWYU pragma: no_include <boost/iterator/iterator_facade.hpp>
// IWYU pragma: no_include <boost/type_index/type_index_facade.hpp>
// IWYU pragma: no_include <stdint.h>

namespace redfish
{
namespace query_param
{

enum class ExpandType : uint8_t
{
    None,
    Links,
    NotLinks,
    Both,
};

// A simple implementation of Trie to help |recursiveSelect|.
class SelectTrieNode
{
  public:
    SelectTrieNode() = default;

    const SelectTrieNode* find(const std::string& jsonKey) const
    {
        auto it = children.find(jsonKey);
        if (it == children.end())
        {
            return nullptr;
        }
        return &it->second;
    }

    // Creates a new node if the key doesn't exist, returns the reference to the
    // newly created node; otherwise, return the reference to the existing node
    SelectTrieNode* emplace(std::string_view jsonKey)
    {
        auto [it, _] = children.emplace(jsonKey, SelectTrieNode{});
        return &it->second;
    }

    bool empty() const
    {
        return children.empty();
    }

    void clear()
    {
        children.clear();
    }

    void setToSelected()
    {
        selected = true;
    }

    bool isSelected() const
    {
        return selected;
    }

  private:
    std::map<std::string, SelectTrieNode, std::less<>> children;
    bool selected = false;
};

// Validates the property in the $select parameter. Every character is among
// [a-zA-Z0-9#@_.] (taken from Redfish spec, section 9.6 Properties)
inline bool isSelectedPropertyAllowed(std::string_view property)
{
    // These a magic number, but with it it's less likely that this code
    // introduces CVE; e.g., too large properties crash the service.
    constexpr int maxPropertyLength = 60;
    if (property.empty() || property.size() > maxPropertyLength)
    {
        return false;
    }
    for (char ch : property)
    {
        if (std::isalnum(static_cast<unsigned char>(ch)) == 0 && ch != '#' &&
            ch != '@' && ch != '.')
        {
            return false;
        }
    }
    return true;
}

struct SelectTrie
{
    SelectTrie() = default;

    // Inserts a $select value; returns false if the nestedProperty is illegal.
    bool insertNode(std::string_view nestedProperty)
    {
        if (nestedProperty.empty())
        {
            return false;
        }
        SelectTrieNode* currNode = &root;
        size_t index = nestedProperty.find_first_of('/');
        while (!nestedProperty.empty())
        {
            std::string_view property = nestedProperty.substr(0, index);
            if (!isSelectedPropertyAllowed(property))
            {
                return false;
            }
            currNode = currNode->emplace(property);
            if (index == std::string::npos)
            {
                break;
            }
            nestedProperty.remove_prefix(index + 1);
            index = nestedProperty.find_first_of('/');
        }
        currNode->setToSelected();
        return true;
    }

    SelectTrieNode root;
};

// The struct stores the parsed query parameters of the default Redfish route.
struct Query
{
    // Only
    bool isOnly = false;
    // Expand
    uint8_t expandLevel = 0;
    ExpandType expandType = ExpandType::None;

    // Skip
    std::optional<size_t> skip = std::nullopt;

    // Top
    static constexpr size_t maxTop = 1000; // Max entries a response contain
    std::optional<size_t> top = std::nullopt;

    // Select
    SelectTrie selectTrie = {};
};

// The struct defines how resource handlers in redfish-core/lib/ can handle
// query parameters themselves, so that the default Redfish route will delegate
// the processing.
struct QueryCapabilities
{
    bool canDelegateOnly = false;
    bool canDelegateTop = false;
    bool canDelegateSkip = false;
    uint8_t canDelegateExpandLevel = 0;
    bool canDelegateSelect = false;
};

// Delegates query parameters according to the given |queryCapabilities|
// This function doesn't check query parameter conflicts since the parse
// function will take care of it.
// Returns a delegated query object which can be used by individual resource
// handlers so that handlers don't need to query again.
inline Query delegate(const QueryCapabilities& queryCapabilities, Query& query)
{
    Query delegated;
    // delegate only
    if (query.isOnly && queryCapabilities.canDelegateOnly)
    {
        delegated.isOnly = true;
        query.isOnly = false;
    }
    // delegate expand as much as we can
    if (query.expandType != ExpandType::None)
    {
        delegated.expandType = query.expandType;
        if (query.expandLevel <= queryCapabilities.canDelegateExpandLevel)
        {
            query.expandType = ExpandType::None;
            delegated.expandLevel = query.expandLevel;
            query.expandLevel = 0;
        }
        else
        {
            query.expandLevel -= queryCapabilities.canDelegateExpandLevel;
            delegated.expandLevel = queryCapabilities.canDelegateExpandLevel;
        }
    }

    // delegate top
    if (query.top && queryCapabilities.canDelegateTop)
    {
        delegated.top = query.top;
        query.top = std::nullopt;
    }

    // delegate skip
    if (query.skip && queryCapabilities.canDelegateSkip)
    {
        delegated.skip = query.skip;
        query.skip = 0;
    }

    // delegate select
    if (!query.selectTrie.root.empty() && queryCapabilities.canDelegateSelect)
    {
        delegated.selectTrie = std::move(query.selectTrie);
        query.selectTrie.root.clear();
    }
    return delegated;
}

inline bool getExpandType(std::string_view value, Query& query)
{
    if (value.empty())
    {
        return false;
    }
    switch (value[0])
    {
        case '*':
            query.expandType = ExpandType::Both;
            break;
        case '.':
            query.expandType = ExpandType::NotLinks;
            break;
        case '~':
            query.expandType = ExpandType::Links;
            break;
        default:
            return false;

            break;
    }
    value.remove_prefix(1);
    if (value.empty())
    {
        query.expandLevel = 1;
        return true;
    }
    constexpr std::string_view levels = "($levels=";
    if (!value.starts_with(levels))
    {
        return false;
    }
    value.remove_prefix(levels.size());

    auto it = std::from_chars(value.data(), value.data() + value.size(),
                              query.expandLevel);
    if (it.ec != std::errc())
    {
        return false;
    }
    value.remove_prefix(static_cast<size_t>(it.ptr - value.data()));
    return value == ")";
}

enum class QueryError
{
    Ok,
    OutOfRange,
    ValueFormat,
};

inline QueryError getNumericParam(std::string_view value, size_t& param)
{
    std::from_chars_result r =
        std::from_chars(value.data(), value.data() + value.size(), param);

    // If the number wasn't representable in the type, it's out of range
    if (r.ec == std::errc::result_out_of_range)
    {
        return QueryError::OutOfRange;
    }
    // All other errors are value format
    if (r.ec != std::errc())
    {
        return QueryError::ValueFormat;
    }
    return QueryError::Ok;
}

inline QueryError getSkipParam(std::string_view value, Query& query)
{
    return getNumericParam(value, query.skip.emplace());
}

inline QueryError getTopParam(std::string_view value, Query& query)
{
    QueryError ret = getNumericParam(value, query.top.emplace());
    if (ret != QueryError::Ok)
    {
        return ret;
    }

    // Range check for sanity.
    if (query.top > Query::maxTop)
    {
        return QueryError::OutOfRange;
    }

    return QueryError::Ok;
}

// Parses and validates the $select parameter.
// As per OData URL Conventions and Redfish Spec, the $select values shall be
// comma separated Resource Path
// Ref:
// 1. https://datatracker.ietf.org/doc/html/rfc3986#section-3.3
// 2.
// https://docs.oasis-open.org/odata/odata/v4.01/os/abnf/odata-abnf-construction-rules.txt
inline bool getSelectParam(std::string_view value, Query& query)
{
    std::vector<std::string> properties;
    bmcweb::split(properties, value, ',');
    if (properties.empty())
    {
        return false;
    }
    // These a magic number, but with it it's less likely that this code
    // introduces CVE; e.g., too large properties crash the service.
    constexpr int maxNumProperties = 10;
    if (properties.size() > maxNumProperties)
    {
        return false;
    }
    for (const auto& property : properties)
    {
        if (!query.selectTrie.insertNode(property))
        {
            return false;
        }
    }
    return true;
}

inline std::optional<Query> parseParameters(boost::urls::params_view urlParams,
                                            crow::Response& res)
{
    Query ret;
    for (const boost::urls::params_view::value_type& it : urlParams)
    {
        if (it.key == "only")
        {
            if (!it.value.empty())
            {
                messages::queryParameterValueFormatError(res, it.value, it.key);
                return std::nullopt;
            }
            ret.isOnly = true;
        }
        else if (it.key == "$expand" && bmcwebInsecureEnableQueryParams)
        {
            if (!getExpandType(it.value, ret))
            {
                messages::queryParameterValueFormatError(res, it.value, it.key);
                return std::nullopt;
            }
        }
        else if (it.key == "$top")
        {
            QueryError topRet = getTopParam(it.value, ret);
            if (topRet == QueryError::ValueFormat)
            {
                messages::queryParameterValueFormatError(res, it.value, it.key);
                return std::nullopt;
            }
            if (topRet == QueryError::OutOfRange)
            {
                messages::queryParameterOutOfRange(
                    res, it.value, "$top",
                    "0-" + std::to_string(Query::maxTop));
                return std::nullopt;
            }
        }
        else if (it.key == "$skip")
        {
            QueryError topRet = getSkipParam(it.value, ret);
            if (topRet == QueryError::ValueFormat)
            {
                messages::queryParameterValueFormatError(res, it.value, it.key);
                return std::nullopt;
            }
            if (topRet == QueryError::OutOfRange)
            {
                messages::queryParameterOutOfRange(
                    res, it.value, it.key,
                    "0-" + std::to_string(std::numeric_limits<size_t>::max()));
                return std::nullopt;
            }
        }
        else if (it.key == "$select")
        {
            if (!getSelectParam(it.value, ret))
            {
                messages::queryParameterValueFormatError(res, it.value, it.key);
                return std::nullopt;
            }
        }
        else
        {
            // Intentionally ignore other errors Redfish spec, 7.3.1
            if (it.key.starts_with("$"))
            {
                // Services shall return... The HTTP 501 Not Implemented
                // status code for any unsupported query parameters that
                // start with $ .
                messages::queryParameterValueFormatError(res, it.value, it.key);
                res.result(boost::beast::http::status::not_implemented);
                return std::nullopt;
            }
            // "Shall ignore unknown or unsupported query parameters that do
            // not begin with $ ."
        }
    }

    if (ret.expandType != ExpandType::None && !ret.selectTrie.root.empty())
    {
        messages::queryCombinationInvalid(res);
        return std::nullopt;
    }

    return ret;
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
    if (url == nullptr)
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

struct ExpandNode
{
    nlohmann::json::json_pointer location;
    std::string uri;

    inline bool operator==(const ExpandNode& other) const
    {
        return location == other.location && uri == other.uri;
    }
};

// Walks a json object looking for Redfish NavigationReference entries that
// might need resolved.  It recursively walks the jsonResponse object, looking
// for links at every level, and returns a list (out) of locations within the
// tree that need to be expanded.  The current json pointer location p is passed
// in to reference the current node that's being expanded, so it can be combined
// with the keys from the jsonResponse object
inline void findNavigationReferencesRecursive(
    ExpandType eType, nlohmann::json& jsonResponse,
    const nlohmann::json::json_pointer& p, int depth, bool inLinks,
    std::vector<ExpandNode>& out)
{
    // If no expand is needed, return early
    if (eType == ExpandType::None)
    {
        return;
    }

    nlohmann::json::array_t* array =
        jsonResponse.get_ptr<nlohmann::json::array_t*>();
    if (array != nullptr)
    {
        size_t index = 0;
        // For arrays, walk every element in the array
        for (auto& element : *array)
        {
            nlohmann::json::json_pointer newPtr = p / index;
            BMCWEB_LOG_DEBUG << "Traversing response at " << newPtr.to_string();
            findNavigationReferencesRecursive(eType, element, newPtr, depth,
                                              inLinks, out);
            index++;
        }
    }
    nlohmann::json::object_t* obj =
        jsonResponse.get_ptr<nlohmann::json::object_t*>();
    if (obj == nullptr)
    {
        return;
    }
    // Navigation References only ever have a single element
    if (obj->size() == 1)
    {
        if (obj->begin()->first == "@odata.id")
        {
            const std::string* uri =
                obj->begin()->second.get_ptr<const std::string*>();
            if (uri != nullptr)
            {
                BMCWEB_LOG_DEBUG << "Found " << *uri << " at " << p.to_string();
                out.push_back({p, *uri});
                return;
            }
        }
    }

    int newDepth = depth;
    auto odataId = obj->find("@odata.id");
    if (odataId != obj->end())
    {
        // The Redfish spec requires all resources to include the resource
        // identifier.  If the object has multiple elements and one of them is
        // "@odata.id" then that means we have entered a new level / expanded
        // resource.  We need to stop traversing if we're already at the desired
        // depth
        if ((obj->size() > 1) && (depth == 0))
        {
            return;
        }
        newDepth--;
    }

    // Loop the object and look for links
    for (auto& element : *obj)
    {
        bool localInLinks = inLinks;
        if (!localInLinks)
        {
            // Check if this is a links node
            localInLinks = element.first == "Links";
        }
        // Only traverse the parts of the tree the user asked for
        // Per section 7.3 of the redfish specification
        if (localInLinks && eType == ExpandType::NotLinks)
        {
            continue;
        }
        if (!localInLinks && eType == ExpandType::Links)
        {
            continue;
        }
        nlohmann::json::json_pointer newPtr = p / element.first;
        BMCWEB_LOG_DEBUG << "Traversing response at " << newPtr;

        findNavigationReferencesRecursive(eType, element.second, newPtr,
                                          newDepth, localInLinks, out);
    }
}

// TODO: When aggregation is enabled and we receive a partially expanded
// response we may need need additional handling when the original URI was
// up tree from a top level collection.
// Isn't a concern until https://gerrit.openbmc.org/c/openbmc/bmcweb/+/60556
// lands.  May want to avoid forwarding query params when request is uptree from
// a top level collection.
inline std::vector<ExpandNode>
    findNavigationReferences(ExpandType eType, int depth,
                             nlohmann::json& jsonResponse)
{
    std::vector<ExpandNode> ret;
    const nlohmann::json::json_pointer root = nlohmann::json::json_pointer("");
    findNavigationReferencesRecursive(eType, jsonResponse, root, depth, false,
                                      ret);
    return ret;
}

// Formats a query parameter string for the sub-query.
// Returns std::nullopt on failures.
// This function shall handle $select when it is added.
// There is no need to handle parameters that's not campatible with $expand,
// e.g., $only, since this function will only be called in side $expand handlers
inline std::optional<std::string> formatQueryForExpand(const Query& query)
{
    // query.expandLevel<=1: no need to do subqueries
    if (query.expandLevel <= 1)
    {
        return "";
    }
    std::string str = "?$expand=";
    bool queryTypeExpected = false;
    switch (query.expandType)
    {
        case ExpandType::None:
            return "";
        case ExpandType::Links:
            queryTypeExpected = true;
            str += '~';
            break;
        case ExpandType::NotLinks:
            queryTypeExpected = true;
            str += '.';
            break;
        case ExpandType::Both:
            queryTypeExpected = true;
            str += '*';
            break;
    }
    if (!queryTypeExpected)
    {
        return std::nullopt;
    }
    str += "($levels=";
    str += std::to_string(query.expandLevel - 1);
    str += ')';
    return str;
}

// Propogates the worst error code to the final response.
// The order of error code is (from high to low)
// 500 Internal Server Error
// 511 Network Authentication Required
// 510 Not Extended
// 508 Loop Detected
// 507 Insufficient Storage
// 506 Variant Also Negotiates
// 505 HTTP Version Not Supported
// 504 Gateway Timeout
// 503 Service Unavailable
// 502 Bad Gateway
// 501 Not Implemented
// 401 Unauthorized
// 451 - 409 Error codes (not listed explictly)
// 408 Request Timeout
// 407 Proxy Authentication Required
// 406 Not Acceptable
// 405 Method Not Allowed
// 404 Not Found
// 403 Forbidden
// 402 Payment Required
// 400 Bad Request
inline unsigned propogateErrorCode(unsigned finalCode, unsigned subResponseCode)
{
    // We keep a explicit list for error codes that this project often uses
    // Higer priority codes are in lower indexes
    constexpr std::array<unsigned, 13> orderedCodes = {
        500, 507, 503, 502, 501, 401, 412, 409, 406, 405, 404, 403, 400};
    size_t finalCodeIndex = std::numeric_limits<size_t>::max();
    size_t subResponseCodeIndex = std::numeric_limits<size_t>::max();
    for (size_t i = 0; i < orderedCodes.size(); ++i)
    {
        if (orderedCodes[i] == finalCode)
        {
            finalCodeIndex = i;
        }
        if (orderedCodes[i] == subResponseCode)
        {
            subResponseCodeIndex = i;
        }
    }
    if (finalCodeIndex != std::numeric_limits<size_t>::max() &&
        subResponseCodeIndex != std::numeric_limits<size_t>::max())
    {
        return finalCodeIndex <= subResponseCodeIndex ? finalCode
                                                      : subResponseCode;
    }
    if (subResponseCode == 500 || finalCode == 500)
    {
        return 500;
    }
    if (subResponseCode > 500 || finalCode > 500)
    {
        return std::max(finalCode, subResponseCode);
    }
    if (subResponseCode == 401)
    {
        return subResponseCode;
    }
    return std::max(finalCode, subResponseCode);
}

// Propogates all error messages into |finalResponse|
inline void propogateError(crow::Response& finalResponse,
                           crow::Response& subResponse)
{
    // no errors
    if (subResponse.resultInt() >= 200 && subResponse.resultInt() < 400)
    {
        return;
    }
    messages::moveErrorsToErrorJson(finalResponse.jsonValue,
                                    subResponse.jsonValue);
    finalResponse.result(
        propogateErrorCode(finalResponse.resultInt(), subResponse.resultInt()));
}

class MultiAsyncResp : public std::enable_shared_from_this<MultiAsyncResp>
{
  public:
    // This object takes a single asyncResp object as the "final" one, then
    // allows callers to attach sub-responses within the json tree that need
    // to be executed and filled into their appropriate locations.  This
    // class manages the final "merge" of the json resources.
    MultiAsyncResp(crow::App& appIn,
                   std::shared_ptr<bmcweb::AsyncResp> finalResIn) :
        app(appIn),
        finalRes(std::move(finalResIn))
    {}

    void addAwaitingResponse(
        const std::shared_ptr<bmcweb::AsyncResp>& res,
        const nlohmann::json::json_pointer& finalExpandLocation)
    {
        res->res.setCompleteRequestHandler(std::bind_front(
            placeResultStatic, shared_from_this(), finalExpandLocation));
    }

    void placeResult(const nlohmann::json::json_pointer& locationToPlace,
                     crow::Response& res)
    {
        BMCWEB_LOG_DEBUG << "placeResult for " << locationToPlace;
        propogateError(finalRes->res, res);
        if (!res.jsonValue.is_object() || res.jsonValue.empty())
        {
            return;
        }
        nlohmann::json& finalObj = finalRes->res.jsonValue[locationToPlace];
        finalObj = std::move(res.jsonValue);
    }

    // Handles the very first level of Expand, and starts a chain of sub-queries
    // for deeper levels.
    void startQuery(const Query& query)
    {
        std::vector<ExpandNode> nodes = findNavigationReferences(
            query.expandType, query.expandLevel, finalRes->res.jsonValue);
        BMCWEB_LOG_DEBUG << nodes.size() << " nodes to traverse";
        const std::optional<std::string> queryStr = formatQueryForExpand(query);
        if (!queryStr)
        {
            messages::internalError(finalRes->res);
            return;
        }
        for (const ExpandNode& node : nodes)
        {
            const std::string subQuery = node.uri + *queryStr;
            BMCWEB_LOG_DEBUG << "URL of subquery:  " << subQuery;
            std::error_code ec;
            crow::Request newReq({boost::beast::http::verb::get, subQuery, 11},
                                 ec);
            if (ec)
            {
                messages::internalError(finalRes->res);
                return;
            }

            auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
            BMCWEB_LOG_DEBUG << "setting completion handler on "
                             << &asyncResp->res;

            addAwaitingResponse(asyncResp, node.location);
            app.handle(newReq, asyncResp);
        }
    }

  private:
    static void
        placeResultStatic(const std::shared_ptr<MultiAsyncResp>& multi,
                          const nlohmann::json::json_pointer& locationToPlace,
                          crow::Response& res)
    {
        multi->placeResult(locationToPlace, res);
    }

    crow::App& app;
    std::shared_ptr<bmcweb::AsyncResp> finalRes;
};

inline void processTopAndSkip(const Query& query, crow::Response& res)
{
    if (!query.skip && !query.top)
    {
        // No work to do.
        return;
    }
    nlohmann::json::object_t* obj =
        res.jsonValue.get_ptr<nlohmann::json::object_t*>();
    if (obj == nullptr)
    {
        // Shouldn't be possible.  All responses should be objects.
        messages::internalError(res);
        return;
    }

    BMCWEB_LOG_DEBUG << "Handling top/skip";
    nlohmann::json::object_t::iterator members = obj->find("Members");
    if (members == obj->end())
    {
        // From the Redfish specification 7.3.1
        // ... the HTTP 400 Bad Request status code with the
        // QueryNotSupportedOnResource message from the Base Message Registry
        // for any supported query parameters that apply only to resource
        // collections but are used on singular resources.
        messages::queryNotSupportedOnResource(res);
        return;
    }

    nlohmann::json::array_t* arr =
        members->second.get_ptr<nlohmann::json::array_t*>();
    if (arr == nullptr)
    {
        messages::internalError(res);
        return;
    }

    if (query.skip)
    {
        // Per section 7.3.1 of the Redfish specification, $skip is run before
        // $top Can only skip as many values as we have
        size_t skip = std::min(arr->size(), *query.skip);
        arr->erase(arr->begin(), arr->begin() + static_cast<ssize_t>(skip));
    }
    if (query.top)
    {
        size_t top = std::min(arr->size(), *query.top);
        arr->erase(arr->begin() + static_cast<ssize_t>(top), arr->end());
    }
}

// Given a JSON subtree |currRoot|, this function erases leaves whose keys are
// not in the |currNode| Trie node.
inline void recursiveSelect(nlohmann::json& currRoot,
                            const SelectTrieNode& currNode)
{
    nlohmann::json::object_t* object =
        currRoot.get_ptr<nlohmann::json::object_t*>();
    if (object != nullptr)
    {
        BMCWEB_LOG_DEBUG << "Current JSON is an object";
        auto it = currRoot.begin();
        while (it != currRoot.end())
        {
            auto nextIt = std::next(it);
            BMCWEB_LOG_DEBUG << "key=" << it.key();
            const SelectTrieNode* nextNode = currNode.find(it.key());
            // Per the Redfish spec section 7.3.3, the service shall select
            // certain properties as if $select was omitted. This applies to
            // every TrieNode that contains leaves and the root.
            constexpr std::array<std::string_view, 5> reservedProperties = {
                "@odata.id", "@odata.type", "@odata.context", "@odata.etag",
                "error"};
            bool reserved =
                std::find(reservedProperties.begin(), reservedProperties.end(),
                          it.key()) != reservedProperties.end();
            if (reserved || (nextNode != nullptr && nextNode->isSelected()))
            {
                it = nextIt;
                continue;
            }
            if (nextNode != nullptr)
            {
                BMCWEB_LOG_DEBUG << "Recursively select: " << it.key();
                recursiveSelect(*it, *nextNode);
                it = nextIt;
                continue;
            }
            BMCWEB_LOG_DEBUG << it.key() << " is getting removed!";
            it = currRoot.erase(it);
        }
    }
    nlohmann::json::array_t* array =
        currRoot.get_ptr<nlohmann::json::array_t*>();
    if (array != nullptr)
    {
        BMCWEB_LOG_DEBUG << "Current JSON is an array";
        // Array index is omitted, so reuse the same Trie node
        for (nlohmann::json& nextRoot : *array)
        {
            recursiveSelect(nextRoot, currNode);
        }
    }
}

// The current implementation of $select still has the following TODOs due to
//  ambiguity and/or complexity.
// 1. combined with $expand; https://github.com/DMTF/Redfish/issues/5058 was
// created for clarification.
// 2. respect the full odata spec; e.g., deduplication, namespace, star (*),
// etc.
inline void processSelect(crow::Response& intermediateResponse,
                          const SelectTrieNode& trieRoot)
{
    BMCWEB_LOG_DEBUG << "Process $select quary parameter";
    recursiveSelect(intermediateResponse.jsonValue, trieRoot);
}

inline void
    processAllParams(crow::App& app, const Query& query,
                     std::function<void(crow::Response&)>& completionHandler,
                     crow::Response& intermediateResponse)
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

    if (query.top || query.skip)
    {
        processTopAndSkip(query, intermediateResponse);
    }

    if (query.expandType != ExpandType::None)
    {
        BMCWEB_LOG_DEBUG << "Executing expand query";
        auto asyncResp = std::make_shared<bmcweb::AsyncResp>(
            std::move(intermediateResponse));

        asyncResp->res.setCompleteRequestHandler(std::move(completionHandler));
        auto multi = std::make_shared<MultiAsyncResp>(app, asyncResp);
        multi->startQuery(query);
        return;
    }

    // According to Redfish Spec Section 7.3.1, $select is the last parameter to
    // to process
    if (!query.selectTrie.root.empty())
    {
        processSelect(intermediateResponse, query.selectTrie.root);
    }

    completionHandler(intermediateResponse);
}

} // namespace query_param
} // namespace redfish
