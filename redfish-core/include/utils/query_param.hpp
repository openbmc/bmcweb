#pragma once
#include "app.hpp"
#include "async_resp.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "routing.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/container/flat_set.hpp>

#include <array>
#include <charconv>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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

// The struct stores the parsed query parameters of the default Redfish route.
struct Query
{
    // Only
    bool isOnly = false;
    // Expand
    uint8_t expandLevel = 0;
    ExpandType expandType = ExpandType::None;
    // Skip
    size_t skip = 0;
    // Top
    size_t top = std::numeric_limits<size_t>::max();
    // Select
    std::vector<std::string> selectedProperties = {};
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
    if (queryCapabilities.canDelegateTop)
    {
        delegated.top = query.top;
        query.top = std::numeric_limits<size_t>::max();
    }
    // delegate skip
    if (queryCapabilities.canDelegateSkip)
    {
        delegated.skip = query.skip;
        query.skip = 0;
    }
    // delegate select
    if (!query.selectedProperties.empty() &&
        queryCapabilities.canDelegateSelect)
    {
        std::swap(delegated.selectedProperties, query.selectedProperties);
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
    return getNumericParam(value, query.skip);
}

static constexpr size_t maxEntriesPerPage = 1000;
inline QueryError getTopParam(std::string_view value, Query& query)
{
    QueryError ret = getNumericParam(value, query.top);
    if (ret != QueryError::Ok)
    {
        return ret;
    }

    // Range check for sanity.
    if (query.top > maxEntriesPerPage)
    {
        return QueryError::OutOfRange;
    }

    return QueryError::Ok;
}

inline std::optional<Query>
    parseParameters(const boost::urls::params_view& urlParams,
                    crow::Response& res)
{
    Query ret;
    for (const boost::urls::params_view::value_type& it : urlParams)
    {
        std::string_view key(it.key.data(), it.key.size());
        std::string_view value(it.value.data(), it.value.size());
        if (key == "only")
        {
            if (!it.value.empty())
            {
                messages::queryParameterValueFormatError(res, value, key);
                return std::nullopt;
            }
            ret.isOnly = true;
        }
        else if (key == "$expand" && bmcwebInsecureEnableQueryParams)
        {
            if (!getExpandType(value, ret))
            {
                messages::queryParameterValueFormatError(res, value, key);
                return std::nullopt;
            }
        }
        else if (key == "$top")
        {
            QueryError topRet = getTopParam(value, ret);
            if (topRet == QueryError::ValueFormat)
            {
                messages::queryParameterValueFormatError(res, value, key);
                return std::nullopt;
            }
            if (topRet == QueryError::OutOfRange)
            {
                messages::queryParameterOutOfRange(
                    res, value, "$top",
                    "1-" + std::to_string(maxEntriesPerPage));
                return std::nullopt;
            }
        }
        else if (key == "$skip")
        {
            QueryError topRet = getSkipParam(value, ret);
            if (topRet == QueryError::ValueFormat)
            {
                messages::queryParameterValueFormatError(res, value, key);
                return std::nullopt;
            }
            if (topRet == QueryError::OutOfRange)
            {
                messages::queryParameterOutOfRange(
                    res, value, key,
                    "1-" + std::to_string(std::numeric_limits<size_t>::max()));
                return std::nullopt;
            }
        }
        else if (key == "$select")
        {
            // TODO(nanzhou): check if these properties are valid?
            boost::split(ret.selectedProperties, value, boost::is_any_of(","));
        }
        else
        {
            // Intentionally ignore other errors Redfish spec, 7.3.1
            if (key.starts_with("$"))
            {
                // Services shall return... The HTTP 501 Not Implemented
                // status code for any unsupported query parameters that
                // start with $ .
                messages::queryParameterValueFormatError(res, value, key);
                res.result(boost::beast::http::status::not_implemented);
                return std::nullopt;
            }
            // "Shall ignore unknown or unsupported query parameters that do
            // not begin with $ ."
        }
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
// might need resolved.  It recursively walks the jsonResponse object,
// looking for links at every level, and returns a list (out) of locations
// within the tree that need to be expanded.  The current json pointer
// location p is passed in to reference the current node that's being
// expanded, so it can be combined with the keys from the jsonResponse
// object
inline void findNavigationReferencesRecursive(
    ExpandType eType, nlohmann::json& jsonResponse,
    const nlohmann::json::json_pointer& p, bool inLinks,
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
            findNavigationReferencesRecursive(eType, element, newPtr, inLinks,
                                              out);
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
                BMCWEB_LOG_DEBUG << "Found element at " << p.to_string();
                out.push_back({p, *uri});
            }
        }
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
                                          localInLinks, out);
    }
}

