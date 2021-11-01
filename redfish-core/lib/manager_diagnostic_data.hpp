#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

namespace redfish
{

std::string readFileThenGrepIntoString(const std::string_view fileName,
                                       const std::string_view grepStr = "")
{
    std::stringstream ss;
    std::ifstream ifs(fileName.data());
    while (ifs.good())
    {
        std::string line;
        std::getline(ifs, line);
        if (line.find(grepStr) != std::string::npos)
        {
            ss << line;
        }
        if (ifs.good())
            ss << std::endl;
    }
    return ss.str();
}

// Returns true if successfully parsed and false otherwise. If parsing was
// successful, value is set accordingly.
// Input: "MemAvailable:      1234 kB"
// Returns true, value set to 1234
bool parseMeminfoValue(const std::string_view content,
                       const std::string_view keyword, int& value)
{
    size_t p = content.find(keyword);
    if (p != std::string::npos)
    {
        std::string_view v = content.substr(p + keyword.size());
        p = v.find("kB");
        if (p != std::string::npos)
        {
            v = v.substr(0, p);
            value = std::atoi(v.data());
            return true;
        }
    }
    return false;
}

void managerDiagnosticDataGetMemoryInfo(int& memAvailable, int& slab,
                                        int& kernelStack)
{
    std::string meminfoBuffer = readFileThenGrepIntoString("/proc/meminfo");

    std::string_view sv(meminfoBuffer.data());
    parseMeminfoValue(sv, "MemAvailable:", memAvailable);
    parseMeminfoValue(sv, "Slab:", slab);
    parseMeminfoValue(sv, "KernelStack:", kernelStack);
}

} // namespace redfish