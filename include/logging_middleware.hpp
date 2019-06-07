#pragma once

#include <crow/app.h>
#include <crow/http_request.h>
#include <crow/http_response.h>

#include <../redfish-core/include/utils/systemd_utils.hpp>
#include <boost/algorithm/string/predicate.hpp>
#define SD_JOURNAL_SUPPRESS_LOCATION
#include <systemd/sd-journal.h>

namespace crow
{

namespace logging
{

namespace dbusRule = sdbusplus::bus::match::rules;

class Middleware
{
  public:
    struct Context
    {
    };

    Middleware()
    {
        // Register for property changed event on logging enabled DBus object
        busMatcher = std::make_unique<sdbusplus::bus::match_t>(
            *crow::connections::systemBus,
            dbusRule::propertiesChanged(
                "/xyz/openbmc_project/logging/rest_api_logs",
                "xyz.openbmc_project.Object.Enable"),
            [this](sdbusplus::message::message& msg) {
                std::string interface;
                std::vector<std::pair<std::string, std::variant<bool>>> props;

                msg.read(interface, props);

                for (auto& prop : props)
                {
                    if (prop.first == "Enabled")
                    {
                        bool* enabled = std::get_if<bool>(&prop.second);
                        if (enabled)
                        {
                            BMCWEB_LOG_DEBUG << "Logging enabled changed to: "
                                             << *enabled;
                            this->loggingEnabled = *enabled;
                        }
                        break;
                    }
                }
            });
    }

    ~Middleware() = default;

    void log(const crow::Request& req, crow::Response& res,
             const std::string_view& username, bool skipBody)
    {
        if (loggingEnabled && !*loggingEnabled)
        {
            return;
        }
        sd_journal_print(
            LOG_INFO, "%s: user: %s %s %s json: %s %d %s",
            redfish::systemd_utils::getUuid().c_str(),
            std::string(username).c_str(),
            std::string(req.methodString()).c_str(),
            std::string(req.url).c_str(),
            (skipBody ? "None" : req.body.c_str()), res.resultInt(),
            std::string(boost::beast::http::obsolete_reason(res.result()))
                .c_str());
    }

    void beforeHandle(crow::Request& /*req*/, Response& /*res*/,
                      Context& /*ctx*/)
    {
        if (!loggingEnabled)
        {
            crow::connections::systemBus->async_method_call(
                [this](const boost::system::error_code ec,
                       std::variant<bool> enabled) {
                    if (ec)
                    {
                        BMCWEB_LOG_DEBUG << "Failed call to get logging status "
                                            "DBUS response error "
                                         << ec;
                        return;
                    }
                    bool* enabledPtr = std::get_if<bool>(&enabled);
                    if (enabledPtr)
                    {
                        BMCWEB_LOG_DEBUG << "Logging enabled set to: "
                                         << *enabledPtr;
                        this->loggingEnabled = *enabledPtr;
                    }
                },
                "xyz.openbmc_project.Settings",
                "/xyz/openbmc_project/logging/rest_api_logs",
                "org.freedesktop.DBus.Properties", "Get",
                "xyz.openbmc_project.Object.Enable", "Enabled");
        }
    }

    void afterHandle(Request& req, Response& res, Context& /*ctx*/)
    {
        if (loggingEnabled && !*loggingEnabled)
        {
            return;
        }
        // Log all traffic for methods other than GET on
        // Redfish and REST routes
        if (req.method() != "GET"_method &&
            (boost::algorithm::starts_with(req.url, "/redfish/v1") ||
             boost::algorithm::starts_with(req.url, "/xyz/") ||
             boost::algorithm::starts_with(req.url, "/org/")))
        {
            bool skipBody = false;

            // Determine whether the body should be logged. Do not log body for
            // requests that contain passwords.
            if ((req.url ==
                 "/xyz/openbmc_project/user/ldap/action/CreateConfig") ||
                (boost::algorithm::starts_with(
                    req.url, "/redfish/v1/AccountService/")) ||
                (req.method() == "POST"_method &&
                 ((req.url == "/redfish/v1/UpdateService") ||
                  (req.url == "/redfish/v1/UpdateService/"))))
            {
                skipBody = true;
            }
            if (req.session)
            {
                log(req, res, req.session->username, skipBody);
            }
        }
    }

  private:
    std::optional<bool> loggingEnabled;
    std::unique_ptr<sdbusplus::bus::match_t> busMatcher;
};

} // namespace logging
} // namespace crow
