#pragma once

// ExternalStorer allows external users (HTTP clients) to temporarily store
// their data on this Redfish server, hence its name.

// The intended use cases are for catching logging messages and hardware error
// notifications from the host, but ExternalStorer is not limited to these.
// The backing store for this data is a RAM disk (tmpfs), so it will be lost
// when the BMC reboots or powers down. Not intended for long-term storage.

// To comply with relevant Redfish schemas, ExternalStorer will carefully
// merge user-provided data with what is already on the system. Overwriting
// system-provided data is not allowed. There are 3 addressing levels:

// Hook = The integration point, an existing Redfish URL containing one
// writable collection, as allowed by a schema.
// Instance = An external user will POST to create a new collection here,
// which will be added to what is already visible at the Hook.
// Entry = An external user will POST to create a new entry here,
// which will be added to that collection (the Instance).

// Example:
// GET /redfish/v1/Systems/system/LogServices
// This is a hook, it contains a writable "Members" collection
// POST /redfish/v1/Systems/system/LogServices
// Created instance, let's assume BMC assigns the ID of "MY_LOG"
// GET /redfish/v1/Systems/system/LogServices/MY_LOG
// As per schema, this contains a superfluous path component "Entries"
// GET /redfish/v1/Systems/system/LogServices/MY_LOG/Entries
// This is a container, ready to go, contains empty "Members" collection
// POST /redfish/v1/Systems/system/LogServices/MY_LOG/Entries
// Created entry, let's assume BMC assigns the ID of "MY_NOTE"
// GET /redfish/v1/Systems/system/LogServices/MY_LOG/Entries/MY_NOTE
// This retrieves the data previously stored when creating that entry
// GET /redfish/v1/Systems/system/LogServices/MY_LOG/Entries
// This container is no longer empty, it now has one element
// GET /redfish/v1/Systems/system/LogServices/MY_LOG
// This retrieves the data previously stored when creating the instance
// GET /redfish/v1/Systems/system/LogServices
// The "Members" collection now contains "MY_LOG" in addition to before

// All data is expressed in the form of a JSON dictionary. The backing store
// uses a similar directory layout, except the superfluous "Entries" is
// unused, and the JSON content for the collection itself is stored as a
// special-case filename "index.json" within that collection's directory.
// The on-disk file format is whatever nlohmann::json I/O operators provide.

// Filesystem layout:
// Directory /tmp/bmcweb/ExternalStorer/HOOK/INSTANCE
// File /tmp/bmcweb/ExternalStorer/HOOK/INSTANCE/ENTRY
// File /tmp/bmcweb/ExternalStorer/HOOK/INSTANCE/index.json

// HOOK is hardcoded, based on URL, example: "Systems/system/LogServices"
// INSTANCE and ENTRY are user-generated (within reason) or BMC-generated
// Each ENTRY file contains JSON of a particular entry under a instance
// The "index.json" is special and contains JSON of that instance itself

// SECURITY WARNING: There is currently no limit on the amount of storage
// taken, nor any automatic cleanup of old content, so clients can cause
// a denial of service attack by consuming all storage. This will be
// addressed by future work.

namespace external_storer
{

constexpr auto pathPrefix{"/tmp/bmcweb/ExternalStorer"};

constexpr auto metadataFile{"index.json"};

// There is intentionally no structure to hold the user-provided data,
// as that is always fetched from the backing store on disk.
class Hook
{
  protected:
    // Omit the leading "/redfish/v1/" and trailing slash
    std::string urlBase;

    // Schema requires an extra URL path component between instance and entry
    std::string urlMiddle;

    // Disallow these user instances, avoid already-existing path components
    std::vector<std::string> denyList;

  public:
    Hook(const std::string& b, const std::string& m,
         const std::vector<std::string>& d) :
        urlBase(b),
        urlMiddle(m), denyList(d)
    {}

    // There is intentionally no handleCreateMiddle()
    void handleCreateInstance(const crow::Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp);
    void handleCreateEntry(const crow::Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& instance, const std::string& middle);

    // The 0-argument Get handled by just-in-time insert at integration point
    void handleGetInstance(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& instance);
    void handleGetMiddle(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& instance, const std::string& middle);
    void handleGetEntry(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& instance, const std::string& middle, const std::string& entry);

