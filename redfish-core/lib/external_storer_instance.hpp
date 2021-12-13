#pragma once

#include "external_storer_entry.hpp"

#include <boost/convert.hpp>
#include <boost/convert/strtol.hpp>

namespace external_storer
{

using Entries = boost::container::flat_map<SeqNum, Entry>;

class Instance
{
  public:
    explicit Instance(const std::string& u) : url(u), lock(), next(1), entries()
    {}
    Instance(const Instance& copy) = delete;
    Instance& operator=(const Instance& assign) = delete;

    void handleContainerGet(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
    {
        std::lock_guard<std::mutex> guard(lock);

        asyncResp->res.jsonValue["@odata.id"] = url;

        // TODO(): Customize @odata.type, Name, Description per-instance
        asyncResp->res.jsonValue["@odata.type"] =
            "#ExternalStorerCollection.ExternalStorerCollection";
        asyncResp->res.jsonValue["Name"] = "ExternalStorer Collection";
        asyncResp->res.jsonValue["Description"] = "ExternalStorer Collection";

        // Build up the Members array with URL to all the elements
        // TODO(): Honor the other query parameters
        nlohmann::json& members = asyncResp->res.jsonValue["Members"];
        members = nlohmann::json::array();
        for (const auto& entry : entries)
        {
            // TODO(): Honor $expand parameter
            members.emplace_back(nlohmann::json{
                {"@odata.id", url + '/' + std::to_string(entry.first)}});
        }

        asyncResp->res.jsonValue["Members@odata.count"] = entries.size();
        // TODO(): If Entries is not empty, and the last ID dumped is not the
        // last ID in Entries, provide nextLink
    }

    void handleEntryGet(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& param)
    {
        std::lock_guard<std::mutex> guard(lock);

        auto convSeq = boost::convert<SeqNum>(param, boost::cnv::strtol());
        if (!convSeq)
        {
            redfish::messages::invalidObject(asyncResp->res, param);
            return;
        }

        SeqNum seq = *convSeq;
        auto found = entries.find(seq);
        if (found == entries.end())
        {
            // TODO(): invalidIndex only takes a 32-bit number, sigh
            redfish::messages::invalidIndex(asyncResp->res,
                                            static_cast<int>(seq));
            return;
        }

        // TODO(): Customize @odata.type, Name, Description per-instance
        // TODO(): Should the individual differ from the collection here?
        asyncResp->res.jsonValue["@odata.type"] =
            "#ExternalStorerEntry.ExternalStorerEntry";
        asyncResp->res.jsonValue["Name"] = "ExternalStorer Entry";
        asyncResp->res.jsonValue["Description"] = "ExternalStorer Entry";

        // TODO(): JSON from user goes here

        // Synthesize "@odata.id" and "Id" from the sequence number
        // Do this last, to ensure overwrite of any user-given JSON here
        asyncResp->res.jsonValue["@odata.id"] = url + '/' + std::to_string(seq);
        asyncResp->res.jsonValue["Id"] = seq;
    }

    std::string getUrl() const
    {
        // URL not modifiable at runtime, thus no locking required
        return url;
    }

  private:
    std::string url;
    std::mutex lock;
    SeqNum next;
    Entries entries;
};

} // namespace external_storer
