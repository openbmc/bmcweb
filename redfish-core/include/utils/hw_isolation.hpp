#pragma once

#include <error_messages.hpp>
#include <sdbusplus/message.hpp>

#include <memory>
#include <string>
#include <vector>

namespace redfish
{
namespace hw_isolation_utils
{

/**
 * @brief API used to isolate the given resource
 *
 * @param[in] asyncResp - The redfish response to return to the caller.
 * @param[in] resourceName - The redfish resource name which trying to isolate.
 * @param[in] resourceId - The redfish resource id which trying to isolate.
 * @param[in] resourceObjPath - The redfish resource dbus object path.
 * @param[in] hwIsolationDbusName - The HardwareIsolation dbus name which is
 *                                  hosting isolation dbus interfaces.
 *
 * @return The redfish response in given response buffer.
 *
 * @note This function will return the appropriate error based on the isolation
 *       dbus "Create" interface error.
 */
inline void
    isolateResource(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                    const std::string& resourceName,
                    const std::string& resourceId,
                    const sdbusplus::message::object_path& resourceObjPath,
                    const std::string& hwIsolationDbusName)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, resourceName, resourceId,
         resourceObjPath](const boost::system::error_code& ec,
                          const sdbusplus::message::message& msg) {
        if (!ec)
        {
            messages::success(asyncResp->res);
            return;
        }

        BMCWEB_LOG_ERROR(
            "DBUS response error [{} : {}] when tried to isolate the given resource: {}",
            ec.value(), ec.message(), resourceObjPath.str);

        const sd_bus_error* dbusError = msg.get_error();
        if (dbusError == nullptr)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        BMCWEB_LOG_ERROR("DBus ErrorName: {} ErrorMsg: {}", dbusError->name,
                         dbusError->message);

        if (std::string_view(
                "xyz.openbmc_project.Common.Error.InvalidArgument") ==
            dbusError->name)
        {
            constexpr bool isolate = false;
            messages::propertyValueIncorrect(
                asyncResp->res, "@odata.id",
                std::to_string(static_cast<int>(isolate)));
        }
        else if (std::string_view(
                     "xyz.openbmc_project.Common.Error.NotAllowed") ==
                 dbusError->name)
        {
            messages::propertyNotWritable(asyncResp->res, "Enabled");
        }
        else if (std::string_view(
                     "xyz.openbmc_project.Common.Error.Unavailable") ==
                 dbusError->name)
        {
            messages::resourceInStandby(asyncResp->res);
        }
        else if (
            std::string_view(
                "xyz.openbmc_project.HardwareIsolation.Error.IsolatedAlready") ==
            dbusError->name)
        {
            messages::resourceAlreadyExists(asyncResp->res, "@odata.id",
                                            resourceName, resourceId);
        }
        else if (std::string_view(
                     "xyz.openbmc_project.Common.Error.TooManyResources") ==
                 dbusError->name)
        {
            messages::createLimitReachedForResource(asyncResp->res);
        }
        else
        {
            BMCWEB_LOG_ERROR(
                "DBus Error is unsupported so returning as Internal Error");
            messages::internalError(asyncResp->res);
        }
        return;
    },
        hwIsolationDbusName, "/xyz/openbmc_project/hardware_isolation",
        "xyz.openbmc_project.HardwareIsolation.Create", "Create",
        resourceObjPath,
        "xyz.openbmc_project.HardwareIsolation.Entry.Type.Manual");
}

/**
 * @brief API used to deisolate the given resource
 *
 * @param[in] asyncResp - The redfish response to return to the caller.
 * @param[in] resourceObjPath - The redfish resource dbus object path.
 * @param[in] hwIsolationDbusName - The HardwareIsolation dbus name which is
 *                                  hosting isolation dbus interfaces.
 *
 * @return The redfish response in given response buffer.
 *
 * @note - This function will try to identify the hardware isolated dbus entry
 *         from associations endpoints by using the given resource dbus object
 *         of "isolated_hw_entry".
 *       - This function will use the last endpoint from the list since the
 *         HardwareIsolation manager may be used the "Resolved" dbus entry
 *         property to indicate the deisolation instead of delete
 *         the entry object.
 */
