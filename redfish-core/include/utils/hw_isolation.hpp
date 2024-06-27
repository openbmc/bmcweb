#pragma once

#include <dbus_utility.hpp>
#include <error_messages.hpp>
#include <sdbusplus/message.hpp>
#include <utils/error_log_utils.hpp>
#include <utils/time_utils.hpp>

#include <memory>
#include <string>
#include <vector>

namespace redfish
{
namespace hw_isolation_utils
{

/**
 * @brief API used to return ChassisPowerStateOffRequiredError with
 *        the Chassis id that will get by using the given resource
 *        object to perform some PATCH operation on the resource object.
 *
 * @param[in] asyncResp - The redfish response to return to the caller.
 * @param[in] resourceObjPath - The redfish resource dbus object path.
 *
 * @return The redfish response in given response buffer.
 */
inline void retChassisPowerStateOffRequiredError(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const sdbusplus::message::object_path& resourceObjPath)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, resourceObjPath](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetAncestorsResponse& ancestors) {
        if (ec)
        {
            BMCWEB_LOG_ERROR(
                "DBUS response error [{} : {}] when tried to get parent Chassis id for the given resource object [{}]",
                ec.value(), ec.message(), resourceObjPath.str);
            messages::internalError(asyncResp->res);
            return;
        }

        if (ancestors.empty())
        {
            BMCWEB_LOG_ERROR(
                "The given resource object [{}] is not the child of the Chassis so failed return ChassisPowerStateOffRequiredError in the response",
                resourceObjPath.str);
            messages::internalError(asyncResp->res);
            return;
        }

        if (ancestors.size() > 1)
        {
            // Should not happen since GetAncestors returs parent object
            // from the given child object path and we are just looking
            // for parent chassis object id alone, so we should get one
            // element.
            BMCWEB_LOG_ERROR(
                "The given resource object [{}] is contains more than one Chassis as parent so failed return ChassisPowerStateOffRequiredError in the response",
                resourceObjPath.str);
            messages::internalError(asyncResp->res);
            return;
        }
        messages::chassisPowerStateOffRequired(
            asyncResp->res,
            sdbusplus::message::object_path(ancestors.begin()->first)
                .filename());
    },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetAncestors", resourceObjPath.str,
        std::array<const char*, 1>{
            "xyz.openbmc_project.Inventory.Item.Chassis"});
}

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

        // The Enabled property value will be "false" to isolate.
        constexpr bool enabledPropVal = false;

        if (std::string_view(
                "xyz.openbmc_project.Common.Error.InvalidArgument") ==
            dbusError->name)
        {
            messages::propertyValueExternalConflict(
                asyncResp->res, "Enabled",
                std::to_string(static_cast<int>(enabledPropVal)));
        }
        else if (std::string_view(
                     "xyz.openbmc_project.Common.Error.NotAllowed") ==
                 dbusError->name)
        {
            retChassisPowerStateOffRequiredError(asyncResp, resourceObjPath);
        }
        else if (
            std::string_view(
                "xyz.openbmc_project.HardwareIsolation.Error.IsolatedAlready") ==
            dbusError->name)
        {
            messages::resourceAlreadyExists(
                asyncResp->res, resourceName, "Enabled",
                std::to_string(static_cast<int>(enabledPropVal)));
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
 */
inline void
    deisolateResource(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const sdbusplus::message::object_path& resourceObjPath,
                      const std::string& hwIsolationDbusName)
{
    // Get the HardwareIsolation entry by using the given resource
    // associations endpoints
    dbus::utility::getAssociationEndPoints(
        resourceObjPath.str + "/isolated_hw_entry",
        [asyncResp, resourceObjPath, hwIsolationDbusName](
            const boost::system::error_code& ec,
            const dbus::utility::MapperEndPoints& vEndpoints) {
        if (ec)
        {
            BMCWEB_LOG_ERROR(
                "DBus response error [{} : {}] when tried to get the hardware isolation entry for the given resource dbus object path: ",
                ec.value(), ec.message(), resourceObjPath.str);
            // The error code (53 == Invalid request descriptor) will be
            // returned if dbus doesn't contains "isolated_hw_entry" for
            // the given resource i.e it is not isolated to deisolate.
            // This case might occur when resource are in the certain state
            if (ec.value() == EBADR)
            {
                messages::propertyValueConflict(asyncResp->res, "Enabled",
                                                "Status.State");
            }
            else
            {
                messages::internalError(asyncResp->res);
            }
            return;
        }

        std::string resourceIsolatedHwEntry;
        resourceIsolatedHwEntry = vEndpoints.back();

        // De-isolate the given resource
        crow::connections::systemBus->async_method_call(
            [asyncResp, resourceIsolatedHwEntry,
             resourceObjPath](const boost::system::error_code& ec1,
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
                retChassisPowerStateOffRequiredError(asyncResp,
                                                     resourceObjPath);
            }
            else if (
                std::string_view(
                    "xyz.openbmc_project.Common.Error.InsufficientPermission") ==
                dbusError->name)
            {
                messages::resourceCannotBeDeleted(asyncResp->res);
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
    });
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
 * @param[in] parentSubtreePath - The resource parent subtree path to get
 *                                the resource object path.
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
    bool enabled, const std::vector<std::string_view>& interfaces,
    const std::string& parentSubtreePath = "/xyz/openbmc_project/inventory")
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
        parentSubtreePath, 0, resourceIfaces);
}

/*
 * @brief The helper API to set the Redfish severity level base on
 *        the given severity.
 *
 * @param[in] asyncResp - The redfish response to return.
 * @param[in] objPath - The D-Bus object path of the given severity.
 * @param[in] severityPropPath - The Redfish severity property json path.
 * @param[in] severityVal - The D-Bus object severity.
 *
 * @return True on success
 *         False on failure and set the error in the redfish response.
 */
inline bool
    setSeverity(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const sdbusplus::message::object_path& objPath,
                const nlohmann::json_pointer<nlohmann::json>& severityPropPath,
                const std::string& severityVal)
{
    std::string sevPropPath = severityPropPath.to_string();
    if (severityVal ==
        "xyz.openbmc_project.Logging.Event.SeverityLevel.Critical")
    {
        asyncResp->res.jsonValue[sevPropPath] = "Critical";
    }
    else if ((severityVal ==
              "xyz.openbmc_project.Logging.Event.SeverityLevel.Warning") ||
             (severityVal ==
              "xyz.openbmc_project.Logging.Event.SeverityLevel.Unknown"))
    {
        asyncResp->res.jsonValue[sevPropPath] = "Warning";
    }
    else if (severityVal ==
             "xyz.openbmc_project.Logging.Event.SeverityLevel.Ok")
    {
        asyncResp->res.jsonValue[sevPropPath] = "OK";
    }
    else
    {
        BMCWEB_LOG_ERROR("Unsupported Severity[{}] from object: {}",
                         severityVal, objPath.str);
        messages::internalError(asyncResp->res);
        return false;
    }
    return true;
}

static void
    assembleEventProperties(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const dbus::utility::DBusPropertiesMap& properties,
                            nlohmann::json& condition, const std::string& path)
{
    using AssociationsValType =
        std::vector<std::tuple<std::string, std::string, std::string>>;
    const AssociationsValType* associations = nullptr;
    const uint64_t* timestamp = nullptr;
    const std::string* msgPropVal = nullptr;
    const std::string* severity = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "Associations",
        associations, "Timestamp", timestamp, "Message", msgPropVal, "Severity",
        severity);

    if (!success)
    {
        messages::internalError(asyncResp->res);
        BMCWEB_LOG_ERROR("Could not read one or more properties from {}", path);
        return;
    }

    if (associations != nullptr)
    {
        for (const auto& assoc : *associations)
        {
            if (std::get<0>(assoc) == "error_log")
            {
                sdbusplus::message::object_path errPath = std::get<2>(assoc);
                // we have only one condition
                nlohmann::json_pointer<nlohmann::json> logEntryPropPath(
                    "/Status/Conditions/0/LogEntry");
                error_log_utils::setErrorLogUri(asyncResp, errPath,
                                                logEntryPropPath, true);
            }
        }
    }

    if (timestamp != nullptr)
    {
        condition["Timestamp"] = redfish::time_utils::getDateTimeStdtime(
            static_cast<std::time_t>(*timestamp));
    }

    if (msgPropVal != nullptr)
    {
        // Host recovered even if there is hardware
        // isolation entry so change the state.
        if (*msgPropVal == "Recovered")
        {
            asyncResp->res.jsonValue["Status"]["State"] = "Enabled";
        }

        const redfish::registries::Message* msgReg =
            registries::getMessage("OpenBMC.0.2.HardwareIsolationReason");

        if (msgReg == nullptr)
        {
            BMCWEB_LOG_ERROR(
                "Failed to get the HardwareIsolationReason message registry to add in the condition");
            messages::internalError(asyncResp->res);
            return;
        }

        // Prepare MessageArgs as per defined in the
        // MessageRegistries
        std::vector<std::string> messageArgs{*msgPropVal};

        // Fill the "msgPropVal" as reason
        std::string message = msgReg->message;
        int i = 0;
        for (const std::string& messageArg : messageArgs)
        {
            std::string argIndex = "%" + std::to_string(++i);
            size_t argPos = message.find(argIndex);
            if (argPos != std::string::npos)
            {
                message.replace(argPos, argIndex.length(), messageArg);
            }
        }
        // Severity will be added based on the event
        // object property
        condition["Message"] = message;
        condition["MessageArgs"] = messageArgs;
        condition["MessageId"] = "OpenBMC.0.2.HardwareIsolationReason";
    }

    if (severity != nullptr)
    {
        // we have only one condition
        nlohmann::json_pointer<nlohmann::json> severityPropPath(
            "/Status/Conditions/0/Severity");
        if (!setSeverity(asyncResp, path, severityPropPath, *severity))
        {
            // Failed to set the severity
            return;
        }
    }
}

