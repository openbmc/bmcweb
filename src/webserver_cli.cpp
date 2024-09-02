#include "boost_formatters.hpp"
#include "logging.hpp"

#include <CLI/App.hpp>
#include <boost/asio/io_context.hpp>
#include <sdbusplus/asio/connection.hpp>

#include <memory>
#include <string>

// Override default log option:
static void cliLogLevel(const std::string& logLevel)
{
    crow::getBmcwebCurrentLoggingLevel() = crow::getLogLevelFromName(logLevel);
}

int main(int argc, char** argv) noexcept(false)
{
    CLI::App app("BMCWeb SetLogLevel CLI");

    cliLogLevel("INFO");

    // Define sdbus interfaces:
    std::string service = "xyz.openbmc_project.bmcweb";
    std::string path = "/xyz/openbmc_project/bmcweb";
    std::string iface = "xyz.openbmc_project.bmcweb";
    std::string method = "SetLogLevel";

    std::string loglevel;
    app.add_option("-l,--loglevel", loglevel, "Set bmcweb log level");

    CLI11_PARSE(app, argc, argv)

    BMCWEB_LOG_INFO("Working on log-level: {}", loglevel);

    // Set up dbus connection:
    boost::asio::io_context io;
    auto conn = std::make_shared<sdbusplus::asio::connection>(io);

    // Attempt to async_call to set logging level
    conn->async_method_call(
        [&io](boost::system::error_code& ec) mutable {
            if (ec)
            {
                BMCWEB_LOG_ERROR("SetLogLevel returned error with {}", ec);
                return;
            }
            BMCWEB_LOG_INFO("Successfully changed log-level ");
            io.stop();
        },
        service, path, iface, method, loglevel);

    io.run();

    return 0;
}