inline std::vector<ExpandNode>
    findNavigationReferences(ExpandType eType, nlohmann::json& jsonResponse)
{
    std::vector<ExpandNode> ret;
    // Start recursion at the root
    findNavigationReferencesRecursive(
        eType, jsonResponse, nlohmann::json::json_pointer(""), false, ret);
    return ret;
}

// Formats a query parameter string for the sub-query.
// This function handles $select when doing sub-query.
// There is no need to handle parameters that's not campatible with $expand,
// e.g., $only, since this function will only be called in side $expand
// handlers
inline std::string formatQueryForExpand(const Query& query)
{
    std::string str;
    if (query.expandLevel >= 2 && query.expandType != ExpandType::None)
    {
        // there needs further $expand
        str += "?$expand=";
        switch (query.expandType)
        {
            case ExpandType::Links:
                str += '~';
                break;
            case ExpandType::NotLinks:
                str += '.';
                break;
            case ExpandType::Both:
                str += '*';
                break;
            default:
                // This shall never happen
                str = "";
        }
        str += "($levels=";
        str += std::to_string(query.expandLevel - 1);
        str += ')';
    }
    if (query.selectedProperties.empty())
    {
        return str;
    }
    str += str.empty() ? '?' : '&';
    str += "$select=";
    str += boost::join(query.selectedProperties, ",");
    return str;
}

inline void
    recursiveSelect(const nlohmann::json& currRoot,
                    const nlohmann::json::json_pointer& currRootPtr,
                    const std::string& prefix, bool parentSelected,
                    const boost::container::flat_set<std::string>& shouldSelect,
                    std::vector<nlohmann::json::json_pointer>& selected)
{
    // If the current node is not a value type, we do recursion
    const nlohmann::json::object_t* object =
        currRoot.get_ptr<const nlohmann::json::object_t*>();
    if (object != nullptr)
    {
        BMCWEB_LOG_DEBUG << "Current JSON is an object "
                         << currRootPtr.to_string();
        for (const auto& [k, v] : currRoot.items())
        {
            std::string newPrefix;
            if (!prefix.empty())
            {
                newPrefix += prefix;
                newPrefix += '/';
            }
            newPrefix += k;
            // do recursion; if the entire current tree is selected, set
            // |parentSelected| to true for recursions
            recursiveSelect(v, currRootPtr / k, newPrefix,
                            parentSelected || shouldSelect.contains(newPrefix),
                            shouldSelect, selected);
        }
        return;
    }
    const nlohmann::json::array_t* array =
        currRoot.get_ptr<const nlohmann::json::array_t*>();
    if (array != nullptr)
    {
        BMCWEB_LOG_DEBUG << "Current JSON is an array "
                         << currRootPtr.to_string();
        for (size_t i = 0; i < array->size(); ++i)
        {
            nlohmann::json::json_pointer newPtr = currRootPtr / i;
            recursiveSelect((*array)[i], currRootPtr / i, prefix,
                            parentSelected, shouldSelect, selected);
        }
        return;
    }
    BMCWEB_LOG_DEBUG << "Current JSON is a property value: " << currRootPtr;
    // otherwise, determine if we select this property
    if (parentSelected || shouldSelect.contains(prefix))
    {
        selected.push_back(currRootPtr);
        return;
    }
    // per the Redfish spec, the service shall select certain properties as
    // if $select was omitted.
    constexpr std::array<std::string_view, 4> odataProperties = {
        "@odata.id", "@odata.type", "@odata.context", "@odata.etag"};
    if (std::any_of(odataProperties.begin(), odataProperties.end(),
                    [&prefix](std::string_view str) { return str == prefix; }))
    {
        selected.push_back(currRootPtr);
    }
}