/*
 * @brief The helper API to set the Redfish Status conditions based on
 *        the given resource event log association.
 *
 * @param[in] asyncResp - The redfish response to return.
 * @param[in] resourceObjPath - The resource D-Bus object object.
 *
 * @return NULL
 */
inline void
    getHwIsolationStatus(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const sdbusplus::message::object_path& resourceObjPath)
{
    dbus::utility::getAssociationEndPoints(
        resourceObjPath.str + "/event_log",
        [asyncResp,
         resourceObjPath](const boost::system::error_code& ec,
                          const dbus::utility::MapperEndPoints& vEndpoints) {
        if (ec)
        {
            if (ec.value() == EBADR)
            {
                // No event so the respective hardware status doesn't need
                // any Redfish status condition
                return;
            }
            BMCWEB_LOG_ERROR(
                "DBus response error [{} : {}] when tried to get the hardware status event for the given resource dbus object path: {} EBADR: {}",
                ec.value(), ec.message(), resourceObjPath.str, EBADR);
            messages::internalError(asyncResp->res);
            return;
        }

        bool found = false;
        std::string hwStatusEventObj;
        for (const auto& endpoint : vEndpoints)
        {
            if (sdbusplus::message::object_path(endpoint)
                    .parent_path()
                    .filename() == "hw_isolation_status")
            {
                hwStatusEventObj = endpoint;
                found = true;
                break;
            }
        }

        if (!found)
        {
            // No event so the respective hardware status doesn't need
            // any Redfish status condition
            return;
        }

        // Get the dbus service name of the hardware status event object
        crow::connections::systemBus->async_method_call(
            [asyncResp,
             hwStatusEventObj](const boost::system::error_code& ec1,
                               const dbus::utility::MapperGetObject& objType) {
            if (ec1)
            {
                BMCWEB_LOG_ERROR(
                    "DBUS response error [{} : {}] when tried to get the dbus name of the hardware status event object {}",
                    ec1.value(), ec1.message(), hwStatusEventObj);
                messages::internalError(asyncResp->res);
                return;
            }

            if (objType.size() > 1)
            {
                BMCWEB_LOG_ERROR(
                    "More than one dbus service implemented the same hardware status event object {}",
                    hwStatusEventObj);
                messages::internalError(asyncResp->res);
                return;
            }

            if (objType[0].first.empty())
            {
                BMCWEB_LOG_ERROR(
                    "The retrieved hardware status event object dbus name is empty");
                messages::internalError(asyncResp->res);
                return;
            }

            // Get event properties and fill into status conditions
            sdbusplus::asio::getAllProperties(
                *crow::connections::systemBus, objType[0].first,
                hwStatusEventObj, "",
                [asyncResp, hwStatusEventObj](
                    const boost::system::error_code& ec2,
                    const dbus::utility::DBusPropertiesMap& properties) {
                if (ec2)
                {
                    BMCWEB_LOG_ERROR(
                        "DBUS response error [{} : {}] when tried to get the hardware status event object properties {}",
                        ec2.value(), ec2.message(), hwStatusEventObj);
                    messages::internalError(asyncResp->res);
                    return;
                }

                // Event is exist and that will get created when
                // the respective hardware is not functional so
                // set the state as "Disabled".
                asyncResp->res.jsonValue["Status"]["State"] = "Disabled";

                nlohmann::json& conditions =
                    asyncResp->res.jsonValue["Status"]["Conditions"];
                conditions = nlohmann::json::array();
                conditions.push_back(nlohmann::json::object());
                nlohmann::json& condition = conditions.back();

                assembleEventProperties(asyncResp, properties, condition,
                                        hwStatusEventObj);
            });
        },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetObject", hwStatusEventObj,
            std::array<const char*, 1>{"xyz.openbmc_project.Logging.Event"});
    });
}

} // namespace hw_isolation_utils
} // namespace redfish
