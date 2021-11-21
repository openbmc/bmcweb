#pragma once

#include <boost/container/flat_map.hpp>
#include <sdbusplus/message/types.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <string>
#include <optional>
#include <vector>
#include <memory>
#include "nlohmann/json.hpp"
#include "../../include/async_resp_class_definition.hpp"

namespace dbus
{

namespace utility
{

using DbusVariantType =
    std::variant<std::vector<std::tuple<std::string, std::string, std::string>>,
                 std::vector<std::string>, std::vector<double>, std::string,
                 int64_t, uint64_t, double, int32_t, uint32_t, int16_t,
                 uint16_t, uint8_t, bool>;
using DBusPropertiesMap =
    boost::container::flat_map<std::string, DbusVariantType>;
using DBusInteracesMap =
    boost::container::flat_map<std::string, DBusPropertiesMap>;
using ManagedObjectType =
    std::vector<std::pair<sdbusplus::message::object_path, DBusInteracesMap>>;

}

}

namespace redfish
{

struct HealthPopulate : std::enable_shared_from_this<HealthPopulate>
{
    HealthPopulate(const std::shared_ptr<bmcweb::AsyncResp>& asyncRespIn);
    HealthPopulate(const std::shared_ptr<bmcweb::AsyncResp>& asyncRespIn,
                   nlohmann::json& status);

    ~HealthPopulate();

    // this should only be called once per url, others should get updated by
    // being added as children to the 'main' health object for the page
    void populate();

    void getGlobalPath();

    void getAllStatusAssociations();

    std::shared_ptr<bmcweb::AsyncResp> asyncResp;
    nlohmann::json& jsonStatus;

    // we store pointers to other HealthPopulate items so we can update their
    // members and reduce dbus calls. As we hold a shared_ptr to them, they get
    // destroyed last, and they need not call populate()
    std::vector<std::shared_ptr<HealthPopulate>> children;

    // self is used if health is for an individual items status, as this is the
    // 'lowest most' item, the rollup will equal the health
    std::optional<std::string> selfPath;

    std::vector<std::string> inventory;
    bool isManagersHealth = false;
    dbus::utility::ManagedObjectType statuses;
    std::string globalInventoryPath = "-"; // default to illegal dbus path
    bool populated = false;
};

}