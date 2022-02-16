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

// Example usage:
// GET /redfish/v1/Systems/system/LogServices
// -> This is a hook, it contains a writable "Members" collection
// POST /redfish/v1/Systems/system/LogServices
// -> Created instance, let's assume BMC assigns the ID of "MY_LOG"
// GET /redfish/v1/Systems/system/LogServices/MY_LOG
// -> As per schema, this contains an additional path component "Entries"
// GET /redfish/v1/Systems/system/LogServices/MY_LOG/Entries
// -> This is a container, ready to go, contains empty "Members" collection
// POST /redfish/v1/Systems/system/LogServices/MY_LOG/Entries
// -> Created entry, let's assume BMC assigns the ID of "MY_NOTE"
// GET /redfish/v1/Systems/system/LogServices/MY_LOG/Entries/MY_NOTE
// -> This retrieves the data previously stored when creating that entry
// GET /redfish/v1/Systems/system/LogServices/MY_LOG/Entries
// -> This container is no longer empty, it now has one element
// GET /redfish/v1/Systems/system/LogServices/MY_LOG
// -> Retrieves some content previously stored when creating the instance
// GET /redfish/v1/Systems/system/LogServices
// -> The "Members" collection now contains "MY_LOG" in addition to before

// All data is expressed in the form of a JSON dictionary. The backing store
// uses a similar directory layout, including the extra "Entries"
// subdirectory. The JSON content for the collection itself is stored as two
// special-case "index.json" filenames within that collection's directories.
// The on-disk file format is whatever is provided by the defaults for the
// nlohmann::json::dump() and nlohmann::json::parse() functions.

// Filesystem layout:
// Directory /run/bmcweb/HOOK/INSTANCE
// Directory /run/bmcweb/HOOK/INSTANCE/MIDDLE
// File /run/bmcweb/HOOK/INSTANCE/index.json
// File /run/bmcweb/HOOK/INSTANCE/MIDDLE/index.json
// File /run/bmcweb/HOOK/INSTANCE/MIDDLE/ENTRY

// HOOK is hardcoded, trimmed from URL, example: "Systems/system/LogServices"
// MIDDLE is hardcoded, a single word, example: "Entries"
// INSTANCE and ENTRY are user-generated (within reason) or BMC-generated
// Each ENTRY file contains JSON of a particular entry under an instance
// The two special "index.json" files contain JSON of that instance itself

// SECURITY WARNING: There is currently no limit on the amount of storage
// taken, nor any automatic cleanup of old content, so clients can cause
// a denial of service attack by consuming all storage. This will be
// addressed by future work.

#include <app.hpp>
#include <registries/privilege_registry.hpp>

#include <fstream>

namespace external_storer
{

// These should become constexpr in a future compiler
inline const std::string defPathPrefix{"/run/bmcweb"};
inline const std::string metadataFile{"index.json"};

// This class only holds configuration and accounting data for the hook.
// As for user-provided data, it is intentionally not here, as it is always
// fetched from the filesystem backing store when needed.
class Hook
{
  private:
    // Omit the leading "/redfish/v1/" and trailing slash
    std::string urlFragmentBase;

    // Schema requires an extra URL path component between instance and entry
    std::string urlFragmentMiddle;

    // Disallow these user instances, avoid already-existing path components
    std::vector<std::string> denyList;

    // Automatically expand these fields when listing the array of entries
    std::vector<std::string> expandList;

    // The common prefix for the Redfish tree, changeable for testing
    std::string pathPrefix;

  public:
    Hook(const std::string& b, const std::string& m,
         const std::vector<std::string>& d, const std::vector<std::string>& e) :
        urlFragmentBase(b),
        urlFragmentMiddle(m), denyList(d), expandList(e),
        pathPrefix(defPathPrefix)
    {}

    void handleCreateInstance(
        const crow::Request& req,
        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp);
    void handleCreateMiddle(const crow::Request& req,
                            const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& instance);
    void handleCreateEntry(const crow::Request& req,
                           const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& instance,
                           const std::string& middle);

