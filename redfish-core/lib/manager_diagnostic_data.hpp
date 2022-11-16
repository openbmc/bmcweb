#pragma once

#include <app.hpp>
#include <async_resp.hpp>
#include <dbus_utility.hpp>
#include <http_request.hpp>
#include <nlohmann/json.hpp>
#include <privileges.hpp>
#include <routing.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>

#include <array>
#include <fstream>
#include <optional>
#include <string>
#include <thread>
#include <tuple>
#include <variant>
#include <vector>

namespace redfish
{

std::optional<int> isNumericPath(const std::string_view path)
{
    size_t p = path.rfind('/');
    if (p == std::string::npos)
    {
        return std::nullopt;
    }
    int id = 0;
    for (size_t i = p + 1; i < path.size(); ++i)
    {
        const char ch = path[i];
        if (ch < '0' || ch > '9')
        {
            return std::nullopt;
        }
        id = id * 10 + (ch - '0');
    }
    return id;
}

long getTicksPerSec()
{
    return sysconf(_SC_CLK_TCK);
}

std::string readFileIntoString(const std::string_view fileName)
{
    std::stringstream ss;
    std::ifstream ifs(fileName.data());
    while (ifs.good())
    {
        std::string line;
        std::getline(ifs, line);
        ss << line;
        if (ifs.good())
        {
            ss << std::endl;
        }
    }
    ifs.close();
    return ss.str();
}

struct ProcessStatistics
{
    int pid;
    int fileDescriptorCount;
    std::string tcomm;
    float utime;
    float stime;
    float uptimeSeconds;
    long memoryUsage;
    std::string objPath;
    std::string serviceName;
    uint restartCount;
};

ProcessStatistics parseTcommUtimeStimeString(std::string_view content,
                                             const long ticksPerSec)
{
    ProcessStatistics ret;
    ret.tcomm = "";
    ret.utime = ret.stime = 0;

    const float invTicksPerSec = 1.0f / static_cast<float>(ticksPerSec);

    // pCol now points to the first part in content after content is split by
    // space.
    // This is not ideal,
    std::string temp(content);
    char* pCol = strtok(temp.data(), " ");

    if (pCol != nullptr)
    {
        std::array<int,4> fields{1, 13, 14, 23}; // tcomm, utime, stime
        uint fieldIdx = 0;
        for (int colIdx = 0; colIdx < 24; ++colIdx)
        {
            if (fieldIdx < fields.size() && colIdx == fields[fieldIdx])
            {
                switch (fieldIdx)
                {
                    case 0:
                    {
                        ret.tcomm = std::string(pCol);
                        break;
                    }
                    case 1:
                        [[fallthrough]];
                    case 2:
                    {
                        int ticks = std::atoi(pCol);
                        float t = static_cast<float>(ticks) * invTicksPerSec;

                        if (fieldIdx == 1)
                        {
                            ret.utime = t;
                        }
                        else if (fieldIdx == 2)
                        {
                            ret.stime = t;
                        }
                        break;
                    }
                    case 3:
                    {
                        ret.memoryUsage = std::atol(pCol);
                    }
                }
                ++fieldIdx;
            }
            pCol = strtok(nullptr, " ");
        }
    }

    if (ticksPerSec <= 0)
    {
        BMCWEB_LOG_ERROR << "ticksPerSec is equal or less than zero\n";
    }

    return ret;
}

int getFdCount(const int pid)
{
    const std::string& fdPath = "/proc/" + std::to_string(pid) + "/fd";
    return std::distance(std::filesystem::directory_iterator(fdPath),
                         std::filesystem::directory_iterator{});
}

ProcessStatistics getProcessStatistics(const int pid, const long ticksPerSec,
                                       float millisNow)
{
    const std::string& statPath = "/proc/" + std::to_string(pid) + "/stat";
    ProcessStatistics ret =
        parseTcommUtimeStimeString(readFileIntoString(statPath), ticksPerSec);
    ret.pid = pid;
    struct stat t_stat;
    stat(("/proc/" + std::to_string(pid)).c_str(), &t_stat);
    struct timespec st_atim = t_stat.st_atim;
    float millisCtime = static_cast<float>(st_atim.tv_sec) * 1000.0f +
                        static_cast<float>(st_atim.tv_nsec) / 1000000.0f;
    ret.uptimeSeconds = (millisNow - millisCtime) / 1000.0f;
    ret.fileDescriptorCount = getFdCount(pid);
    return ret;
}

void populateDaemonStats(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         ProcessStatistics& ps)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, ps](const boost::system::error_code serviceNameEc,
                        dbus::utility::DbusVariantType& serviceName) mutable {
        if (serviceNameEc)
        {
            BMCWEB_LOG_ERROR << "Dbus call issue for pid " << ps.pid;
            return;
        }

        ps.serviceName = std::get<std::string>(serviceName);

        crow::connections::systemBus->async_method_call(
            [asyncResp,
             ps](const boost::system::error_code restartCountEc,
                 dbus::utility::DbusVariantType& restartCount) mutable {
            if (restartCountEc)
            {
                BMCWEB_LOG_ERROR << "Dbus call issue for pid " << ps.pid;
                return;
            }

            ps.restartCount = std::get<uint32_t>(restartCount);

            nlohmann::json processStat;
            processStat["CommandLine"] = ps.tcomm + " - " + ps.serviceName +
                                         " (" + std::to_string(ps.pid) + ")";
            processStat["NFileDescriptors"] = ps.fileDescriptorCount;
            processStat["KernelTimeSeconds"] = ps.stime;
            processStat["ResidentSetSizeBytes"] = ps.memoryUsage;
            processStat["RestartCount"] = ps.restartCount;
            processStat["UptimeSeconds"] = ps.uptimeSeconds;
            processStat["UserTimeSeconds"] = ps.utime;
            asyncResp->res.jsonValue["TopProcesses"].push_back(processStat);
            },
            "org.freedesktop.systemd1", ps.objPath.c_str(),
            "org.freedesktop.DBus.Properties", "Get",
            "org.freedesktop.systemd1.Service", "NRestarts");
        },
        "org.freedesktop.systemd1", ps.objPath.c_str(),
        "org.freedesktop.DBus.Properties", "Get",
        "org.freedesktop.systemd1.Unit", "Id");
}

