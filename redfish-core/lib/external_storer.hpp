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
// -> Created entry, let's assume BMC assigns the ID of "MY_ALERT"
// GET /redfish/v1/Systems/system/LogServices/MY_LOG/Entries/MY_ALERT
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
// Directory /run/bmcweb/redfish/v1/HOOK/INSTANCE
// Directory /run/bmcweb/redfish/v1/HOOK/INSTANCE/MIDDLE
// Directory /run/bmcweb/redfish/v1/HOOK/INSTANCE/MIDDLE/ENTRY
// File /run/bmcweb/redfish/v1/HOOK/INSTANCE/index.json
// File /run/bmcweb/redfish/v1/HOOK/INSTANCE/MIDDLE/index.json
// File /run/bmcweb/redfish/v1/HOOK/INSTANCE/MIDDLE/ENTRY/index.json

// HOOK is hardcoded, trimmed from URL, example: "Systems/system/LogServices"
// MIDDLE is hardcoded, a single word, example: "Entries"
// INSTANCE and ENTRY are user-generated (within reason) or BMC-generated
// Each ENTRY index.json file contains JSON of one entry within an instance
// The two higher "index.json" files contain JSON of that instance itself

// SECURITY WARNING: There is currently no limit on the amount of storage
// taken, nor any automatic cleanup of old content, so clients can cause
// a denial of service attack by consuming all storage. This will be
// addressed by future work.

#include "app.hpp"
#include "openbmc_dbus_rest.hpp"
#include "registries/privilege_registry.hpp"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <fstream>

namespace external_storer
{

// These should become constexpr in a future compiler
inline const std::filesystem::path defPathPrefix{"/run/bmcweb"};
inline const std::filesystem::path redfishPrefix{"/redfish/v1"};
inline const std::filesystem::path jsonFilename{"index.json"};

// This class only holds configuration and accounting data for the hook.
// As for user-provided data, it is intentionally not here, as it is always
// fetched from the filesystem backing store when needed.
class Hook
{
  private:
    // Relative location of hook, after redfishPrefix, before instance
    std::filesystem::path pathBase;

    // Optional middle keyword, required by some schemas, example "Entries"
    std::filesystem::path pathMiddle;

    // Disallow these user instances, avoid already-existing path components
    std::vector<std::string> denyList;

    // Automatically expand these fields when listing the array of entries
    std::vector<std::string> expandList;

    // The root directory for local storage, changeable only for testing
    std::filesystem::path pathPrefix;

  public:
    Hook(const std::string& b, const std::string& m,
         const std::vector<std::string>& d, const std::vector<std::string>& e) :
        pathBase(b),
        pathMiddle(m), denyList(d), expandList(e), pathPrefix(defPathPrefix)
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

    // Utility functions for building up locations
    std::filesystem::path locBase() const;
    std::filesystem::path locInstance(const std::string& instance) const;
    std::filesystem::path locMiddle(const std::string& instance) const;
    std::filesystem::path locEntry(const std::string& instance,
                                   const std::string& entry) const;
    std::filesystem::path locToFileDir(const std::filesystem::path& loc) const;
    std::filesystem::path locToFileJson(const std::filesystem::path& loc) const;

