#pragma once

#include "external_storer_instance.hpp"

#include <app.hpp>
#include <boost/beast/http/verb.hpp>
#include <registries/privilege_registry.hpp>

namespace external_storer
{

using Index = int;

// TODO(): Should map value be std::unique_ptr instead? Or an Instance itself?
using Instances = boost::container::flat_map<Index, std::shared_ptr<Instance>>;
using InstancesIter = Instances::iterator;

// Global container for all ExternalStorer instances on this server
Instances instances;

// Unfortunately, BMCWEB_ROUTE takes literals only, no variables
// Fall back to C macros, which will become literals
// Although unpleasant, this minimizes duplication of information
#define ES_URL_0 "/redfish/v1/Stuff/"
#define ES_URL_1 "/redfish/v1/MoreStuff/"

// Use literal string pasting to append suffix to each, similarly
#define ES_URL_SUFFIX "<str>/"
#define ES_URL_0_S ES_URL_0 ES_URL_SUFFIX
#define ES_URL_1_S ES_URL_1 ES_URL_SUFFIX

} // namespace external_storer

namespace redfish
{

inline void newExternalStorer(external_storer::Index index,
                              const std::string& url)
{
    // Ensure each index number is only initialized once
    if (external_storer::instances.find(index) !=
        external_storer::instances.end())
    {
        return;
    }

    auto newInstance = std::make_shared<external_storer::Instance>(url);

    external_storer::instances[index] = newInstance;
}

inline void initExternalStorer()
{
    // TODO(): This is obviously only an example
    newExternalStorer(0, ES_URL_0);
    newExternalStorer(1, ES_URL_1);
}

inline void handleExternalStorerCollectionGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    // URL not modifiable at runtime, thus no locking required
    asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/ExternalStorers";
    asyncResp->res.jsonValue["@odata.type"] =
        "#ExternalStorerService.ExternalStorerService";
    asyncResp->res.jsonValue["Name"] = "ExternalStorer Service";
    asyncResp->res.jsonValue["Description"] = "ExternalStorer Service";

    // Build up the Members array with URL to all the instances
    nlohmann::json& members = asyncResp->res.jsonValue["Members"];
    members = nlohmann::json::array();
    for (const auto& instance : external_storer::instances)
    {
        members.emplace_back(
            nlohmann::json{{"@odata.id", instance.second->getUrl()}});
    }

    asyncResp->res.jsonValue["Members@odata.count"] =
        external_storer::instances.size();
}

inline void requestRoutesExternalStorerInstances(App& app)
{
    // Request routes for each of the instances
    // URL must be literal, can't use a loop, must cut and paste, sigh

    // TODO(): Perhaps wrap these in a further macro to reduce repetition
    BMCWEB_ROUTE(app, ES_URL_0)
        .privileges(redfish::privileges::getExternalStorer)
        .methods(boost::beast::http::verb::get)(
            [i = external_storer::instances[0]](
                const crow::Request&,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                i->handleContainerGet(asyncResp);
            });
    BMCWEB_ROUTE(app, ES_URL_0)
        .privileges(redfish::privileges::getExternalStorer)
        .methods(boost::beast::http::verb::post)(
            [i = external_storer::instances[0]](
                const crow::Request& req,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                i->handleContainerPost(req, asyncResp);
            });

    BMCWEB_ROUTE(app, ES_URL_1)
        .privileges(redfish::privileges::getExternalStorer)
        .methods(boost::beast::http::verb::get)(
            [i = external_storer::instances[1]](
                const crow::Request&,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                i->handleContainerGet(asyncResp);
            });
    BMCWEB_ROUTE(app, ES_URL_1)
        .privileges(redfish::privileges::getExternalStorer)
        .methods(boost::beast::http::verb::post)(
            [i = external_storer::instances[1]](
                const crow::Request& req,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                i->handleContainerPost(req, asyncResp);
            });
}

inline void requestRoutesExternalStorerEntries(App& app)
{
    // Request routes for individual entries within the instances
    // URL must be literal, can't use a loop, must cut and paste, sigh

    BMCWEB_ROUTE(app, ES_URL_0_S)
        .privileges(redfish::privileges::getExternalStorer)
        .methods(boost::beast::http::verb::get)(
            [i = external_storer::instances[0]](
                const crow::Request&,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const std::string& param) {
                i->handleEntryGet(asyncResp, param);
            });
    BMCWEB_ROUTE(app, ES_URL_0_S)
        .privileges(redfish::privileges::getExternalStorer)
        .methods(boost::beast::http::verb::put)(
            [i = external_storer::instances[0]](
                const crow::Request& req,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const std::string& param) {
                i->handleEntryPut(req, asyncResp, param);
            });
    BMCWEB_ROUTE(app, ES_URL_0_S)
        .privileges(redfish::privileges::getExternalStorer)
        .methods(boost::beast::http::verb::delete_)(
            [i = external_storer::instances[0]](
                const crow::Request&,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const std::string& param) {
                i->handleEntryDelete(asyncResp, param);
            });

    BMCWEB_ROUTE(app, ES_URL_1_S)
        .privileges(redfish::privileges::getExternalStorer)
        .methods(boost::beast::http::verb::get)(
            [i = external_storer::instances[1]](
                const crow::Request&,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const std::string& param) {
                i->handleEntryGet(asyncResp, param);
            });
    BMCWEB_ROUTE(app, ES_URL_1_S)
        .privileges(redfish::privileges::getExternalStorer)
        .methods(boost::beast::http::verb::put)(
            [i = external_storer::instances[1]](
                const crow::Request& req,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const std::string& param) {
                i->handleEntryPut(req, asyncResp, param);
            });
    BMCWEB_ROUTE(app, ES_URL_1_S)
        .privileges(redfish::privileges::getExternalStorer)
        .methods(boost::beast::http::verb::delete_)(
            [i = external_storer::instances[1]](
                const crow::Request&,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const std::string& param) {
                i->handleEntryDelete(asyncResp, param);
            });
}

inline void requestRoutesExternalStorerCollection(App& app)
{
    initExternalStorer();

    BMCWEB_ROUTE(app, "/redfish/v1/ExternalStorers/")
        .privileges(redfish::privileges::getExternalStorer)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                handleExternalStorerCollectionGet(asyncResp);
            });

    requestRoutesExternalStorerInstances(app);
    requestRoutesExternalStorerEntries(app);
}

} // namespace redfish
