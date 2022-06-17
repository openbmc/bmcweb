#pragma once
#include "app.hpp"
#include "async_resp.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "routing.hpp"

#include <charconv>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <fstream>

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

inline bool processOnly(crow::App& app, const nlohmann::json::array_t& members,
                        crow::Response& res,
                        std::function<void(crow::Response&)>& completionHandler)
{
    BMCWEB_LOG_DEBUG << "Processing only query param";
    if (members.size() != 1)
    {
        BMCWEB_LOG_DEBUG << "Members contains " << members.size()
                         << " element, returning full collection.";
        completionHandler(res);
        return true;
    }

    const nlohmann::json& firstMember = *members.begin();

    auto itUrl = firstMember.find("@odata.id");
    if (itUrl == firstMember.begin()->end())
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

inline void processFileResources(crow::Response& intermediateResponse,
                                  std::string_view target)
{
    BMCWEB_LOG_DEBUG << "FSR Target " << target;
    BMCWEB_LOG_DEBUG << "FSR intermediateResponse.resultInt() "
        << intermediateResponse.resultInt();

    if (intermediateResponse.resultInt() == 404)
    {
        BMCWEB_LOG_DEBUG << "FSR 404";
    }

    // Assemble path
    std::filesystem::path path("/run/bmcweb");
    BMCWEB_LOG_DEBUG << "FSR Path " << path.c_str();
    path += target;
    BMCWEB_LOG_DEBUG << "FSR Path " << path.c_str();
    path.append("index.json");
    BMCWEB_LOG_DEBUG << "FSR Path " << path.c_str();
    BMCWEB_LOG_DEBUG << "FSR Filename? " << path.has_filename();
    
    std::ifstream input;
    input.open(path);

    if (!input)
    {
        int err = errno;
        BMCWEB_LOG_ERROR << "FSR Error opening " << path
                         << " input: " << strerror(err);
        return;
    }

    // Must supply 3rd argument to avoid throwing exceptions
    nlohmann::json j = nlohmann::json::parse(input, nullptr, false);

    input.close();

    // Must be good, or if not, must be at EOF, to deem file I/O successful
    if (!(input.good()))
    {
        if (!(input.eof()))
        {
            int err = errno;
            BMCWEB_LOG_ERROR << "FSR Error closing " << path
                             << " input: " << strerror(err);
            return;
        }
    }

    // Even if file I/O successful, content must be a valid JSON dictionary
    if (j.is_discarded())
    {
        BMCWEB_LOG_ERROR << "FSR Input " << path << " not valid JSON";
        return;
    }
    if (!(j.is_object()))
    {
        BMCWEB_LOG_ERROR << "FSR Input " << path << " not JSON dictionary";
        return;
    }

    intermediateResponse.jsonValue.update(j);
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
    findNavigationReferences(ExpandType eType, nlohmann::json& jsonResponse,
                             const nlohmann::json::json_pointer& root)
{
    std::vector<ExpandNode> ret;
    findNavigationReferencesRecursive(eType, jsonResponse, root, false, ret);
    return ret;
}

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
        Query query, std::shared_ptr<bmcweb::AsyncResp>& res,
        const nlohmann::json::json_pointer& finalExpandLocation)
    {
        res->res.setCompleteRequestHandler(std::bind_front(
            onEndStatic, shared_from_this(), query, finalExpandLocation));
    }

    void onEnd(Query query, const nlohmann::json::json_pointer& locationToPlace,
               crow::Response& res)
    {
        nlohmann::json& finalObj = finalRes->res.jsonValue[locationToPlace];
        finalObj = std::move(res.jsonValue);

        if (query.expandLevel <= 0)
        {
            // Last level to expand, no need to go deeper
            return;
        }
        // Now decrease the depth by one to account for the tree node we
        // just resolved
        query.expandLevel--;

        std::vector<ExpandNode> nodes = findNavigationReferences(
            query.expandType, finalObj, locationToPlace);
        BMCWEB_LOG_DEBUG << nodes.size() << " nodes to traverse";
        for (const ExpandNode& node : nodes)
        {
            BMCWEB_LOG_DEBUG << "Expanding " << locationToPlace;
            std::error_code ec;
            crow::Request newReq({boost::beast::http::verb::get, node.uri, 11},
                                 ec);
            if (ec)
            {
                messages::internalError(res);
                return;
            }

            auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
            BMCWEB_LOG_DEBUG << "setting completion handler on "
                             << &asyncResp->res;
            addAwaitingResponse(query, asyncResp, node.location);
            app.handle(newReq, asyncResp);
        }
    }

  private:
    static void onEndStatic(const std::shared_ptr<MultiAsyncResp>& multi,
                            Query query,
                            const nlohmann::json::json_pointer& locationToPlace,
                            crow::Response& res)
    {
        multi->onEnd(query, locationToPlace, res);
    }

    crow::App& app;
    std::shared_ptr<bmcweb::AsyncResp> finalRes;
};