    std::vector<std::string> listInstances(void) const;
};

// Conservative filename rules to begin with, can relax later if needed
bool validateFilename(const std::string& name)
{
    if (name.empty())
    {
        return false;
    }

    // Max length 64 bytes
    if (name.size() > 64)
    {
        return false;
    }

    // Each byte must be in range [A-Za-z0-9_-]
    for (const char& c : name)
    {
        if (c >= 'A' && c <= 'Z')
        {
            continue;
        }

        if (c >= 'a' && c <= 'z')
        {
            continue;
        }

        if (c >= '0' && c <= '9')
        {
            continue;
        }

        if (c == '_' || c == '-')
        {
            continue;
        }

        return false;
    }

    // Must not be the well-known reserved filename
    if (name == metadataFile)
    {
        return false;
    }

    return true;
}

bool validateFilename(const std::string& name, const std::vector<std::string>& denyList)
{
    if (!(validateFilename(name)))
    {
        return false;
    }

    // Must not be within the denylist
    for (const auto& denyFile : denyList)
    {
        if (name == denyFile)
        {
            return false;
        }
    }

    return true;
}

std::string extractId(const nlohmann::json& content)
{
    std::string id;

    auto foundId = content.find("Id");
    if (foundId != content.end())
    {
        id = foundId.value();
    }
    else
    {
        boost::uuids::random_generator gen;

        // Roll a random UUID for server-assigned ID
        id = boost::uuids::to_string(gen());
    }

    return id;
}

void trimId(nlohmann::json& content)
{
    // No need, this is already implied by the filename on disk
    auto foundId = content.find("Id");
    if (foundId != content.end())
    {
        content.erase(foundId);
    }

    // No need, this will be dynamically built when output to user
    auto foundOdataId = content.find("@odata.id");
    if (foundOdataId != content.end())
    {
        content.erase(foundOdataId);
    }
}

void Hook::handleCreateInstance(const crow::Request& req, const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    nlohmann::json content;

    content = nlohmann::json::parse(req.body, nullptr, false);
    if (content.is_discarded())
    {
        BMCWEB_LOG_INFO << "Uploaded content not JSON";
        redfish::messages::malformedJSON(asyncResp->res);
        return;
    }
    if (!(content.is_object()))
    {
        BMCWEB_LOG_INFO << "Uploaded JSON type not a dictionary";
        redfish::messages::unrecognizedRequestBody(asyncResp->res);
        return;
    }

    std::string id = extractId(content);

    trimId(content);

    // Also trim "Id" and "@odata.id" from the inner layer
    auto foundMiddle = content.find(urlMiddle);
    if (foundMiddle != content.end())
    {
        // Get reference, to be able to modify the original content
        auto& middle = foundMiddle.value();
        if (middle.is_object())
        {
            trimId(middle);
        }
    }

    if (!(validateFilename(id, denyList)))
    {
        BMCWEB_LOG_INFO << "Uploaded ID not acceptable";
        redfish::messages::invalidObject(asyncResp->res, "Id");
        return;
    }

    auto suffix = urlBase + "/" + id;
    auto path = std::string{pathPrefix} + "/" + suffix;
    auto url = std::string{"/redfish/v1/"} + suffix;

    if (std::filesystem::exists(path))
    {
        BMCWEB_LOG_INFO << "Uploaded ID already exists on system";
        redfish::messages::resourceAlreadyExists(asyncResp->res, "ID", "Id", id);
        return;
    }

    std::error_code ec;
    std::filesystem::create_directories(path, ec);

    if (ec)
    {
        BMCWEB_LOG_INFO << "Problem making directory " << path;
        redfish::messages::operationFailed(asyncResp->res);
        return;
    }

    auto filename = path + "/" + metadataFile;

    std::ofstream output;

    // It is assumed tmpfs is fast enough for this I/O never to block
    output.open(filename);
    output << content;
    output.close();

    if (!(output.good()))
    {
        BMCWEB_LOG_INFO << "Problem writing file " << filename;
        redfish::messages::operationFailed(asyncResp->res);
        return;
    }

    redfish::messages::created(asyncResp->res);

    // For ease of use, include Location in both header and body
    asyncResp->res.addHeader(boost::beast::http::field::location, url);
    asyncResp->res.jsonValue["Location"] = url;
}

void Hook::handleCreateEntry(const crow::Request& req, const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, const std::string& instance, const std::string& middle)
{
    // Before doing anything with filesystem, validate naming restrictions
    if (!(validateFilename(instance, denyList)))
    {
        BMCWEB_LOG_INFO << "ID within URL is not acceptable";
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }

    // Validate the middle path component in URL is the expected constant
    if (middle != urlMiddle)
    {
        BMCWEB_LOG_INFO << "URL middle path component is not " << urlMiddle;
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }

    nlohmann::json content;

    content = nlohmann::json::parse(req.body, nullptr, false);
    if (content.is_discarded())
    {
        BMCWEB_LOG_INFO << "Uploaded content not JSON";
        redfish::messages::malformedJSON(asyncResp->res);
        return;
    }
    if (!(content.is_object()))
    {
        BMCWEB_LOG_INFO << "Uploaded JSON type not a dictionary";
        redfish::messages::unrecognizedRequestBody(asyncResp->res);
        return;
    }

    std::string id = extractId(content);

    // Unlike instance, no need to do a second layer of trimming underneath
    trimId(content);

    // Unlike instance, names on denyList are perfectly OK at this layer
    if (!(validateFilename(id)))
    {
        BMCWEB_LOG_INFO << "Uploaded ID not acceptable";
        redfish::messages::invalidObject(asyncResp->res, "Id");
        return;
    }

    auto suffix = urlBase + "/" + instance;
    auto path = std::string{pathPrefix} + "/" + suffix;
    auto url = std::string{"/redfish/v1/"} + suffix;

    // The instance must already have been created earlier
    if (!(std::filesystem::exists(path)))
    {
        BMCWEB_LOG_INFO << "Cannot add entry to nonexistent instance " << instance;
        redfish::messages::resourceMissingAtURI(asyncResp->res, url);
        return;
    }

    auto filename = path + "/" + id;
    url += (std::string{"/"} + urlMiddle + "/" + id);

    if (std::filesystem::exists(filename))
    {
        BMCWEB_LOG_INFO << "Uploaded ID already exists on system";
        redfish::messages::resourceAlreadyExists(asyncResp->res, "ID", "Id", id);
        return;
    }

    std::ofstream output;

    // It is assumed tmpfs is fast enough for this I/O never to block
    output.open(filename);
    output << content;
    output.close();

    if (!(output.good()))
    {
        BMCWEB_LOG_INFO << "Problem writing file " << filename;
        redfish::messages::operationFailed(asyncResp->res);
        return;
    }

    redfish::messages::created(asyncResp->res);

    // For ease of use, include Location in both header and body
    asyncResp->res.addHeader(boost::beast::http::field::location, url);
    asyncResp->res.jsonValue["Location"] = url;
}

void Hook::handleGetInstance(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, const std::string& instance)
{
    // Before doing anything with filesystem, validate naming restrictions
    if (!(validateFilename(instance, denyList)))
    {
        BMCWEB_LOG_INFO << "ID within URL is not acceptable";
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }

    auto suffix = urlBase + "/" + instance;
    auto path = std::string{pathPrefix} + "/" + suffix;
    auto filename = path + "/" + metadataFile;

    if (!(std::filesystem::exists(filename)))
    {
        BMCWEB_LOG_INFO << "Instance not found with ID " << instance;
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }

    nlohmann::json content;

    std::ifstream input;

    // It is assumed tmpfs is fast enough for this I/O never to block
    input.open(filename);
    input >> content;
    input.close();

    if (!(input.good()))
    {
        BMCWEB_LOG_INFO << "Problem reading file " << filename;
        redfish::messages::operationFailed(asyncResp->res);
        return;
    }

    if (content.is_discarded())
    {
        BMCWEB_LOG_INFO << "JSON not parsed, corrupted file " << filename;
        redfish::messages::operationFailed(asyncResp->res);
        return;
    }
    if (!(content.is_object()))
    {
        BMCWEB_LOG_INFO << "JSON not dictionary, corrupted file " << filename;
        redfish::messages::operationFailed(asyncResp->res);
        return;
    }

    auto foundMiddle = content.find(urlMiddle);
    if (foundMiddle != content.end())
    {
        content.erase(foundMiddle);
    }

    auto url = std::string{"/redfish/v1/"} + suffix;

    // Regenerate these, as they were intentionally trimmed before storage
    content["Id"] = instance;
    content["@odata.id"] = url;

    url += (std::string{"/"} + urlMiddle);

    // Synthesize key with link to middle layer
    auto middle = nlohmann::json::object();
    middle["@odata.id"] = url;
    content[urlMiddle] = middle;

    redfish::messages::success(asyncResp->res);

    asyncResp->res.jsonValue = content;
}

void Hook::handleGetMiddle(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, const std::string& instance, const std::string& middle)
{
    (void)asyncResp;
    (void)instance;
    (void)middle;
    // ### build JSON result from Entries item of metadata file for instance
    // ### insert special @odata.id item
    // ### insert special Id item (it's just the Middle)
    // ### synthesize Members (from directory listing)
    // ### each element of JSON array is JSON dictionary with 1 item, "@odata.id" and URL
    // ### synthesize Members@odata.count
}

void Hook::handleGetEntry(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, const std::string& instance, const std::string& middle, const std::string& entry)
{
    (void)asyncResp;
    (void)instance;
    (void)middle;
    (void)entry;
    // ### build JSON result from metadata file for entry
    // ### insert special @odata.id item
    // ### insert special Id item
}

std::vector<std::string> Hook::listInstances(void) const
{
    std::vector<std::string> files;

    auto path = std::string{pathPrefix} + "/" + urlBase;

    // If containing directory not found, there can be no instances
    if (!(std::filesystem::exists(path)))
    {
        return files;
    }

    auto dirs = std::filesystem::directory_iterator{path};
    for (const auto& dir : dirs)
    {
        // Only match subdirectories
        if (!(dir.is_directory()))
        {
            continue;
        }

        auto file = dir.path().filename().string();

        if (!(validateFilename(file, denyList)))
        {
            continue;
        }

        files.push_back(file);
    }

    return files;
}

// TODO(): Is there a better way than by using a global?
std::shared_ptr<Hook> globalHookLogServices;

// ### hookPost needs to check the denylist
// ### middleGet and entryGet need to fail out if urlmiddle not match

// TODO(): The wildcard GET match currently can not coexist
//  with the hardcoded string matches, one solution is to
//  add a priority system, as discussed here:
//  https://gerrit.openbmc-project.xyz/c/openbmc/bmcweb/+/43502


} // namespace external_storer

