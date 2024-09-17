#pragma once

#include "registries/privilege_registry.hpp"

#include <boost/system/error_code.hpp>

#include <format>
#include <functional>
#include <memory>
#include <string>
#include <utility>
namespace bmcweb
{

struct GoogleAuthenticator
{
    using Handler =
        std::function<void(const boost::system::error_code& ec, bool)>;

    std::string userName;
    Handler continuation;

    GoogleAuthenticator(const std::string& user, Handler handler) :
        userName(user), continuation(std::move(handler))
    {}

    static void checkBypassCallback(GoogleAuthenticator gAuth,
                                    const boost::system::error_code& ec,
                                    const std::string& bypass)
    {
        if (ec)
        {
            BMCWEB_LOG_ERROR("Failed to get BypassMFA {}", ec.message());
            gAuth.continuation(ec, false);
            return;
        }
        if (bypass ==
            "xyz.openbmc_project.User.MultiFactorAuthConfiguration.Type.GoogleAuthenticator")
        {
            BMCWEB_LOG_DEBUG("BypassMFA is enabled");
            gAuth.continuation(ec, false);
            return;
        }

        checkAuthentication(std::move(gAuth));
    }
    static void checkBypass(GoogleAuthenticator gAuth)
    {
        std::string objectPathStr =
            std::format("/xyz/openbmc_project/user/{}", gAuth.userName);
        sdbusplus::asio::getProperty<std::string>(
            *crow::connections::systemBus, "xyz.openbmc_project.User.Manager",
            objectPathStr, "xyz.openbmc_project.User.TOTPAuthenticator",
            "BypassedProtocol",
            std::bind_front(&GoogleAuthenticator::checkBypassCallback,
                            std::move(gAuth)));
    }
    static void checkAuthenticationCallback(GoogleAuthenticator gAuth,
                                            const boost::system::error_code& ec,
                                            bool present)
    {
        if (ec)
        {
            BMCWEB_LOG_ERROR(
                "Failed to get Google Authenticator SecretKeyIsValidProperty {}",
                ec.message());
            gAuth.continuation(ec, false);
            return;
        }

        if (!present)
        {
            BMCWEB_LOG_DEBUG("Google Authenticator secret key is not setup");
            gAuth.continuation(ec, true);
            return;
        }
        gAuth.continuation(ec, false);
    }
    static void checkAuthentication(GoogleAuthenticator gAuth)
    {
        std::string objectPathStr =
            std::format("/xyz/openbmc_project/user/{}", gAuth.userName);
        sdbusplus::asio::getProperty<bool>(
            *crow::connections::systemBus, "xyz.openbmc_project.User.Manager",
            objectPathStr, "xyz.openbmc_project.User.TOTPAuthenticator",
            "SecretKeyIsValid",
            std::bind_front(&GoogleAuthenticator::checkAuthenticationCallback,
                            std::move(gAuth)));
    }
    static void checkMfaCallback(GoogleAuthenticator gAuth,
                                 const boost::system::error_code& ec,
                                 const std::string& authTypes)
    {
        if (ec)
        {
            BMCWEB_LOG_ERROR("Failed to get Google Authenticator status {}",
                             ec.message());
            gAuth.continuation(ec, false);
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
            gAuth.continuation(ec, false);
            return;
        }
        BMCWEB_LOG_DEBUG(
            "Google Authenticator is enabled check for secret key setup");
        checkBypass(std::move(gAuth));
    }
    static void checkMfa(GoogleAuthenticator gAuth)
    {
        sdbusplus::asio::getProperty<std::string>(
            *crow::connections::systemBus, "xyz.openbmc_project.User.Manager",
            "/xyz/openbmc_project/user",
            "xyz.openbmc_project.User.MultiFactorAuthConfiguration", "Enabled",
            std::bind_front(&GoogleAuthenticator::checkMfaCallback,
                            std::move(gAuth)));
    }
};
} // namespace bmcweb