inline void
    deisolateResource(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const sdbusplus::message::object_path& resourceObjPath,
                      const std::string& hwIsolationDbusName)
{
    // Get the HardwareIsolation entry by using the given resource
    // associations endpoints
    crow::connections::systemBus->async_method_call(
        [asyncResp, resourceObjPath, hwIsolationDbusName](
            boost::system::error_code& ec,
            const std::variant<std::vector<std::string>>& vEndpoints) {
        if (ec)
        {
            BMCWEB_LOG_ERROR(
                "DBus response error [{} : {}] when tried to get the hardware isolation entry for the given resource dbus object path: ",
                ec.value(), ec.message(), resourceObjPath.str);
            messages::internalError(asyncResp->res);
            return;
        }

        std::string resourceIsolatedHwEntry;
        const std::vector<std::string>* endpoints =
            std::get_if<std::vector<std::string>>(&(vEndpoints));
        if (endpoints == nullptr)
        {
            BMCWEB_LOG_ERROR(
                "Failed to get Associations endpoints for the given object path: {}",
                resourceObjPath.str);
            messages::internalError(asyncResp->res);
            return;
        }
        resourceIsolatedHwEntry = endpoints->back();

        // De-isolate the given resource
        crow::connections::systemBus->async_method_call(
            [asyncResp,
             resourceIsolatedHwEntry](const boost::system::error_code& ec1,
                                      const sdbusplus::message::message& msg) {
            if (!ec1)
            {
                messages::success(asyncResp->res);
                return;
            }

            BMCWEB_LOG_ERROR(
                "DBUS response error [{} : {}] when tried to isolate the given resource: {}",
                ec1.value(), ec1.message(), resourceIsolatedHwEntry);

            const sd_bus_error* dbusError = msg.get_error();

            if (dbusError == nullptr)
            {
                messages::internalError(asyncResp->res);
                return;
            }

            BMCWEB_LOG_ERROR("DBus ErrorName: {} ErrorMsg: {}", dbusError->name,
                             dbusError->message);

            if (std::string_view(
                    "xyz.openbmc_project.Common.Error.NotAllowed") ==
                dbusError->name)
            {
                messages::propertyNotWritable(asyncResp->res, "Entry");
            }
            else if (std::string_view(
                         "xyz.openbmc_project.Common.Error.Unavailable") ==
                     dbusError->name)
            {
                messages::resourceInStandby(asyncResp->res);
            }
            else
            {
                BMCWEB_LOG_ERROR(
                    "DBus Error is unsupported so returning as Internal Error");
                messages::internalError(asyncResp->res);
            }
            return;
        },
            hwIsolationDbusName, resourceIsolatedHwEntry,
            "xyz.openbmc_project.Object.Delete", "Delete");
    },
        "xyz.openbmc_project.ObjectMapper",
        resourceObjPath.str + "/isolated_hw_entry",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Association", "endpoints");
}

/**
 * @brief API used to process hardware (aka resource) isolation request
 *        This API can be used to any redfish resource if that redfish
 *        supporting isolation feature (the resource can be isolate
 *        from system boot)
 *
 * @param[in] asyncResp - The redfish response to return to the caller.
 * @param[in] resourceName - The redfish resource name which trying to isolate.
 * @param[in] resourceId - The redfish resource id which trying to isolate.
 * @param[in] enabled - The redfish resource "Enabled" property value.
 * @param[in] interfaces - The redfish resource dbus interfaces which will use
 *                         to get the given resource dbus objects from
 *                         the inventory.
 *
 * @return The redfish response in given response buffer.
 *
 * @note - This function will identify the given resource dbus object from
 *         the inventory by using the given resource dbus interfaces along
 *         with "Object:Enable" interface (which is used to map the "Enabled"
 *         redfish property to dbus "Enabled" property - The "Enabled" is
 *         used to do isolate the resource from system boot) and the given
 *         redfish resource "Id".
 *       - This function will do either isolate or deisolate based on the
 *         given "Enabled" property value.
 */
inline void processHardwareIsolationReq(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& resourceName, const std::string& resourceId,
    bool enabled, const std::vector<std::string_view>& interfaces)
{
    std::vector<std::string_view> resourceIfaces(interfaces.begin(),
                                                 interfaces.end());
    resourceIfaces.emplace_back("xyz.openbmc_project.Object.Enable");

    // Make sure the given resourceId is present in inventory
    crow::connections::systemBus->async_method_call(
        [asyncResp, resourceName, resourceId,
         enabled](boost::system::error_code& ec,
                  const dbus::utility::MapperGetSubTreePathsResponse& objects) {
        if (ec)
        {
            BMCWEB_LOG_ERROR(
                "DBus response error [{} : {}] when tried to check the given resource is present in the inventory",
                ec.value(), ec.message());
            messages::internalError(asyncResp->res);
            return;
        }

        sdbusplus::message::object_path resourceObjPath;
        for (const auto& object : objects)
        {
            sdbusplus::message::object_path path(object);
            if (path.filename() == resourceId)
            {
                resourceObjPath = path;
                break;
            }
        }

        if (resourceObjPath.str.empty())
        {
            messages::resourceNotFound(asyncResp->res, resourceName,
                                       resourceId);
            return;
        }

        // Get the HardwareIsolation DBus name
        crow::connections::systemBus->async_method_call(
            [asyncResp, resourceObjPath, enabled, resourceName,
             resourceId](const boost::system::error_code& ec1,
                         const dbus::utility::MapperGetObject& objType) {
            if (ec1)
            {
                BMCWEB_LOG_ERROR(
                    "DBUS response error [{} : {}] when tried to get the HardwareIsolation dbus name to isolate: ",
                    ec1.value(), ec1.message(), resourceObjPath.str);
                messages::internalError(asyncResp->res);
                return;
            }

            if (objType.size() > 1)
            {
                BMCWEB_LOG_ERROR(
                    "More than one dbus service implemented HardwareIsolation");
                messages::internalError(asyncResp->res);
                return;
            }

            if (objType[0].first.empty())
            {
                BMCWEB_LOG_ERROR(
                    "The retrieved HardwareIsolation dbus name is empty");
                messages::internalError(asyncResp->res);
                return;
            }

            // Make sure whether need to isolate or de-isolate
            // the given resource
            if (!enabled)
            {
                isolateResource(asyncResp, resourceName, resourceId,
                                resourceObjPath, objType[0].first);
            }
            else
            {
                deisolateResource(asyncResp, resourceObjPath, objType[0].first);
            }
        },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetObject",
            "/xyz/openbmc_project/hardware_isolation",
            std::array<const char*, 1>{
                "xyz.openbmc_project.HardwareIsolation.Create"});
    },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
        "/xyz/openbmc_project/inventory", 0, resourceIfaces);
}

} // namespace hw_isolation_utils
} // namespace redfish
