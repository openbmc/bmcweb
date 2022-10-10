#pragma once

#include <app.hpp>
#include <async_resp.hpp>
#include <dbus_utility.hpp>
#include <http_request.hpp>
#include <nlohmann/json.hpp>
#include <privileges.hpp>
#include <routing.hpp>

#include <array>
#include <optional>
#include <string>

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
    return ss.str();
}

struct ProcessStatistics
{
    int pid = 0;
    std::string tcomm = "";
    float utime = 0.0F;
    float stime = 0.0F;
    float uptimeSeconds = 0.0F;
};

ProcessStatistics parseTcommUtimeStimeString(std::string_view content,
                                             const long ticksPerSec)
{
    ProcessStatistics ret;
    ret.tcomm = "";
    ret.utime = ret.stime = 0;

    const float invTicksPerSec = 1.0F / static_cast<float>(ticksPerSec);

    // pCol now points to the first part in content after content is split by
    // space.
    // This is not ideal,
    std::string temp(content);
    char* pCol = strtok(temp.data(), " ");

    if (pCol != nullptr)
    {
        std::array<int,3> fields{1, 13, 14}; // tcomm, utime, stime
        uint fieldIdx = 0;
        for (int colIdx = 0; colIdx < 15; ++colIdx)
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
    float millisCtime = static_cast<float>(st_atim.tv_sec) * 1000.0F +
                        static_cast<float>(st_atim.tv_nsec) / 1000000.0F;
    ret.uptimeSeconds = (millisNow - millisCtime) / 1000.0F;
    return ret;
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

    float millisNow = static_cast<float>(tv.tv_sec) * 1000.0F +
                      static_cast<float>(tv.tv_usec) / 1000.0F;
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

    nlohmann::json processStats = nlohmann::json::array();
    for (const auto& ps : pss)
    {
        nlohmann::json processStat;
        processStat["CommandLine"] = std::to_string(ps.pid) + " " + ps.tcomm;
        processStat["UserTimeSeconds"] = ps.utime;
        processStat["KernelTimeSeconds"] = ps.stime;
        processStat["UptimeSeconds"] = ps.uptimeSeconds;
        processStats.push_back(processStat);
    }
    asyncResp->res.jsonValue["TopProcesses"] = processStats;
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
}

inline void requestRoutesManagerDiagnosticData(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/ManagerDiagnosticData")
        .privileges(redfish::privileges::getManagerDiagnosticData)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleManagerDiagnosticDataGet, std::ref(app)));
}

} // namespace redfish