inline nlohmann::json performSelect(const nlohmann::json& root,
                                    std::span<const std::string> shouldSelect)
{
    std::vector<nlohmann::json::json_pointer> selected;
    // start recursion at the root
    recursiveSelect(root, nlohmann::json::json_pointer(""), "", false,
                    {shouldSelect.begin(), shouldSelect.end()}, selected);
    BMCWEB_LOG_DEBUG << "Selected size: " << selected.size();
    if (selected.empty())
    {
        return nlohmann::json{};
    }
    nlohmann::json res;
    for (auto const& ptr : selected)
    {
        res[ptr.to_string()] = root[ptr];
    }
    BMCWEB_LOG_DEBUG << "Flattened selected tree: " << res.dump(2);
    return res.unflatten();
};

class MultiAsyncResp : public std::enable_shared_from_this<MultiAsyncResp>
{
  public:
    // This object takes a single asyncResp object as the "final" one, then
    // allows callers to attach sub-responses within the json tree that need
    // to be executed and filled into their appropriate locations.  This
    // class manages the final "merge" of the json resources.
    MultiAsyncResp(crow::App& app,
                   std::shared_ptr<bmcweb::AsyncResp> finalResIn) :
        app(app),
        finalRes(std::move(finalResIn))
    {}

    void addAwaitingResponse(
        std::shared_ptr<bmcweb::AsyncResp>& res,
        const nlohmann::json::json_pointer& finalExpandLocation)
    {
        res->res.setCompleteRequestHandler(std::bind_front(
            placeResultStatic, shared_from_this(), finalExpandLocation));
    }

    // Places sub-queries to the corresponding location
    void placeResult(const nlohmann::json::json_pointer& locationToPlace,
                     crow::Response& res)
    {
        nlohmann::json& finalObj = finalRes->res.jsonValue[locationToPlace];
        finalObj = std::move(res.jsonValue);
    }

    // Handles the very first level of Expand, and starts a chain of
    // sub-queries for deeper levels.
    void startQuery(const Query& query)
    {
        // perform $select if any first
        if (!query.selectedProperties.empty())
        {
            finalRes->res.jsonValue = performSelect(finalRes->res.jsonValue,
                                                    query.selectedProperties);
        }
        // then find all nodes that need to expand
        std::vector<ExpandNode> nodes =
            findNavigationReferences(query.expandType, finalRes->res.jsonValue);
        BMCWEB_LOG_DEBUG << nodes.size() << " nodes to traverse";
        std::string queryStr = formatQueryForExpand(query);
        for (const ExpandNode& node : nodes)
        {
            BMCWEB_LOG_DEBUG << "URL of subquery:  " << (node.uri + queryStr);
            std::error_code ec;
            crow::Request newReq(
                {boost::beast::http::verb::get, node.uri + queryStr, 11}, ec);
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

inline void
    processSelect(crow::Response& intermediateResponse,
                  std::span<const std::string> shouldSelect,
                  std::function<void(crow::Response&)>& completionHandler)
{
    intermediateResponse.jsonValue =
        performSelect(intermediateResponse.jsonValue, shouldSelect);
    completionHandler(intermediateResponse);
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
    if (query.expandType != ExpandType::None)
    {
        BMCWEB_LOG_DEBUG
            << "Executing expand query (with other query parameters combinations)";
        // TODO(ed) this is a copy of the response object.  Admittedly,
        // we're inherently doing something inefficient, but we shouldn't
        // have to do a full copy
        auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
        asyncResp->res.setCompleteRequestHandler(std::move(completionHandler));
        asyncResp->res.jsonValue = std::move(intermediateResponse.jsonValue);
        auto multi = std::make_shared<MultiAsyncResp>(app, asyncResp);

        // Start the Expand query; note that |MultiAsyncResp| handles
        // parameters combined with $expand, e.g., $select
        multi->startQuery(query);
        return;
    }
    if (!query.selectedProperties.empty())
    {
        BMCWEB_LOG_DEBUG << "Executing select query";
        processSelect(intermediateResponse, query.selectedProperties,
                      completionHandler);
        return;
    }
    completionHandler(intermediateResponse);
}

} // namespace query_param
} // namespace redfish
