#include <nlohmann/json.hpp>
#include "../../include/dbus_singleton.hpp"
#include <boost/beast/http/parser.hpp>
#include <boost/beast/core/flat_static_buffer.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/circular_buffer/space_optimized.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/http/read.hpp>
#include "health_class_decl.hpp"

#include "task.hpp"
#include <app_class_decl.hpp>
#include <boost/algorithm/string.hpp>

#include "update_service.hpp"

namespace redfish
{

void requestRoutesTaskMonitor(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/TaskService/Tasks/<str>/Monitor/")
        .privileges(redfish::privileges::getTask)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& strParam) {
                auto find = std::find_if(
                    task::tasks.begin(), task::tasks.end(),
                    [&strParam](const std::shared_ptr<task::TaskData>& task) {
                        if (!task)
                        {
                            return false;
                        }

                        // we compare against the string version as on failure
                        // strtoul returns 0
                        return std::to_string(task->index) == strParam;
                    });

                if (find == task::tasks.end())
                {
                    messages::resourceNotFound(asyncResp->res, "Monitor",
                                               strParam);
                    return;
                }
                std::shared_ptr<task::TaskData>& ptr = *find;
                // monitor expires after 204
                if (ptr->gave204)
                {
                    messages::resourceNotFound(asyncResp->res, "Monitor",
                                               strParam);
                    return;
                }
                ptr->populateResp(asyncResp->res);
            });
}

void requestRoutesTask(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/TaskService/Tasks/<str>/")
        .privileges(redfish::privileges::getTask)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& strParam) {
                auto find = std::find_if(
                    task::tasks.begin(), task::tasks.end(),
                    [&strParam](const std::shared_ptr<task::TaskData>& task) {
                        if (!task)
                        {
                            return false;
                        }

                        // we compare against the string version as on failure
                        // strtoul returns 0
                        return std::to_string(task->index) == strParam;
                    });

                if (find == task::tasks.end())
                {
                    messages::resourceNotFound(asyncResp->res, "Tasks",
                                               strParam);
                    return;
                }

                std::shared_ptr<task::TaskData>& ptr = *find;

                asyncResp->res.jsonValue["@odata.type"] = "#Task.v1_4_3.Task";
                asyncResp->res.jsonValue["Id"] = strParam;
                asyncResp->res.jsonValue["Name"] = "Task " + strParam;
                asyncResp->res.jsonValue["TaskState"] = ptr->state;
                asyncResp->res.jsonValue["StartTime"] =
                    crow::utility::getDateTime(ptr->startTime);
                if (ptr->endTime)
                {
                    asyncResp->res.jsonValue["EndTime"] =
                        crow::utility::getDateTime(*(ptr->endTime));
                }
                asyncResp->res.jsonValue["TaskStatus"] = ptr->status;
                asyncResp->res.jsonValue["Messages"] = ptr->messages;
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/TaskService/Tasks/" + strParam;
                if (!ptr->gave204)
                {
                    asyncResp->res.jsonValue["TaskMonitor"] =
                        "/redfish/v1/TaskService/Tasks/" + strParam +
                        "/Monitor";
                }
                if (ptr->payload)
                {
                    const task::Payload& p = *(ptr->payload);
                    asyncResp->res.jsonValue["Payload"] = {
                        {"TargetUri", p.targetUri},
                        {"HttpOperation", p.httpOperation},
                        {"HttpHeaders", p.httpHeaders},
                        {"JsonBody",
                         p.jsonBody.dump(
                             2, ' ', true,
                             nlohmann::json::error_handler_t::replace)}};
                }
                asyncResp->res.jsonValue["PercentComplete"] =
                    ptr->percentComplete;
            });
}

void requestRoutesTaskCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/TaskService/Tasks/")
        .privileges(redfish::privileges::getTaskCollection)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#TaskCollection.TaskCollection";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/TaskService/Tasks";
                asyncResp->res.jsonValue["Name"] = "Task Collection";
                asyncResp->res.jsonValue["Members@odata.count"] =
                    task::tasks.size();
                nlohmann::json& members = asyncResp->res.jsonValue["Members"];
                members = nlohmann::json::array();

                for (const std::shared_ptr<task::TaskData>& task : task::tasks)
                {
                    if (task == nullptr)
                    {
                        continue; // shouldn't be possible
                    }
                    members.emplace_back(nlohmann::json{
                        {"@odata.id", "/redfish/v1/TaskService/Tasks/" +
                                          std::to_string(task->index)}});
                }
            });
}

void requestRoutesTaskService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/TaskService/")
        .privileges(redfish::privileges::getTaskService)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#TaskService.v1_1_4.TaskService";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/TaskService";
                asyncResp->res.jsonValue["Name"] = "Task Service";
                asyncResp->res.jsonValue["Id"] = "TaskService";
                asyncResp->res.jsonValue["DateTime"] =
                    crow::utility::getDateTimeOffsetNow().first;
                asyncResp->res.jsonValue["CompletedTaskOverWritePolicy"] =
                    "Oldest";

                asyncResp->res.jsonValue["LifeCycleEventOnTaskStateChange"] =
                    true;

                auto health = std::make_shared<HealthPopulate>(asyncResp);
                health->populate();
                asyncResp->res.jsonValue["Status"]["State"] = "Enabled";
                asyncResp->res.jsonValue["ServiceEnabled"] = true;
                asyncResp->res.jsonValue["Tasks"] = {
                    {"@odata.id", "/redfish/v1/TaskService/Tasks"}};
            });
}

