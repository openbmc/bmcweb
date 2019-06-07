#pragma once

#include <crow/app.h>
#include <crow/http_request.h>
#include <crow/http_response.h>

#include <boost/algorithm/string/predicate.hpp>

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

    Middleware() : loggingEnabled(false)
    {
        BMCWEB_LOG_DEBUG << "Logging middleware c'tor";
        auto method = crow::connections::systemBus->new_method_call(
            "xyz.openbmc_project.Settings",
            "/xyz/openbmc_project/logging/rest_api_logs",
            "org.freedesktop.DBus.Properties", "Get");

        method.append("xyz.openbmc_project.Object.Enable", "Enabled");

        auto reply = crow::connections::systemBus->call(method);

        std::variant<bool> enabled;
        reply.read(enabled);

        bool* enabledPtr = std::get_if<bool>(&enabled);

        if (!enabledPtr)
        {
            BMCWEB_LOG_ERROR << "Failed to read logging enabled setting";
        }
        else
        {
            loggingEnabled = *enabledPtr;
            BMCWEB_LOG_DEBUG << "REST/Redfish logging enabled status: "
                             << loggingEnabled;
        }

        // Register for property changed event on logging enabled DBus object
        busMatcher = std::make_unique<sdbusplus::bus::match_t>(
            *crow::connections::systemBus,
            dbusRule::propertiesChanged(
                "/xyz/openbmc_project/logging/rest_api_logs",
                "xyz.openbmc_project.Object.Enable"),
            [this](sdbusplus::message::message& msg) {
                std::string interface;
                std::map<std::string, std::variant<bool>> props;

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

    void beforeHandle(crow::Request& /*req*/, Response& /*res*/,
                      Context& /*ctx*/)
    {
    }

    void afterHandle(Request& req, Response& res, Context& /*ctx*/)
    {
        if (!loggingEnabled)
        {
            return;
        }
        // Log all traffic for methods other than GET on
        // Redfish and REST routes
        if (req.method() != "GET"_method &&
            (boost::algorithm::starts_with(req.url, "/redfish/v1") ||
             boost::algorithm::starts_with(req.url, "/xyz/") ||
             boost::algorithm::starts_with(req.url, "/org/") ||
             req.url == "/login"))
        {
            bool skipBody = false;

            // Determine whether the body should be logged. Do not log body for
            // requests that contain passwords (login, redfish sessions and
            // user account configurations)
            if ((req.url == "/login") ||
                (req.url ==
                 "/xyz/openbmc_project/user/ldap/action/CreateConfig") ||
                (req.url == "/redfish/v1/SessionService/Sessions") ||
                (req.url == "/redfish/v1/SessionService/Sessions/") ||
                (boost::algorithm::starts_with(req.url,
                                               "/redfish/v1/AccountService/")))
            {
                skipBody = true;
            }

            std::cout << req.remoteIp << " "
                      << "user:"
                      << (req.username.empty() ? res.username : req.username)
                      << " " << req.methodString() << " " << req.url
                      << " json:" << (skipBody ? "None" : req.body) << " "
                      << res.resultInt() << " "
                      << boost::beast::http::obsolete_reason(res.result())
                      << std::endl;
        }
    }

  private:
    bool loggingEnabled;
    std::unique_ptr<sdbusplus::bus::match_t> busMatcher;
};

} // namespace logging
} // namespace crow
