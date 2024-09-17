#pragma once

#include "registries/privilege_registry.hpp"

#include <functional>
#include <string>
namespace bmcweb
{

struct GoogleAuthenticator
{
    using Handler =
        std::function<void(const boost::system::error_code& ec, bool)>;

    Handler continuation;

    std::string userName;

    void tryAuthentication(const std::string& user, Handler handler)
    {
        userName = user;
        continuation = std::move(handler);
        checkMfa();
    }

    void checkBypass()
    {
        std::string objectPathStr =
            std::format("/xyz/openbmc_project/user/{}", userName);
        sdbusplus::asio::getProperty<std::string>(
            *crow::connections::systemBus, "xyz.openbmc_project.User.Manager",
            objectPathStr.data(), "xyz.openbmc_project.User.TOTPAuthenticator",
            "BypassedProtocol",
            [thisp = std::move(*this)](const boost::system::error_code& ec,
                                       const std::string& bypass) mutable {
                if (ec)
                {
                    BMCWEB_LOG_ERROR("Failed to get BypassMFA {}",
                                     ec.message());
                    thisp.continuation(ec, false);
                    return;
                }
                if (bypass ==
                    "xyz.openbmc_project.User.MultiFactorAuthConfiguration.Type.GoogleAuthenticator")
                {
                    BMCWEB_LOG_DEBUG("BypassMFA is enabled");
                    thisp.continuation(ec, false);
                    return;
                }

                thisp.checkAuthentication();
            });
    }
    void checkAuthentication()
    {
        static constexpr std::string_view SecretKeyIsValidProperty =
            "SecretKeyIsValid";
        std::string objectPathStr =
            std::format("/xyz/openbmc_project/user/{}", userName);
        sdbusplus::asio::getProperty<bool>(
            *crow::connections::systemBus, "xyz.openbmc_project.User.Manager",
            objectPathStr.data(), "xyz.openbmc_project.User.TOTPAuthenticator",
            SecretKeyIsValidProperty.data(),
            [thisp = std::move(*this)](const boost::system::error_code& ec,
                                       bool present) mutable {
                if (ec)
                {
                    BMCWEB_LOG_ERROR(
                        "Failed to get Google Authenticator SecretKeyIsValidProperty {}",
                        ec.message());
                    thisp.continuation(ec, false);
                    return;
                }

                if (!present)
                {
                    BMCWEB_LOG_DEBUG(
                        "Google Authenticator secret key is not setup");
                    thisp.continuation(ec, true);
                    return;
                }
                thisp.continuation(ec, false);
            });
    }
    void checkMfa()
    {
        sdbusplus::asio::getProperty<std::string>(
            *crow::connections::systemBus, "xyz.openbmc_project.User.Manager",
            "/xyz/openbmc_project/user",
            "xyz.openbmc_project.User.MultiFactorAuthConfiguration", "Enabled",
            [thisp = std::move(*this)](const boost::system::error_code& ec,
                                       const std::string& authTypes) mutable {
                if (ec)
                {
                    BMCWEB_LOG_ERROR(
                        "Failed to get Google Authenticator status {}",
                        ec.message());
                    thisp.continuation(ec, false);
                    return;
                }
                bool enabled =
                    authTypes.find(
                        "xyz.openbmc_project.User.MultiFactorAuthConfiguration.Type.GoogleAuthenticator") !=
                    std::string::npos;

                if (!enabled)
                {
                    BMCWEB_LOG_DEBUG(
                        "Google Authenticator is not enabled proceed with session creation");
                    thisp.continuation(ec, false);
                    return;
                }
                BMCWEB_LOG_DEBUG(
                    "Google Authenticator is enabled check for secret key setup");
                thisp.checkBypass();
            });
    }
};
} // namespace bmcweb
