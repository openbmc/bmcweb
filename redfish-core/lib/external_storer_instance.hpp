#pragma once

#include <boost/convert.hpp>
#include <boost/convert/strtol.hpp>

namespace external_storer
{

// TODO(): These should be configurable on a per-instance basis
const size_t limitBytes = 1048576;
const size_t limitCount = 1024;

class Entry
{
  public:
    nlohmann::json content;
    size_t size;

  public:
    Entry(const nlohmann::json& j, size_t s) : content(j), size(s)
    {}
};

using SeqNum = int64_t;

using Entries = boost::container::flat_map<SeqNum, std::shared_ptr<Entry>>;
using EntriesIter = Entries::iterator;

class Instance
{
  public:
    explicit Instance(const std::string& u) :
        url(u), lock(), next(1), entries(), sizeBytes(0), sizeCount(0)
    {}

    Instance(const Instance& copy) = delete;
    Instance& operator=(const Instance& assign) = delete;

    void handleContainerGet(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
    {
        std::lock_guard<std::mutex> guard(lock);

        asyncResp->res.jsonValue["@odata.id"] = url;

        // TODO(): Customize @odata.type per-instance
        asyncResp->res.jsonValue["@odata.type"] =
            "#ExternalStorerCollection.ExternalStorerCollection";

        // TODO(): These are filler and might not be needed
        asyncResp->res.jsonValue["Name"] = "ExternalStorer Collection";
        asyncResp->res.jsonValue["Description"] = "ExternalStorer Collection";

        // Build up the Members array with URL to all the elements
        // TODO(): Honor the other query parameters
        nlohmann::json& members = asyncResp->res.jsonValue["Members"];
        members = nlohmann::json::array();
        for (const auto& entry : entries)
        {
            // TODO(): Honor $expand parameter
            members.emplace_back(
                nlohmann::json{{"@odata.id", seqToUrl(entry.first)}});
        }

        asyncResp->res.jsonValue["Members@odata.count"] = sizeCount;
        // TODO(): If, for any reason (or maybe only if output is truncated),
        //  the last ID dumped was not the last ID in Entries,
        //  provide nextLink with $filter=Id gt lastIdDumped.
    }

    void
        handleContainerPost(const crow::Request& req,
                            const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
    {
        std::lock_guard<std::mutex> guard(lock);

        auto newEntry = inputToNewEntry(asyncResp, req.body);
        if (!newEntry)
        {
            // Error message already included
            return;
        }

        ++sizeCount;
        sizeBytes += (*newEntry).size;

        BMCWEB_LOG_DEBUG << "Storage now " << sizeCount << " items "
                         << sizeBytes << " bytes";

        while (sizeCount > limitCount)
        {
            jettisonOldest();
            BMCWEB_LOG_DEBUG << "Count reduced to " << sizeCount;
        }
        while (sizeBytes > limitBytes)
        {
            jettisonOldest();
            BMCWEB_LOG_DEBUG << "Bytes reduced to " << sizeBytes;
        }

        auto newSeq = next;
        ++next;

        entries[newSeq] = std::make_shared<Entry>(*newEntry);

        asyncResp->res.addHeader(boost::beast::http::field::location,
                                 seqToUrl(newSeq));

        BMCWEB_LOG_DEBUG << "Created ID: " << std::to_string(newSeq);
    }

    void handleEntryGet(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& param)
    {
        std::lock_guard<std::mutex> guard(lock);

        auto entry = paramToFoundEntry(asyncResp, param);
        if (!entry)
        {
            // Error message already included
            return;
        }

        auto seq = (*entry)->first;

        asyncResp->res.jsonValue = (*entry)->second->content;

        // Only fill this in if user did not provide it
        if (asyncResp->res.jsonValue.find("@odata.type") ==
            asyncResp->res.jsonValue.end())
        {
            asyncResp->res.jsonValue["@odata.type"] =
                "#ExternalStorerEntry.ExternalStorerEntry";
        }

        // Synthesize "@odata.id" and "Id" from the sequence number
        asyncResp->res.jsonValue["@odata.id"] = seqToUrl(seq);
        asyncResp->res.jsonValue["Id"] = seq;
    }

    void handleEntryPut(const crow::Request& req,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& param)
    {
        std::lock_guard<std::mutex> guard(lock);

        // TODO(): IMPLEMENT
        // TODO(): this uses the lookup of GET followed by the input acceptance
        // of POST
        (void)req;
        (void)param;
        redfish::messages::noOperation(asyncResp->res);
    }

    void handleEntryDelete(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& param)
    {
        std::lock_guard<std::mutex> guard(lock);

        // TODO(): IMPLEMENT
        // TODO(): this uses the lookup of GET followed by deletion
        (void)param;
        redfish::messages::noOperation(asyncResp->res);
    }

    std::string getUrl() const
    {
        // URL not modifiable at runtime, thus no locking required
        return url;
    }

  private:
    std::optional<EntriesIter>
        paramToFoundEntry(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& param)
    {
        auto convSeq = boost::convert<SeqNum>(param, boost::cnv::strtol());
        if (!convSeq)
        {
            redfish::messages::invalidObject(asyncResp->res, param);
            return std::nullopt;
        }

        SeqNum seq = *convSeq;
        auto found = entries.find(seq);
        if (found == entries.end())
        {
            // TODO(): invalidIndex only takes a 32-bit number, sigh
            redfish::messages::invalidIndex(asyncResp->res,
                                            static_cast<int>(seq));
            return std::nullopt;
        }

        return {found};
    }

    std::optional<Entry>
        inputToNewEntry(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& input)
    {
        nlohmann::json content;

        content = nlohmann::json::parse(input, nullptr, false);
        if (content.is_discarded())
        {
            BMCWEB_LOG_INFO << "Uploaded content not JSON";
            redfish::messages::malformedJSON(asyncResp->res);
            return std::nullopt;
        }
        if (!(content.is_object()))
        {
            BMCWEB_LOG_INFO << "Uploaded JSON type not a dictionary";
            redfish::messages::unrecognizedRequestBody(asyncResp->res);
            return std::nullopt;
        }

        size_t size;

        // Use size of dumped string as proxy for size of entry object itself
        // Round-trip parsing also checks for string encoding errors
        try
        {
            size = content.dump().size();
        }
        catch (nlohmann::json::exception&)
        {
            BMCWEB_LOG_INFO << "Uploaded JSON contains malformed string";
            redfish::messages::unrecognizedRequestBody(asyncResp->res);
            return std::nullopt;
        }

        // Ensure user did not send an empty dictionary
        auto foundAnything = content.begin();
        if (foundAnything == content.end())
        {
            BMCWEB_LOG_INFO << "Uploaded JSON contains empty dictionary";
            redfish::messages::unrecognizedRequestBody(asyncResp->res);
            return std::nullopt;
        }

        // Ensure user did not try to overwrite our reserved fields
        auto foundOdataId = content.find("@odata.id");
        if (foundOdataId != content.end())
        {
            BMCWEB_LOG_INFO << "Uploaded JSON has reserved @odata.id field";
            redfish::messages::invalidObject(asyncResp->res, "@odata.id");
            return std::nullopt;
        }

        auto foundId = content.find("Id");
        if (foundId != content.end())
        {
            BMCWEB_LOG_INFO << "Uploaded JSON has reserved Id field";
            redfish::messages::invalidObject(asyncResp->res, "Id");
            return std::nullopt;
        }

        // Uploaded content is deemed acceptable
        BMCWEB_LOG_DEBUG << "Upload accepted, raw " << input.size()
                         << " cooked " << size << " bytes";
        return {{content, size}};
    }

    std::string seqToUrl(SeqNum seq) const
    {
        std::string s = url;

        auto len = s.size();
        if (len < 1)
        {
            BMCWEB_LOG_ERROR << "Internal error: Empty URL";
            return s;
        }

        // Add slash as separator, but only if necessary
        if (s[len - 1] != '/')
        {
            s += '/';
        }

        s += std::to_string(seq);
        return s;
    }

    void jettisonOldest(void)
    {
        auto oldest = entries.begin();
        if (oldest == entries.end())
        {
            BMCWEB_LOG_ERROR << "Internal error: Jettison of empty map";
            return;
        }

        auto seq = oldest->first;
        auto oldSize = oldest->second->size;

        oldest->second.reset();
        entries.erase(oldest);

        sizeBytes -= oldSize;
        --sizeCount;

        BMCWEB_LOG_DEBUG << "ExternalStorer jettisoned: "
                         << std::to_string(seq);
    }

  private:
    std::string url;
    std::mutex lock;
    SeqNum next;
    Entries entries;
    size_t sizeBytes;
    size_t sizeCount;
};

} // namespace external_storer