namespace redfish
{

// The URL layout under LogServices requires "Entries" path component,
// which seems unnecessary, but is required by the schema.
// POST(HOOK) = create new instance
// POST(HOOK/INSTANCE) = not allowed
// POST(HOOK/INSTANCE/Entries) = create new entry
// GET(HOOK) = supplement existing hook with our added instances
// GET(HOOK/INSTANCE) = return boilerplate of desired instance
// GET(HOOK/INSTANCE/Entries) = return Members array of all entries
// GET(HOOK/INSTANCE/Entries/ENTRY) = return content of desired entry
inline void requestRoutesExternalStorerLogServices(App& app)
{
    constexpr auto base{"Systems/system/LogServices"};

    // This denylist comes from requestRoutesSystemLogServiceCollection()
    std::vector<std::string> deny{"EventLog", "Dump", "Crashdump", "HostLogger"};

    // Capturing by copy, in lambdas below, preserves shared_ptr lifetime
    auto hook = std::make_shared<external_storer::Hook>(base, "Entries", deny);

    external_storer::globalHookLogServices = hook;

    // TODO(): Trailing slash, or not?
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/LogServices/")
        .privileges(redfish::privileges::postLogService)
        .methods(boost::beast::http::verb::post)(
            [hook](const crow::Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                hook->handleCreateInstance(req, asyncResp);
            });

    // There are intentionally no 1-argument or 3-argument POST handlers
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/LogServices/<str>/<str>/")
        .privileges(redfish::privileges::postLogService)
        .methods(boost::beast::http::verb::post)(
            [hook](const crow::Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& instance,
                       const std::string& middle) {
                hook->handleCreateEntry(req, asyncResp, instance, middle);
            });

    // The 0-argument GET is handled at the integration point, not here
    // It is at log_services.hpp requestRoutesSystemLogServiceCollection()
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/LogServices/<str>/")
        .privileges(redfish::privileges::getLogService)
        .methods(boost::beast::http::verb::get)(
            [hook](const crow::Request&,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& instance) {
                hook->handleGetInstance(asyncResp, instance);
            });

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/LogServices/<str>/<str>/")
        .privileges(redfish::privileges::getLogService)
        .methods(boost::beast::http::verb::get)(
            [hook](const crow::Request&,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& instance, const std::string& middle) {
                hook->handleGetMiddle(asyncResp, instance, middle);
            });

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/system/LogServices/<str>/<str>/<str>/")
        .privileges(redfish::privileges::getLogService)
        .methods(boost::beast::http::verb::get)(
            [hook](const crow::Request&,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& instance, const std::string& middle,
                       const std::string& entry) {
                hook->handleGetEntry(asyncResp, instance, middle, entry);
            });
}

inline void requestRoutesExternalStorer(App& app)
{
    // LogServices is the first, add additional services here
    requestRoutesExternalStorerLogServices(app);
}

} // namespace redfish
