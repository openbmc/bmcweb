// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "webserver_cli.hpp"

#include "boost_formatters.hpp"
#include "logging.hpp"
#include "webserver_run.hpp"

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
    crow::getBmcwebCurrentLoggingLevel() = crow::getLogLevelFromName(logLevel);
}

static constexpr std::array<std::string, 7> levels{
    "DISABLED", "CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG", "ENABLED"};

// Check if debug level is valid
static std::string validateLogLevel(std::string& input)
{
    std::ranges::transform(input, input.begin(), ::toupper);
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
        std::ranges::transform(level, level.begin(), ::tolower);
        help.append(level + "\n");
    }
    return help;
}

static int setLogLevel(std::string& loglevel)
{
    // Define sdbus interfaces:
    std::string service = "xyz.openbmc_project.bmcweb";
    std::string path = "/xyz/openbmc_project/bmcweb";
    std::string iface = "xyz.openbmc_project.bmcweb";
    std::string method = "SetLogLevel";

<<<<<<< PATCH SET (4d0541 WIP: Implement modernize-use-ranges)
    std::string loglevel;
    app.require_subcommand(1);

    const CLI::Validator levelValidator =
        CLI::Validator(validateLogLevel, "valid level");

    CLI::App* sub = app.add_subcommand("loglevel", "Set bmcweb log level");
    sub->add_option("level", loglevel, helpMsg())
        ->required()
        ->check(levelValidator);

    CLI11_PARSE(app, argc, argv)

    std::ranges::transform(loglevel, loglevel.begin(), ::toupper);
=======
    std::transform(loglevel.begin(), loglevel.end(), loglevel.begin(),
                   ::toupper);
>>>>>>> BASE      (9ad2c6 Update to boost 1.89)
    // Set up dbus connection:
    boost::asio::io_context io;
    auto conn = std::make_shared<sdbusplus::asio::connection>(io);

    // Attempt to async_call to set logging level
    conn->async_method_call(
        [&io, loglevel](boost::system::error_code& ec) mutable {
            if (ec)
            {
                BMCWEB_LOG_ERROR("SetLogLevel returned error with {}", ec);
            }
            else
            {
                BMCWEB_LOG_INFO("logging level changed to: {}", loglevel);
            }
            io.stop();
        },
        service, path, iface, method, loglevel);

    io.run();

    return 0;
}

int runCLI(int argc, char** argv) noexcept(false)
{
    CLI::App app("BMCWeb CLI");

    const CLI::Validator levelValidator =
        CLI::Validator(validateLogLevel, "valid level");

    CLI::App* loglevelsub =
        app.add_subcommand("loglevel", "Set bmcweb log level");

    std::string loglevel;
    loglevelsub->add_option("level", loglevel, helpMsg())
        ->required()
        ->check(levelValidator);

    CLI::App* daemon = app.add_subcommand("daemon", "Run webserver");

    CLI11_PARSE(app, argc, argv)

    if (loglevelsub->parsed())
    {
        cliLogLevel("INFO");
        return setLogLevel(loglevel);
    }
    if (daemon->parsed())
    {
        return runWebserver();
    }
    runWebserver();

    return 0;
}