    // The 0-argument Get handled by just-in-time insert at integration point
    void handleGetInstance(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& instance);
    void handleGetMiddle(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& instance,
                         const std::string& middle);
    void handleGetEntry(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& instance, const std::string& middle,
                        const std::string& entry);

    // For use by the integration point
    std::vector<std::string> listInstances(void) const;

    // For use during testing
    void deleteAll(void);
    void setPathPrefix(const std::string& path);
};

// This global provides the integration point for LogServices
std::shared_ptr<Hook> hookLogServices;

std::optional<nlohmann::json> readJsonFile(const std::string& filename)
{
    nlohmann::json content;
    std::ifstream input;

    input.open(filename);

    if (!input)
    {
        int err = errno;
        BMCWEB_LOG_ERROR << "Error opening " << filename
                         << " input: " << strerror(err);
        return std::nullopt;
    }

    // Must supply 3rd argument to avoid throwing exceptions
    content = nlohmann::json::parse(input, nullptr, false);

    input.close();

    // Must be good, or if not, must be at EOF, to deem file I/O successful
    if (!(input.good()))
    {
        if (!(input.eof()))
        {
            int err = errno;
            BMCWEB_LOG_ERROR << "Error closing " << filename
                             << " input: " << strerror(err);
            return std::nullopt;
        }
    }

    // Even if file I/O successful, content must be a valid JSON dictionary
    if (content.is_discarded())
    {
        BMCWEB_LOG_ERROR << "Input " << filename << " not valid JSON";
        return std::nullopt;
    }
    if (!(content.is_object()))
    {
        BMCWEB_LOG_ERROR << "Input " << filename << " not JSON dictionary";
        return std::nullopt;
    }

    return {content};
}

// It is assumed tmpfs is fast enough for this I/O never to block
std::optional<std::streampos> writeJsonFile(const std::string& filename,
                                            const nlohmann::json& content)
{
    std::ofstream output;
    std::streampos size;

    output.open(filename, std::ofstream::trunc);

    if (!output)
    {
        int err = errno;
        BMCWEB_LOG_ERROR << "Error opening " << filename
                         << " output: " << strerror(err);
        return std::nullopt;
    }

    // Must supply 4th argument to avoid throwing exceptions
    output << content.dump(-1, ' ', false,
                           nlohmann::json::error_handler_t::replace);

    size = output.tellp();

    output.close();

    if (!(output.good()))
    {
        int err = errno;
        BMCWEB_LOG_ERROR << "Error closing " << filename
                         << " output: " << strerror(err);
        return std::nullopt;
    }

    return {size};
}

// Conservative filename rules to begin with, can relax later if needed
bool validateFilename(const std::string& name)
{
    if (name.empty())
    {
        BMCWEB_LOG_ERROR << "Filename empty";
        return false;
    }

    // Max length 64 bytes
    if (name.size() > 64)
    {
        BMCWEB_LOG_ERROR << "Filename too long";
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

        BMCWEB_LOG_ERROR << "Filename contains invalid characters";
        return false;
    }

    // Must not be the well-known reserved filename
    if (name == metadataFile)
    {
        BMCWEB_LOG_ERROR << "Filename " << name << " is reserved";
        return false;
    }

    return true;
}

bool validateFilename(const std::string& name,
                      const std::vector<std::string>& denyList)
{
    if (!(validateFilename(name)))
    {
        // Error message has already been printed
        return false;
    }

    // Must not be within the denylist
    for (const auto& denyFile : denyList)
    {
        if (name == denyFile)
        {
            BMCWEB_LOG_ERROR << "Filename " << name << " is reserved";
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

void trimMembers(nlohmann::json& content)
{
    // Entries must be added one at a time, using separate POST commands
    auto foundMembers = content.find("Members");
    if (foundMembers != content.end())
    {
        content.erase(foundMembers);
    }

    // No need, this will be dynamically built when output to user
    auto foundCount = content.find("Members@odata.count");
    if (foundCount != content.end())
    {
        content.erase(foundCount);
    }
}

void Hook::handleCreateInstance(
    const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    nlohmann::json content;
    content = nlohmann::json::parse(req.body, nullptr, false);
    if (content.is_discarded())
    {
        BMCWEB_LOG_ERROR << "Uploaded content not JSON";
        redfish::messages::malformedJSON(asyncResp->res);
        return;
    }
    if (!(content.is_object()))
    {
        BMCWEB_LOG_ERROR << "Uploaded JSON type not a dictionary";
        redfish::messages::unrecognizedRequestBody(asyncResp->res);
        return;
    }

    std::string id = extractId(content);
    trimId(content);

    auto innerContent = nlohmann::json::object();

    // Promote the inner layer to its own JSON object
    auto foundMiddle = content.find(urlFragmentMiddle);
    if (foundMiddle != content.end())
    {
        innerContent = foundMiddle.value();
        content.erase(foundMiddle);

        if (!(innerContent.is_object()))
        {
            BMCWEB_LOG_ERROR << "Interior JSON type not a dictionary";
            redfish::messages::unrecognizedRequestBody(asyncResp->res);
            return;
        }

        // Also trim "Id" and "@odata.id" from the inner layer
        trimId(innerContent);

        // Trim "Members" as well, user not allowed bulk upload yet
        trimMembers(innerContent);
    }

    if (!(validateFilename(id, denyList)))
    {
        BMCWEB_LOG_ERROR << "Uploaded instance ID not acceptable";
        redfish::messages::actionParameterValueFormatError(
            asyncResp->res, id, "Id", "CreateInstance");
        return;
    }

    auto suffix = urlFragmentBase + '/' + id;

    auto path = pathPrefix + '/' + suffix;
    auto url = std::string{"/redfish/v1/"} + suffix;

    if (std::filesystem::exists(path))
    {
        BMCWEB_LOG_ERROR << "Uploaded instance ID already exists on system";
        redfish::messages::resourceAlreadyExists(asyncResp->res, "ID", "Id",
                                                 id);
        return;
    }

    auto innerPath = path + '/' + urlFragmentMiddle;

    std::error_code ec;
    std::filesystem::create_directories(innerPath, ec);

    if (ec)
    {
        BMCWEB_LOG_ERROR << "Problem making directories " << innerPath;
        redfish::messages::operationFailed(asyncResp->res);
        return;
    }

    auto filename = path + '/' + metadataFile;
    auto innerFilename = innerPath + '/' + metadataFile;

    if (!(writeJsonFile(filename, content).has_value()))
    {
        BMCWEB_LOG_ERROR << "Problem writing file " << filename;
        redfish::messages::operationFailed(asyncResp->res);
        return;
    }

    if (!(writeJsonFile(innerFilename, innerContent).has_value()))
    {
        BMCWEB_LOG_ERROR << "Problem writing file " << innerFilename;
        redfish::messages::operationFailed(asyncResp->res);
        return;
    }

    redfish::messages::created(asyncResp->res);

    // For ease of use, include Location in both header and body
    asyncResp->res.addHeader(boost::beast::http::field::location, url);
    asyncResp->res.jsonValue["Location"] = url;
}

void Hook::handleCreateMiddle(
    const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& instance)
{
    // Before doing anything with filesystem, validate naming restrictions
    if (!(validateFilename(instance, denyList)))
    {
        BMCWEB_LOG_ERROR << "Instance ID within URL is not acceptable";
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }

    nlohmann::json content;

    content = nlohmann::json::parse(req.body, nullptr, false);
    if (content.is_discarded())
    {
        BMCWEB_LOG_ERROR << "Uploaded content not JSON";
        redfish::messages::malformedJSON(asyncResp->res);
        return;
    }
    if (!(content.is_object()))
    {
        BMCWEB_LOG_ERROR << "Uploaded JSON type not a dictionary";
        redfish::messages::unrecognizedRequestBody(asyncResp->res);
        return;
    }

    std::string id = extractId(content);

    // Unlike instance, no need to do a second layer of trimming underneath
    trimId(content);

    // Unlike instance, names on denyList are perfectly OK at this layer
    if (!(validateFilename(id)))
    {
        BMCWEB_LOG_ERROR << "Uploaded entry ID not acceptable";
        redfish::messages::actionParameterValueFormatError(asyncResp->res, id,
                                                           "Id", "CreateEntry");
        return;
    }

    auto suffix = urlFragmentBase + '/' + instance;

    auto path = pathPrefix + '/' + suffix;
    auto url = std::string{"/redfish/v1/"} + suffix;

    // The instance must already have been created earlier
    if (!(std::filesystem::exists(path)))
    {
        BMCWEB_LOG_ERROR << "Cannot add entry to nonexistent instance "
                         << instance;
        redfish::messages::resourceNotFound(asyncResp->res, "Instance", url);
        return;
    }

    auto append = urlFragmentMiddle + '/' + id;

    auto filename = path + '/' + append;
    auto newUrl = url + '/' + append;

    if (std::filesystem::exists(filename))
    {
        BMCWEB_LOG_ERROR << "Uploaded entry ID already exists within instance";
        redfish::messages::resourceAlreadyExists(asyncResp->res, "ID", "Id",
                                                 id);
        return;
    }

    if (!(writeJsonFile(filename, content).has_value()))
    {
        BMCWEB_LOG_ERROR << "Problem writing file " << filename;
        redfish::messages::operationFailed(asyncResp->res);
        return;
    }

    redfish::messages::created(asyncResp->res);

    // For ease of use, include Location in both header and body
    asyncResp->res.addHeader(boost::beast::http::field::location, newUrl);
    asyncResp->res.jsonValue["Location"] = newUrl;
}

void Hook::handleCreateEntry(
    const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& instance, const std::string& middle)
{
    // Validate the middle path component in URL is the expected constant
    if (middle != urlFragmentMiddle)
    {
        BMCWEB_LOG_ERROR << "URL middle path component is not "
                         << urlFragmentMiddle;
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }

    // This handler has the same function as if the middle were omitted
    handleCreateMiddle(req, asyncResp, instance);
}

std::vector<std::string> listFilenames(const std::string& path)
{
    std::vector<std::string> files;

    // If containing directory not found, there can be no filenames
    if (!(std::filesystem::exists(path)))
    {
        return files;
    }

    auto entries = std::filesystem::directory_iterator{path};
    for (const auto& entry : entries)
    {
        // Only match regular files
        if (!(entry.is_regular_file()))
        {
            continue;
        }

        auto file = entry.path().filename().string();

        // The denylist only affects instance names, not regular filenames
        if (!(validateFilename(file)))
        {
            continue;
        }

        files.push_back(file);
    }

    return files;
}

std::vector<std::string> Hook::listInstances(void) const
{
    std::vector<std::string> files;

    auto path = pathPrefix + '/' + urlFragmentBase;

    // If containing directory not found, there can be no instances
    if (!(std::filesystem::exists(path)))
    {
        return files;
    }

    auto entries = std::filesystem::directory_iterator{path};
    for (const auto& entry : entries)
    {
        // Only match subdirectories
        if (!(entry.is_directory()))
        {
            continue;
        }

        auto file = entry.path().filename().string();

        if (!(validateFilename(file, denyList)))
        {
            continue;
        }

        files.push_back(file);
    }

    return files;
}

void Hook::handleGetInstance(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& instance)
{
    // Before doing anything with filesystem, validate naming restrictions
    if (!(validateFilename(instance, denyList)))
    {
        BMCWEB_LOG_ERROR << "Instance ID within URL is not acceptable";
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }

    auto suffix = urlFragmentBase + '/' + instance;

    auto path = pathPrefix + '/' + suffix;
    auto filename = path + '/' + metadataFile;

    if (!(std::filesystem::exists(filename)))
    {
        BMCWEB_LOG_ERROR << "Instance not found with ID " << instance;
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }

    auto contentOpt = readJsonFile(filename);
    if (!(contentOpt.has_value()))
    {
        BMCWEB_LOG_ERROR << "Problem reading file " << filename;
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }

    auto content = *contentOpt;

    auto url = std::string{"/redfish/v1/"} + suffix;

    // Regenerate these, as they were intentionally trimmed before storage
    content["Id"] = instance;
    content["@odata.id"] = url;

    auto innerUrl = url + '/' + urlFragmentMiddle;

    // Synthesize a correct link to middle layer
    auto middleObject = nlohmann::json::object();
    middleObject["@odata.id"] = innerUrl;
    content[urlFragmentMiddle] = middleObject;

    redfish::messages::success(asyncResp->res);

    asyncResp->res.jsonValue = std::move(content);
}

void Hook::handleGetMiddle(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& instance,
                           const std::string& middle)
{
    // Before doing anything with filesystem, validate naming restrictions
    if (!(validateFilename(instance, denyList)))
    {
        BMCWEB_LOG_ERROR << "Instance ID within URL is not acceptable";
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }

    // Validate the middle path component in URL is the expected constant
    if (middle != urlFragmentMiddle)
    {
        BMCWEB_LOG_ERROR << "URL middle path component is not "
                         << urlFragmentMiddle;
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }

    auto suffix = urlFragmentBase + '/' + instance + '/' + urlFragmentMiddle;

    auto path = pathPrefix + '/' + suffix;
    auto url = std::string{"/redfish/v1/"} + suffix;

    auto filename = path + '/' + metadataFile;

    if (!(std::filesystem::exists(filename)))
    {
        BMCWEB_LOG_ERROR << "Instance not found with ID " << instance;
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }

    auto contentOpt = readJsonFile(filename);
    if (!(contentOpt.has_value()))
    {
        BMCWEB_LOG_ERROR << "Problem reading file " << filename;
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }

    auto content = *contentOpt;

    // Omit, as would supply no new information: content["Id"] =
    // urlFragmentMiddle;
    content["@odata.id"] = url;

    auto pathSlash = path + '/';
    auto urlSlash = url + '/';
    auto files = listFilenames(path);

    // Synthesize special "Members" array with links to all our entries
    auto membersArray = nlohmann::json::array();
    for (const auto& file : files)
    {
        auto fileObject = nlohmann::json::object();
        fileObject["@odata.id"] = urlSlash + file;
        fileObject["Id"] = file;

        // Automatically expand only the fields listed in expandList
        if (!(expandList.empty()))
        {
            auto entryFilename = pathSlash + file;
            auto entryContentOpt = readJsonFile(entryFilename);
            if (entryContentOpt.has_value())
            {
                auto entryContent = *entryContentOpt;

                for (const auto& key : expandList)
                {
                    auto valueIter = entryContent.find(key);
                    if (valueIter != entryContent.end())
                    {
                        fileObject[key] = *valueIter;
                    }
                }
            }
        }

        membersArray += fileObject;
    }

    // Finish putting the pieces together
    content["Members"] = membersArray;
    content["Members@odata.count"] = files.size();

    redfish::messages::success(asyncResp->res);

    asyncResp->res.jsonValue = std::move(content);
}

void Hook::handleGetEntry(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& instance,
                          const std::string& middle, const std::string& entry)
{
    // Before doing anything with filesystem, validate naming restrictions
    if (!(validateFilename(instance, denyList)))
    {
        BMCWEB_LOG_ERROR << "Instance ID within URL is not acceptable";
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }

    // Validate the middle path component in URL is the expected constant
    if (middle != urlFragmentMiddle)
    {
        BMCWEB_LOG_ERROR << "URL middle path component is not "
                         << urlFragmentMiddle;
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }

    // Unlike instance, names on denyList are perfectly OK at this layer
    if (!(validateFilename(entry)))
    {
        BMCWEB_LOG_ERROR << "Entry ID within URL not acceptable";
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }

    auto suffix = urlFragmentBase + '/' + instance + '/' + urlFragmentMiddle +
                  '/' + entry;

    auto filename = pathPrefix + '/' + suffix;
    auto url = std::string{"/redfish/v1/"} + suffix;

    if (!(std::filesystem::exists(filename)))
    {
        BMCWEB_LOG_ERROR << "Entry not found with ID " << entry;
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }

    auto contentOpt = readJsonFile(filename);
    if (!(contentOpt.has_value()))
    {
        BMCWEB_LOG_ERROR << "Problem reading file " << filename;
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }

    auto content = *contentOpt;

    // Regenerate these, as they were intentionally trimmed before storage
    content["Id"] = entry;
    content["@odata.id"] = url;

    redfish::messages::success(asyncResp->res);

    asyncResp->res.jsonValue = std::move(content);
}

void Hook::deleteAll(void)
{
    std::error_code ec;

    auto count = std::filesystem::remove_all(pathPrefix, ec);

    if (ec)
    {
        BMCWEB_LOG_ERROR << "Problem deleting: " << ec.message();
    }

    if (count > 0)
    {
        BMCWEB_LOG_INFO << "Deleted all " << count << " files/dirs from "
                        << pathPrefix;
    }
}

void Hook::setPathPrefix(const std::string& path)
{
    // This function is only for testing, loudly warn if used
    BMCWEB_LOG_WARNING << "Changing path prefix to " << path;

    pathPrefix = path;
}

} // namespace external_storer

namespace redfish
{

// The URL layout under LogServices requires "Entries" path component,
// which seems unnecessary, but is required by the schema.
// POST(HOOK) = create new instance
// POST(HOOK/INSTANCE) = create new entry         | these 2 endpoints
// POST(HOOK/INSTANCE/Entries) = create new entry | do the same thing
// POST(HOOK/INSTANCE/Entries/ENTRY) = not allowed
// GET(HOOK) = supplement existing hook with our added instances
// GET(HOOK/INSTANCE) = return boilerplate of desired instance
// GET(HOOK/INSTANCE/Entries) = return Members array of all entries
// GET(HOOK/INSTANCE/Entries/ENTRY) = return content of desired entry
inline void requestRoutesExternalStorerLogServices(App& app)
{
    constexpr auto urlFragmentBase{"Systems/system/LogServices"};
    constexpr auto midWord{"Entries"};

    // These names come from requestRoutesSystemLogServiceCollection()
    std::vector<std::string> denyList{"EventLog", "Dump", "Crashdump",
                                      "HostLogger"};

    // These names come from the "required" field of LogEntry JSON schema
    std::vector<std::string> expandList{"EntryType", "@odata.id", "@odata.type",
                                        "Id", "Name"};

    // Capturing by copy, in lambdas below, preserves shared_ptr lifetime
    auto hook = std::make_shared<external_storer::Hook>(
        urlFragmentBase, midWord, denyList, expandList);

    external_storer::hookLogServices = hook;

    // Only 0-argument, 1-argument, and 2-argument POST routes exist
    // There intentionally is no 3-argument POST handler
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/LogServices/")
        .privileges(redfish::privileges::postLogService)
        .methods(boost::beast::http::verb::post)(
            [hook](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                hook->handleCreateInstance(req, asyncResp);
            });

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/LogServices/<str>/")
        .privileges(redfish::privileges::postLogService)
        .methods(boost::beast::http::verb::post)(
            [hook](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& instance) {
                hook->handleCreateMiddle(req, asyncResp, instance);
            });

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/LogServices/<str>/<str>/")
        .privileges(redfish::privileges::postLogService)
        .methods(boost::beast::http::verb::post)(
            [hook](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& instance, const std::string& middle) {
                hook->handleCreateEntry(req, asyncResp, instance, middle);
            });

    // Only 1-argument, 2-argument, and 3-argument GET routes are here
    // The 0-argument GET route is already handled by the integration point
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

// NOTE: Currently, this works, but by luck, perhaps due to the fact that
// the ExternalStorer routes are requested last. The router currently does
// not cleanly support overlapping routes. So, the wildcard GET matchers
// currently can not cleanly coexist with the various hardcoded string
// matchers (see denyList) at the same URL position. One possible
// solution is to add a priority system to disambiguate, as discussed here:
// https://gerrit.openbmc-project.xyz/c/openbmc/bmcweb/+/43502
inline void requestRoutesExternalStorer(App& app)
{
    // LogServices is the first, add additional services here
    requestRoutesExternalStorerLogServices(app);
}

} // namespace redfish