    // For use only during testing
    void deleteAll(void);
    void setPathPrefix(const std::filesystem::path& newPrefix);
};

std::filesystem::path safeAppend(const std::filesystem::path& a,
                                 const std::filesystem::path& b)
{
    std::filesystem::path result{a};

    // Unfortunately, a / b returns surprising/wrong results if b is absolute
    if (b.is_absolute())
    {
        // The absolute path already starts with necessary directory separator
        result += b;
        return result;
    }

    result /= b;
    return result;
}

std::filesystem::path Hook::locBase(void) const
{
    return safeAppend(redfishPrefix, pathBase);
}

std::filesystem::path Hook::locInstance(const std::string& instance) const
{
    return safeAppend(locBase(), instance);
}

std::filesystem::path Hook::locMiddle(const std::string& instance) const
{
    // The middle component is optional, some schemas might not need it
    if (pathMiddle.empty())
    {
        return locInstance(instance);
    }

    return safeAppend(locInstance(instance), pathMiddle);
}

std::filesystem::path Hook::locEntry(const std::string& instance,
                                     const std::string& entry) const
{
    return safeAppend(locMiddle(instance), entry);
}

std::filesystem::path Hook::locToFileDir(const std::filesystem::path& loc) const
{
    return safeAppend(pathPrefix, loc);
}

std::filesystem::path
    Hook::locToFileJson(const std::filesystem::path& loc) const
{
    // Safe to use / operator here, jsonFilename constant always relative
    return locToFileDir(loc) / jsonFilename;
}

std::optional<nlohmann::json>
    readJsonFile(const std::filesystem::path& filename)
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

std::optional<std::streampos>
    writeJsonFile(const std::filesystem::path& filename,
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

// The "proposedName" should be a basename, with no directory separators
// Conservative filename rules to begin with, can relax later if needed
bool validateFilename(const std::filesystem::path& proposedName)
{
    if (!(crow::openbmc_mapper::validateFilename(proposedName)))
    {
        BMCWEB_LOG_ERROR << "Filename contains invalid characters";
        return false;
    }

    return true;
}

bool validateFilename(const std::filesystem::path& name,
                      const std::vector<std::string>& denyList)
{
    if (!(validateFilename(name)))
    {
        // Error message has already been printed
        return false;
    }

    // Must not be within the denylist
    if (std::find(denyList.begin(), denyList.end(), name) != denyList.end())
    {
        BMCWEB_LOG_ERROR << "Filename " << name << " is reserved";
        return false;
    }

    return true;
}

std::string extractId(const nlohmann::json& content)
{
    std::string id;

    if (content.is_object())
    {
        auto foundId = content.find("Id");
        if (foundId != content.end())
        {
            if (foundId->is_string())
            {
                id = foundId.value();
                if (!(id.empty()))
                {
                    return id;
                }
            }
        }
    }

    boost::uuids::random_generator gen;

    // Roll a random UUID for server-assigned ID
    id = boost::uuids::to_string(gen());
    BMCWEB_LOG_INFO << "Generated UUID " << id;

    return id;
}

void stripFieldsId(nlohmann::json& content)
{
    if (!(content.is_object()))
    {
        return;
    }

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

void stripFieldsMembers(nlohmann::json& content)
{
    if (!(content.is_object()))
    {
        return;
    }

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

void insertResponseLocation(crow::Response& response,
                            const std::string& location)
{
    // Add Location to header
    response.addHeader(boost::beast::http::field::location, location);

    // Add Location to body, but must dig through schema first
    if (!(response.jsonValue.is_object()))
    {
        BMCWEB_LOG_ERROR << "No Location because not object";
        return;
    }

    // ExtendedInfo must already be an array of at least 1 element (object)
    auto ei = response.jsonValue.find("@Message.ExtendedInfo");
    if (ei == response.jsonValue.end())
    {
        BMCWEB_LOG_ERROR << "No Location because no ExtendedInfo";
        return;
    }
    if (!(ei->is_array()))
    {
        BMCWEB_LOG_ERROR << "No Location because ExtendedInfo not array";
        return;
    }
    if (ei->empty())
    {
        BMCWEB_LOG_ERROR << "No Location because ExtendedInfo empty";
        return;
    }
    if (!((*ei)[0].is_object()))
    {
        BMCWEB_LOG_ERROR
            << "No Location because ExtendedInfo element not object";
        return;
    }

    // MessageArgs must be an array, create if it does not already exist
    auto ma = (*ei)[0].find("MessageArgs");
    if (ma == (*ei)[0].end())
    {
        (*ei)[0]["MessageArgs"] = nlohmann::json::array();
        ma = (*ei)[0].find("MessageArgs");
    }
    if (!(ma->is_array()))
    {
        BMCWEB_LOG_ERROR << "No Location because MessageArgs not array";
        return;
    }

    ma->emplace_back(location);
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

    std::string idInstance = extractId(content);
    stripFieldsId(content);

    auto innerContent = nlohmann::json::object();

    if (!(pathMiddle.empty()))
    {
        // Promote the inner layer to its own JSON object
        auto foundMiddle = content.find(pathMiddle);
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
            stripFieldsId(innerContent);

            // Trim "Members" as well, user not allowed bulk upload yet
            stripFieldsMembers(innerContent);
        }
    }

    if (!(validateFilename(idInstance, denyList)))
    {
        BMCWEB_LOG_ERROR << "Uploaded instance ID not acceptable";
        redfish::messages::actionParameterValueFormatError(
            asyncResp->res, idInstance, "Id", "POST");
        return;
    }

    std::filesystem::path outerUrl = locInstance(idInstance);
    std::filesystem::path outerDir = locToFileDir(outerUrl);

    std::error_code ec;

    if (std::filesystem::exists(outerDir, ec))
    {
        BMCWEB_LOG_ERROR << "Uploaded instance ID already exists on system";
        redfish::messages::resourceAlreadyExists(asyncResp->res, "String", "Id",
                                                 idInstance);
        return;
    }
    if (ec)
    {
        BMCWEB_LOG_ERROR << "Problem checking for " << outerDir
                         << " duplicate: " << ec.message();
        redfish::messages::operationFailed(asyncResp->res);
        return;
    }

    std::filesystem::path innerUrl = locMiddle(idInstance);
    std::filesystem::path innerDir = locToFileDir(innerUrl);

    // If no middle keyword, then no need to create multiple layers
    if (pathMiddle.empty())
    {
        innerDir = outerDir;
    }

    std::filesystem::create_directories(innerDir, ec);

    if (ec)
    {
        BMCWEB_LOG_ERROR << "Problem making " << innerDir
                         << " directories: " << ec.message();
        redfish::messages::operationFailed(asyncResp->res);
        return;
    }

    std::filesystem::path outerFilename = locToFileJson(outerUrl);

    if (!(writeJsonFile(outerFilename, content).has_value()))
    {
        BMCWEB_LOG_ERROR << "Problem writing file " << outerFilename;
        redfish::messages::operationFailed(asyncResp->res);
        return;
    }

    if (!(pathMiddle.empty()))
    {
        std::filesystem::path innerFilename = locToFileJson(innerUrl);

        if (!(writeJsonFile(innerFilename, innerContent).has_value()))
        {
            BMCWEB_LOG_ERROR << "Problem writing file " << innerFilename;
            redfish::messages::operationFailed(asyncResp->res);
            return;
        }
    }

    redfish::messages::created(asyncResp->res);

    insertResponseLocation(asyncResp->res, outerUrl);
}

void Hook::handleCreateMiddle(
    const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& idInstance)
{
    // Before doing anything with filesystem, validate naming restrictions
    if (!(validateFilename(idInstance, denyList)))
    {
        BMCWEB_LOG_ERROR << "Instance ID within URL is not acceptable";
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }

    nlohmann::json content;

    // Keep this in sync with post-I/O validation in readJsonFile()
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

    std::string idEntry = extractId(content);

    // Unlike instance, no need to do a second layer of trimming underneath
    stripFieldsId(content);

    // Unlike instance, names on denyList are perfectly OK for entry
    if (!(validateFilename(idEntry)))
    {
        BMCWEB_LOG_ERROR << "Uploaded entry ID not acceptable";
        redfish::messages::actionParameterValueFormatError(
            asyncResp->res, idEntry, "Id", "POST");
        return;
    }

    std::filesystem::path outerUrl = locInstance(idInstance);
    std::filesystem::path outerDir = locToFileDir(outerUrl);

    std::error_code ec;

    // The instance must already have been created earlier
    if (!(std::filesystem::exists(outerDir, ec)))
    {
        BMCWEB_LOG_ERROR << "Cannot add entry to nonexistent instance "
                         << idInstance;
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }
    if (ec)
    {
        BMCWEB_LOG_ERROR << "Problem checking for " << outerDir
                         << " existence: " << ec.message();
        redfish::messages::operationFailed(asyncResp->res);
        return;
    }

    std::filesystem::path entryUrl = locEntry(idInstance, idEntry);
    std::filesystem::path entryDir = locToFileDir(entryUrl);

    std::filesystem::create_directories(entryDir, ec);

    if (ec)
    {
        BMCWEB_LOG_ERROR << "Problem making " << entryDir
                         << " directories: " << ec.message();
        redfish::messages::operationFailed(asyncResp->res);
        return;
    }

    std::filesystem::path entryFilename = locToFileJson(entryUrl);

    if (std::filesystem::exists(entryFilename, ec))
    {
        BMCWEB_LOG_ERROR << "Uploaded entry ID already exists within instance";
        redfish::messages::resourceAlreadyExists(asyncResp->res, "String", "Id",
                                                 idEntry);
        return;
    }
    if (ec)
    {
        BMCWEB_LOG_ERROR << "Problem checking for " << entryFilename
                         << " duplicate: " << ec.message();
        redfish::messages::operationFailed(asyncResp->res);
        return;
    }

    if (!(writeJsonFile(entryFilename, content).has_value()))
    {
        BMCWEB_LOG_ERROR << "Problem writing file " << entryFilename;
        redfish::messages::operationFailed(asyncResp->res);
        return;
    }

    redfish::messages::created(asyncResp->res);

    insertResponseLocation(asyncResp->res, entryUrl);
}

void Hook::handleCreateEntry(
    const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& idInstance, const std::string& keywordMiddle)
{
    // Validate the middle path component in URL is the expected constant
    if (keywordMiddle != pathMiddle)
    {
        BMCWEB_LOG_ERROR << "URL middle path component is not " << pathMiddle;
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }

    // This handler has the same function as if the middle were omitted
    handleCreateMiddle(req, asyncResp, idInstance);
}

// Given a dir, list its subdirs, but only those subdirs which are themselves
// directories, and contain an "index.json" file within them.
// Those "index.json" files are only checked for existence, nothing more.
std::vector<std::filesystem::path>
    listJsonDirs(const std::filesystem::path& dir)
{
    std::vector<std::filesystem::path> files;
    std::error_code ec;

    // If containing directory not found, there can be no subdirectories
    if (!(std::filesystem::exists(dir, ec)))
    {
        BMCWEB_LOG_INFO << "Location " << dir << " nonexistent";
        return files;
    }
    if (ec)
    {
        BMCWEB_LOG_ERROR << "Problem checking for " << dir
                         << " existence: " << ec.message();
        return files;
    }

    // Old-style C++ iter loop, to get error checking, not using ranged for
    for (auto entries = std::filesystem::directory_iterator{dir};
         entries != std::filesystem::end(entries); entries.increment(ec))
    {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "Problem with " << dir
                             << " iterating: " << ec.message();
            break;
        }

        const auto& entry = *entries;

        // Only match directories
        if (!(entry.is_directory()))
        {
            continue;
        }

        auto dirBasename = entry.path().filename();

        // Validating against denyList not needed for entry, only for instance
        if (!(validateFilename(dirBasename)))
        {
            continue;
        }

        // Safe to use / operator here, jsonFilename constant always relative
        auto jsonWithin = entry.path() / jsonFilename;

        // The directory must contain the special JSON filename
        if (!(std::filesystem::exists(jsonWithin, ec)))
        {
            continue;
        }
        if (ec)
        {
            BMCWEB_LOG_ERROR << "Problem checking for " << jsonWithin
                             << " existence: " << ec.message();
            continue;
        }

        files.emplace_back(dirBasename);
    }

    return files;
}

// Returns all existing instances under this hook, as a list of basenames
std::vector<std::string> Hook::listInstances(void) const
{
    auto instanceDirs = listJsonDirs(locToFileDir(locBase()));

    std::vector<std::string> result;
    for (const auto& instanceDir : instanceDirs)
    {
        if (!(validateFilename(instanceDir, denyList)))
        {
            continue;
        }

        result.emplace_back(instanceDir.string());
    }

    return result;
}

void Hook::handleGetInstance(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& idInstance)
{
    // If optional middle keyword not in use, instance same as middle
    if (pathMiddle.empty())
    {
        return handleGetMiddle(asyncResp, idInstance, pathMiddle);
    }

    // Before doing anything with filesystem, validate naming restrictions
    if (!(validateFilename(idInstance, denyList)))
    {
        BMCWEB_LOG_ERROR << "Instance ID within URL is not acceptable";
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }

    auto outerUrl = locInstance(idInstance);
    auto outerFilename = locToFileJson(outerUrl);

    std::error_code ec;

    if (!(std::filesystem::exists(outerFilename, ec)))
    {
        BMCWEB_LOG_ERROR << "Instance not found with ID " << idInstance;
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }
    if (ec)
    {
        BMCWEB_LOG_ERROR << "Problem checking for " << outerFilename
                         << " existence: " << ec.message();
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }

    auto contentOpt = readJsonFile(outerFilename);
    if (!(contentOpt.has_value()))
    {
        BMCWEB_LOG_ERROR << "Problem reading file " << outerFilename;
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }

    auto& content = *contentOpt;

    // Regenerate these, as they were intentionally trimmed before storage
    content["Id"] = idInstance;
    content["@odata.id"] = outerUrl;

    auto innerUrl = locMiddle(idInstance);

    // Synthesize a correct link to middle layer
    auto middleObject = nlohmann::json::object();
    middleObject["@odata.id"] = innerUrl;
    content[pathMiddle] = middleObject;

    redfish::messages::success(asyncResp->res);

    asyncResp->res.jsonValue = std::move(content);
}

void Hook::handleGetMiddle(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& idInstance,
                           const std::string& keywordMiddle)
{
    // Before doing anything with filesystem, validate naming restrictions
    if (!(validateFilename(idInstance, denyList)))
    {
        BMCWEB_LOG_ERROR << "Instance ID within URL is not acceptable";
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }

    // Validate the middle path component in URL is the expected constant
    if (keywordMiddle != pathMiddle)
    {
        BMCWEB_LOG_ERROR << "URL middle path component is not " << pathMiddle;
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }

    auto innerUrl = locMiddle(idInstance);
    auto innerDir = locToFileDir(innerUrl);
    auto innerFilename = locToFileJson(innerUrl);

    std::error_code ec;

    if (!(std::filesystem::exists(innerFilename, ec)))
    {
        BMCWEB_LOG_ERROR << "Instance not found with ID " << idInstance;
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }
    if (ec)
    {
        BMCWEB_LOG_ERROR << "Problem checking for " << idInstance
                         << " existence: " << ec.message();
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }

    auto contentOpt = readJsonFile(innerFilename);
    if (!(contentOpt.has_value()))
    {
        BMCWEB_LOG_ERROR << "Problem reading file " << innerFilename;
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }

    auto& content = *contentOpt;

    // Regenerate these, as they were intentionally trimmed before storage
    content["Id"] = pathMiddle;
    content["@odata.id"] = innerUrl;

    // Do not pass denylist in here, it is only for instance, not entry
    auto files = listJsonDirs(innerDir);

    // Synthesize special "Members" array with links to all our entries
    auto membersArray = nlohmann::json::array();
    for (const auto& file : files)
    {
        // Safe to use / operator here, "file" already known to be relative
        std::filesystem::path entryUrl = innerUrl / file;

        auto fileObject = nlohmann::json::object();
        fileObject["@odata.id"] = entryUrl;
        fileObject["Id"] = file;

        // Automatically expand only the fields listed in expandList
        if (!(expandList.empty()))
        {
            std::filesystem::path entryFilename = locToFileJson(entryUrl);
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
                          const std::string& idInstance,
                          const std::string& keywordMiddle,
                          const std::string& idEntry)
{
    // Before doing anything with filesystem, validate naming restrictions
    if (!(validateFilename(idInstance, denyList)))
    {
        BMCWEB_LOG_ERROR << "Instance ID within URL is not acceptable";
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }

    // Validate the middle path component in URL is the expected constant
    if (keywordMiddle != pathMiddle)
    {
        BMCWEB_LOG_ERROR << "URL middle path component is not " << pathMiddle;
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }

    // Unlike instance, names on denyList are perfectly OK at this layer
    if (!(validateFilename(idEntry)))
    {
        BMCWEB_LOG_ERROR << "Entry ID within URL not acceptable";
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }

    auto entryUrl = locEntry(idInstance, idEntry);
    auto entryFilename = locToFileJson(entryUrl);

    std::error_code ec;

    if (!(std::filesystem::exists(entryFilename, ec)))
    {
        BMCWEB_LOG_ERROR << "Entry not found with ID " << idEntry;
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }
    if (ec)
    {
        BMCWEB_LOG_ERROR << "Problem checking for " << idEntry
                         << " existence: " << ec.message();
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }

    auto contentOpt = readJsonFile(entryFilename);
    if (!(contentOpt.has_value()))
    {
        BMCWEB_LOG_ERROR << "Problem reading file " << entryFilename;
        asyncResp->res.result(boost::beast::http::status::not_found);
        return;
    }

    auto& content = *contentOpt;

    // Regenerate these, as they were intentionally trimmed before storage
    content["Id"] = idEntry;
    content["@odata.id"] = entryUrl;

    redfish::messages::success(asyncResp->res);

    asyncResp->res.jsonValue = std::move(content);
}

void Hook::deleteAll(void)
{
    std::error_code ec;

    auto count = std::filesystem::remove_all(pathPrefix, ec);

    if (ec)
    {
        BMCWEB_LOG_ERROR << "Problem with " << pathPrefix
                         << " deleting: " << ec.message();
    }

    if (count > 0)
    {
        BMCWEB_LOG_INFO << "Deleted all " << count << " files/dirs from "
                        << pathPrefix;
    }
}

void Hook::setPathPrefix(const std::filesystem::path& newPrefix)
{
    // This function is only for testing, loudly warn if used
    BMCWEB_LOG_WARNING << "Changing path prefix to " << newPrefix;

    pathPrefix = newPrefix;
}

// Constructs a hook with known-good settings for usage with LogServices
inline Hook makeLogServices()
{
    const std::string pathBase{"Systems/system/LogServices"};
    const std::string midWord{"Entries"};

    // These names come from requestRoutesSystemLogServiceCollection()
    std::vector<std::string> denyList{"EventLog", "Dump", "Crashdump",
                                      "HostLogger"};

    // These names come from the "required" field of LogEntry JSON schema
    std::vector<std::string> expandList{"EntryType", "@odata.id", "@odata.type",
                                        "Id", "Name"};

    // Additional useful names to pre-expand
    expandList.emplace_back("Created");

    return {pathBase, midWord, denyList, expandList};
}

inline std::shared_ptr<Hook>
    rememberLogServices(const std::shared_ptr<Hook>& hookIncoming = nullptr)
{
    static std::shared_ptr<Hook> hookLogServices = nullptr;

    // If incoming pointer is valid, remember it for next time
    if (hookIncoming)
    {
        hookLogServices = hookIncoming;
    }

    return hookLogServices;
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
inline void requestRoutesExternalStorerLogServices(
    App& app, const std::shared_ptr<external_storer::Hook>& hook)
{
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

    // The integration point also needs to access the correct hook
    external_storer::rememberLogServices(hook);
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
    auto hookLogServices = std::make_shared<external_storer::Hook>(
        external_storer::makeLogServices());

    // The shared_ptr will be copied, stretching out its lifetime
    requestRoutesExternalStorerLogServices(app, hookLogServices);
}

} // namespace redfish