constexpr char const* crashdumpObject = "com.intel.crashdump";
constexpr char const* crashdumpPath = "/com/intel/crashdump";
constexpr char const* crashdumpInterface = "com.intel.crashdump";
constexpr char const* deleteAllInterface =
    "xyz.openbmc_project.Collection.DeleteAll";
constexpr char const* crashdumpOnDemandInterface =
    "com.intel.crashdump.OnDemand";
constexpr char const* crashdumpTelemetryInterface =
    "com.intel.crashdump.Telemetry";

void createDumpTaskCallback(const crow::Request& req,
                           const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const uint32_t& dumpId, const std::string& dumpPath,
                           const std::string& dumpType)
{
    std::shared_ptr<task::TaskData> task = task::TaskData::createTask(
        [dumpId, dumpPath, dumpType](
            boost::system::error_code err, sdbusplus::message::message& m,
            const std::shared_ptr<task::TaskData>& taskData) {
            if (err)
            {
                BMCWEB_LOG_ERROR << "Error in creating a dump";
                taskData->state = "Cancelled";
                return task::completed;
            }
            std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::variant<std::string>>>>>
                interfacesList;

            sdbusplus::message::object_path objPath;

            m.read(objPath, interfacesList);

            if (objPath.str ==
                "/xyz/openbmc_project/dump/" +
                    std::string(boost::algorithm::to_lower_copy(dumpType)) +
                    "/entry/" + std::to_string(dumpId))
            {
                nlohmann::json retMessage = messages::success();
                taskData->messages.emplace_back(retMessage);

                std::string headerLoc =
                    "Location: " + dumpPath + std::to_string(dumpId);
                taskData->payload->httpHeaders.emplace_back(
                    std::move(headerLoc));

                taskData->state = "Completed";
                return task::completed;
            }
            return task::completed;
        },
        "type='signal',interface='org.freedesktop.DBus."
        "ObjectManager',"
        "member='InterfacesAdded', "
        "path='/xyz/openbmc_project/dump'");

    task->startTimer(std::chrono::minutes(3));
    task->populateResp(asyncResp->res);
    task->payload.emplace(req);
}

void requestRoutesCrashdumpCollect(App& app)
{
    //Note: Deviated from redfish privilege registry for GET & HEAD
    //method for security reasons.
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/LogServices/Crashdump/"
                      "Actions/LogService.CollectDiagnosticData/")
        // The below is incorrect;  Should be ConfigureManager
        //.privileges(redfish::privileges::postLogService)
        .privileges({{"ConfigureComponents"}})
        .methods(
            boost::beast::http::verb::
                post)([](const crow::Request& req,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
            std::string diagnosticDataType;
            std::string oemDiagnosticDataType;
            if (!redfish::json_util::readJson(
                    req, asyncResp->res, "DiagnosticDataType",
                    diagnosticDataType, "OEMDiagnosticDataType",
                    oemDiagnosticDataType))
            {
                return;
            }

            if (diagnosticDataType != "OEM")
            {
                BMCWEB_LOG_ERROR
                    << "Only OEM DiagnosticDataType supported for Crashdump";
                messages::actionParameterValueFormatError(
                    asyncResp->res, diagnosticDataType, "DiagnosticDataType",
                    "CollectDiagnosticData");
                return;
            }

            auto collectCrashdumpCallback = [asyncResp, req](
                                                const boost::system::error_code
                                                    ec,
                                                const std::string&) {
                if (ec)
                {
                    if (ec.value() ==
                        boost::system::errc::operation_not_supported)
                    {
                        messages::resourceInStandby(asyncResp->res);
                    }
                    else if (ec.value() ==
                             boost::system::errc::device_or_resource_busy)
                    {
                        messages::serviceTemporarilyUnavailable(asyncResp->res,
                                                                "60");
                    }
                    else
                    {
                        messages::internalError(asyncResp->res);
                    }
                    return;
                }
                std::shared_ptr<task::TaskData> task =
                    task::TaskData::createTask(
                        [](boost::system::error_code err,
                           sdbusplus::message::message&,
                           const std::shared_ptr<task::TaskData>& taskData) {
                            if (!err)
                            {
                                taskData->messages.emplace_back(
                                    messages::taskCompletedOK(
                                        std::to_string(taskData->index)));
                                taskData->state = "Completed";
                            }
                            return task::completed;
                        },
                        "type='signal',interface='org.freedesktop.DBus."
                        "Properties',"
                        "member='PropertiesChanged',arg0namespace='com.intel."
                        "crashdump'");
                task->startTimer(std::chrono::minutes(5));
                task->populateResp(asyncResp->res);
                task->payload.emplace(req);
            };

            if (oemDiagnosticDataType == "OnDemand")
            {
                crow::connections::systemBus->async_method_call(
                    std::move(collectCrashdumpCallback), crashdumpObject,
                    crashdumpPath, crashdumpOnDemandInterface,
                    "GenerateOnDemandLog");
            }
            else if (oemDiagnosticDataType == "Telemetry")
            {
                crow::connections::systemBus->async_method_call(
                    std::move(collectCrashdumpCallback), crashdumpObject,
                    crashdumpPath, crashdumpTelemetryInterface,
                    "GenerateTelemetryLog");
            }
            else
            {
                BMCWEB_LOG_ERROR << "Unsupported OEMDiagnosticDataType: "
                                 << oemDiagnosticDataType;
                messages::actionParameterValueFormatError(
                    asyncResp->res, oemDiagnosticDataType,
                    "OEMDiagnosticDataType", "CollectDiagnosticData");
                return;
            }
        });
}

}