void getDaemonStats(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                    ProcessStatistics& ps)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, ps](const boost::system::error_code ec,
                        sdbusplus::message::object_path& objPath) mutable {
        if (ec)
        {
            //BMCWEB_LOG_DEBUG << "This pid " << ps.pid << " has no systemd unit";
            nlohmann::json processStat;
            processStat["CommandLine"] =
                ps.tcomm + " (" + std::to_string(ps.pid) + ")";
            processStat["NFileDescriptors"] = ps.fileDescriptorCount;
            processStat["KernelTimeSeconds"] = ps.stime;
            processStat["ResidentSetSizeBytes"] = ps.memoryUsage;
            processStat["UptimeSeconds"] = ps.uptimeSeconds;
            processStat["UserTimeSeconds"] = ps.utime;
            //asyncResp->res.jsonValue["TopProcesses"].push_back(processStat);
            return;
        }

        // BMCWEB_LOG_DEBUG << "This pid " << ps.pid << " has service name "
        //                  << objPath.str;
        ps.objPath = objPath.str;
        populateDaemonStats(asyncResp, ps);
        },
        "org.freedesktop.systemd1", "/org/freedesktop/systemd1",
        "org.freedesktop.systemd1.Manager", "GetUnitByPID",
        static_cast<uint32_t>(ps.pid));
}

/**
 * populateProcessUptime populates the process uptime statistics.
 */