inline bool processCollectionParams(
    const Query& query, crow::App& app, crow::Response& res,
    std::function<void(crow::Response&)>& completionHandler)
{
    nlohmann::json::object_t* obj =
        res.jsonValue.get_ptr<nlohmann::json::object_t*>();
    if (obj == nullptr)
    {
        // Shouldn't be possible.  All responses should be objects.
        messages::internalError(res);
        return false;
    }

    BMCWEB_LOG_DEBUG << "Handling collection queries";
    nlohmann::json::object_t::iterator members = obj->find("Members");
    if (members == obj->end())
    {
        // From the Redfish specification 7.3.1
        // ... the HTTP 400 Bad Request status code with the
        // QueryNotSupportedOnResource message from the Base Message Registry
        // for any supported query parameters that apply only to resource
        // collections but are used on singular resources.
        messages::queryNotSupportedOnResource(res);
        return false;
    }

    nlohmann::json::array_t* arr =
        members->second.get_ptr<nlohmann::json::array_t*>();
    if (arr == nullptr)
    {
        messages::internalError(res);
        return false;
    }

    // Process $only
    if (query.isOnly)
    {
        processOnly(app, *arr, res, completionHandler);
        // Only is the only thing that takes ownerhsip of the response
        return true;
    }

    // Process $skip
    // Per section 7.3.1 of the Redfish specification, $skip is run before
    // $top
    // Can only skip as many values as we have
    size_t skip = std::min(arr->size(), query.skip);
    arr->erase(arr->begin(), arr->begin() + static_cast<ssize_t>(skip));

    // Process $top
    // Can only top as many values as we have
    size_t top = std::min(arr->size(), query.top);
    arr->erase(arr->begin() + static_cast<ssize_t>(top), arr->end());

    return false;
}

inline void
    processAllParams(crow::App& app, const Query query,
                     std::function<void(crow::Response&)>& completionHandler,
                     crow::Response& intermediateResponse,
                     std::string_view target)
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

    // Handles only, top, and skip
    if (query.isOnly || query.top != std::numeric_limits<size_t>::max() ||
        query.skip != 0)
    {
        if (processCollectionParams(query, app, intermediateResponse,
                                    completionHandler))
        {
            return;
        }
    }

    if (query.expandType != ExpandType::None)
    {
        BMCWEB_LOG_DEBUG << "Executing expand query";
        // TODO(ed) this is a copy of the response object.  Admittedly,
        // we're inherently doing something inefficient, but we shouldn't
        // have to do a full copy
        auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
        asyncResp->res.setCompleteRequestHandler(std::move(completionHandler));
        asyncResp->res.jsonValue = std::move(intermediateResponse.jsonValue);
        auto multi = std::make_shared<MultiAsyncResp>(app, asyncResp);

        // Start the chain by "ending" the root response
        multi->onEnd(query, nlohmann::json::json_pointer(""), asyncResp->res);
        return;
    }

    // At this point the results of the query are ready to send, the handler
    // has had a chance to populate the response and/or any errors have been
    // processed. Now we can see if any JSON has been stored for this URI via 
    // the External Storer feature (IE stored on disk my a prior POST).
    processFileResources(intermediateResponse, target);
    
    completionHandler(intermediateResponse);
}

} // namespace query_param
} // namespace redfish
