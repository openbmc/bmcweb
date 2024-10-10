#include "boost_formatters.hpp"
#include "logging.hpp"

#include <CLI/CLI.hpp>
#include <boost/asio/io_context.hpp>
#include <sdbusplus/asio/connection.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <memory>
#include <string>

// Override default log option:
static void cliLogLevel(const std::string& logLevel)
{
    bmcweb::getBmcwebCurrentLoggingLevel() =
        bmcweb::getLogLevelFromName(logLevel);
}

static constexpr std::array<std::string, 7> levels{
    "DISABLED", "CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG", "ENABLED"};

// Check if debug level is valid
static std::string validateLogLevel(std::string& input)
{
    std::transform(input.begin(), input.end(), input.begin(), ::toupper);
    const std::string* iter = std::ranges::find(levels, input);
    if (iter == levels.end())
    {
        return {"Invalid log level"};
    }
    return {};
}

static std::string helpMsg()
{
    std::string help = "\nLog levels to choose from:\n";
    for (const std::string& prompt : levels)
    {
        std::string level = prompt;
        std::transform(level.begin(), level.end(), level.begin(), ::tolower);
        help.append(level + "\n");
    }
    return help;
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
    app.require_subcommand(1);

    const CLI::Validator levelValidator =
        CLI::Validator(validateLogLevel, "valid level");

    CLI::App* sub = app.add_subcommand("loglevel", "Set bmcweb log level");
    sub->add_option("level", loglevel, helpMsg())
        ->required()
        ->check(levelValidator);

    CLI11_PARSE(app, argc, argv)

    std::transform(loglevel.begin(), loglevel.end(), loglevel.begin(),
                   ::toupper);
    // Set up dbus connection:
    boost::asio::io_context io;
    auto conn = std::make_shared<sdbusplus::asio::connection>(io);

    // Attempt to async_call to set logging level
    conn->async_method_call(
        [&io, &loglevel](boost::system::error_code& ec) mutable {
            if (ec)
            {
                BMCWEB_LOG_ERROR("SetLogLevel returned error with {}", ec);
                return;
            }
            BMCWEB_LOG_INFO("logging level changed to: {}", loglevel);
            io.stop();
        },
        service, path, iface, method, loglevel);

    io.run();

    return 0;
}
