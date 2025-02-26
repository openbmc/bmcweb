#include <unistd.h>

#include <array>
#include <climits>
#include <string>

inline std::string getHostName()
{
    std::string hostName;

    std::array<char, HOST_NAME_MAX> hostNameCStr{};
    if (gethostname(hostNameCStr.data(), hostNameCStr.size()) == 0)
    {
        hostName = hostNameCStr.data();
    }
    return hostName;
}
