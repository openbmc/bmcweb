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
bool validateId(const std::string& name)
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

    return true;
}

void Hook::handleCreateInstance(const crow::Request& req, const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    // ### must be JSON dictionary
    // ### silently drop @odata.id item
    // ### Id is optional, roll up random UUID if not given
    // ### silently drop Id item (it will be implied by filename)
    // ### disallow bad filename
    // ### disallow denylist filenames or special filename
    // ### check disk, disallow duplicate filename
    // ### make leading directories if necessary
    // ### write JSON to disk
    // ### stuff resulting URL in Location field of header
    (void)req;
    (void)asyncResp;
}

void Hook::handleCreateEntry(const crow::Request& req, const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, const std::string& instance, const std::string& middle)
{
    (void)req;
    (void)asyncResp;
    (void)instance;
    (void)middle;
    // ### must be JSON dictionary
    // ### silently drop @odata.id member
    // ### Id is optional, roll up random UUID if not given
    // ### silently drop Id item (it will be implied by filename)
    // ### disallow bad filename
    // ### disallow special filename (denylist is OK, it's only for instance)
    // ### check disk, instance directory must already exist
    // ### check disk, disallow duplicate filename within instance directory
    // ### write JSON to disk
    // ### stuff resulting URL in Location field of header
}

void Hook::handleGetInstance(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, const std::string& instance)
{
    (void)asyncResp;
    (void)instance;
    // ### build JSON result from metadata file
    // ### drop Entries item from JSON
    // ### insert special @odata.id item
    // ### insert special Id item (it's the instance name)
    // ### synthesize Entries (only 1 entry, the Middle)
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
        // Only match directories
        if (!(dir.is_directory()))
        {
            continue;
        }

        auto file = dir.path().filename().string();

        // Exclude reserved filename from list
        if (file == metadataFile)
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


}; // namespace external_storer

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

}; // namespace redfish
