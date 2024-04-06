

#include "logging.hpp"
#include "webserver_run.hpp"

#include <exception>
#include <memory>
#include <string>

int main(int /*argc*/, char** /*argv*/)
{
    try
    {
        return run();
    }
    catch (const std::exception& e)
    {
        BMCWEB_LOG_CRITICAL("Threw exception to main: {}", e.what());
        return -1;
    }
    catch (...)
    {
        BMCWEB_LOG_CRITICAL("Threw exception to main");
        return -1;
    }
}