void populateProcessUptime(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    constexpr std::string_view procPath = "/proc/";
    long ticksPerSec = getTicksPerSec();
    struct timeval tv;
    gettimeofday(&tv, nullptr);

    std::vector<ProcessStatistics> pss;

    float millisNow = static_cast<float>(tv.tv_sec) * 1000.0f +
                      static_cast<float>(tv.tv_usec) / 1000.0f;
    for (const auto& procEntry : std::filesystem::directory_iterator(procPath))
    {
        const std::string& path = procEntry.path();
        std::optional<int> pid = isNumericPath(path);
        if (pid.has_value())
        {
            ProcessStatistics ps =
                getProcessStatistics(*pid, ticksPerSec, millisNow);
            pss.push_back(ps);
        }
    }

    std::sort(pss.begin(), pss.end(),
              [](const ProcessStatistics& a, const ProcessStatistics& b) {
        return a.uptimeSeconds > b.uptimeSeconds;
    });

    asyncResp->res.jsonValue["TopProcesses"] = nlohmann::json::array();
    for (auto& ps : pss)
    {
        getDaemonStats(asyncResp, ps);
    }
}

std::array<uint64_t, 3> parseBootInfo()
{
    const std::string& bootInfoPath = "/var/google/bootinfo";
    std::string bootinfoFile = readFileIntoString(bootInfoPath);
    char* col = strtok(bootinfoFile.data(), " ");

    // {Boot Count, Crash Count}
    std::array<uint64_t, 3> bootinfo{0, 0, 0};

    // If file does not exist, then just set boot and crash counts to 0

    for (size_t i = 0; i < bootinfo.size(); ++i)
    {
        if (col == NULL)
        {
            break;
        }

        bootinfo[i] = static_cast<uint64_t>(std::stoull(col));
        col = strtok(NULL, " ");
    }

    return bootinfo;
}

void populateBootInfo(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    std::array<uint64_t, 3> bootinfo = parseBootInfo();

    nlohmann::json bootStats;
    bootStats["BootCount"] = bootinfo[0];
    bootStats["CrashCount"] = bootinfo[1];

    asyncResp->res.jsonValue["BootInfo"] = bootStats;
}

/**
 * handleManagerDiagnosticData supports ManagerDiagnosticData.
 * It retrieves BMC health information from various DBus resources and returns
 * the information through the response.
 */
inline void handleManagerDiagnosticDataGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    asyncResp->res.jsonValue["@odata.type"] =
        "#ManagerDiagnosticData.v1_0_0.ManagerDiagnosticData";
    asyncResp->res.jsonValue["@odata.id"] =
        "/redfish/v1/Managers/bmc/ManagerDiagnosticData";
    asyncResp->res.jsonValue["Id"] = "ManagerDiagnosticData";
    asyncResp->res.jsonValue["Name"] = "Manager Diagnostic Data";

    populateProcessUptime(asyncResp);
    populateBootInfo(asyncResp);
}

void updateBootStats()
{
    crow::connections::systemBus->async_method_call(
        [](const boost::system::error_code ec,
           dbus::utility::DBusPropertiesMap& properties) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "Cannot get BMC host properties" << ec;
            return;
        }

        std::array<uint64_t, 3> bootinfo = parseBootInfo();
        bool updated = false;

        for (const auto& it : properties)
        {
            // If last reboot time is different then what is stored, then update
            // info
            if (it.first == "LastRebootTime" &&
                std::get<uint64_t>(it.second) != bootinfo[2])
            {
                updated = true;
                bootinfo[2] = std::get<uint64_t>(it.second);
            }

            if (it.first == "LastRebootCause" && updated)
            {
                ++bootinfo[0];
                if (std::get<std::string>(it.second) ==
                    "xyz.openbmc_project.State.BMC.RebootCause.Unknown")
                {
                    ++bootinfo[1];
                }
            }
        }

        if (updated)
        {
            std::ofstream bootinfoFile("/var/google/bootinfo");
            bootinfoFile << bootinfo[0] << " " << bootinfo[1] << " "
                         << bootinfo[2] << '\n';
            bootinfoFile.close();
        }
        },
        "xyz.openbmc_project.State.BMC", "/xyz/openbmc_project/state/bmc0",
        "org.freedesktop.DBus.Properties", "GetAll",
        "xyz.openbmc_project.State.BMC");
}

inline void requestRoutesManagerDiagnosticData(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/ManagerDiagnosticData")
        .privileges(redfish::privileges::getManagerDiagnosticData)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleManagerDiagnosticDataGet, std::ref(app)));

    updateBootStats();
}

} // namespace redfish
