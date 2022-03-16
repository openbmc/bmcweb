#pragma once

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
    crow::connections::systemBus->async_method_call(
        [asyncResp, resourceObjPath](
            boost::system::error_code& ec,
            const std::variant<std::vector<std::string>>& vEndpoints) {
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

        bool found = false;
        std::string hwStatusEventObj;
        for (const auto& endpoint : *endpoints)
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

            using AssociationsValType =
                std::vector<std::tuple<std::string, std::string, std::string>>;
            using HwStausEventPropertiesType = boost::container::flat_map<
                std::string,
                std::variant<std::string, uint64_t, AssociationsValType>>;

            // Get event properties and fill into status conditions
            crow::connections::systemBus->async_method_call(
                [asyncResp, hwStatusEventObj](
                    const boost::system::error_code& ec2,
                    const HwStausEventPropertiesType& properties) {
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

                for (const auto& property : properties)
                {
                    if (property.first == "Associations")
                    {
                        const AssociationsValType* associations =
                            std::get_if<AssociationsValType>(&property.second);
                        if (associations == nullptr)
                        {
                            BMCWEB_LOG_ERROR(
                                "Failed to get the Associations from object: {}",
                                hwStatusEventObj);
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        for (const auto& assoc : *associations)
                        {
                            if (std::get<0>(assoc) == "error_log")
                            {
                                sdbusplus::message::object_path errPath =
                                    std::get<2>(assoc);
                                // we have only one condition
                                nlohmann::json_pointer<nlohmann::json>
                                    logEntryPropPath(
                                        "/Status/Conditions/0/LogEntry");
                                error_log_utils::setErrorLogUri(
                                    asyncResp, errPath, logEntryPropPath, true);
                            }
                        }
                    }
                    else if (property.first == "Timestamp")
                    {
                        const uint64_t* timestamp =
                            std::get_if<uint64_t>(&property.second);
                        if (timestamp == nullptr)
                        {
                            BMCWEB_LOG_ERROR(
                                "Failed to get the Timestamp from object: {}",
                                hwStatusEventObj);
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        condition["Timestamp"] =
                            redfish::time_utils::getDateTimeStdtime(
                                static_cast<std::time_t>(*timestamp));
                    }
                    else if (property.first == "Message")
                    {
                        const std::string* msgPropVal =
                            std::get_if<std::string>(&property.second);
                        if (msgPropVal == nullptr)
                        {
                            BMCWEB_LOG_ERROR(
                                "Failed to get the Message from object: {}",
                                hwStatusEventObj);
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        // Host recovered even if there is hardware
                        // isolation entry so change the state.
                        if (*msgPropVal == "Recovered")
                        {
                            asyncResp->res.jsonValue["Status"]["State"] =
                                "Enabled";
                        }

                        const redfish::registries::Message* msgReg =
                            registries::getMessage(
                                "OpenBMC.0.2.HardwareIsolationReason");

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
                                message.replace(argPos, argIndex.length(),
                                                messageArg);
                            }
                        }
                        // Severity will be added based on the event
                        // object property
                        condition["Message"] = message;
                        condition["MessageArgs"] = messageArgs;
                        condition["MessageId"] =
                            "OpenBMC.0.2.HardwareIsolationReason";
                    }
                    else if (property.first == "Severity")
                    {
                        const std::string* severity =
                            std::get_if<std::string>(&property.second);
                        if (severity == nullptr)
                        {
                            BMCWEB_LOG_ERROR(
                                "Failed to get the Severity from object: {}",
                                hwStatusEventObj);
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        // we have only one condition
                        nlohmann::json_pointer<nlohmann::json> severityPropPath(
                            "/Status/Conditions/0/Severity");
                        if (!setSeverity(asyncResp, hwStatusEventObj,
                                         severityPropPath, *severity))
                        {
                            // Failed to set the severity
                            return;
                        }
                    }
                }
            },
                objType[0].first, hwStatusEventObj,
                "org.freedesktop.DBus.Properties", "GetAll", "");
        },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetObject", hwStatusEventObj,
            std::array<const char*, 1>{"xyz.openbmc_project.Logging.Event"});
    },
        "xyz.openbmc_project.ObjectMapper", resourceObjPath.str + "/event_log",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Association", "endpoints");
}

} // namespace hw_isolation_utils
} // namespace redfish
