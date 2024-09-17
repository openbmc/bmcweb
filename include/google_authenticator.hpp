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

struct GoogleAuthenticator :
    public std::enable_shared_from_this<GoogleAuthenticator>
{
    using Handler =
        std::function<void(const boost::system::error_code& ec, bool)>;

    Handler continuation;

    std::string userName;

    void tryMfa(const std::string& user, Handler handler)
    {
        userName = user;
        continuation = std::move(handler);
        checkMfa();
    }
    void checkBypassCallback(const boost::system::error_code& ec,
                             const std::string& bypass)
    {
        if (ec)
        {
            BMCWEB_LOG_ERROR("Failed to get BypassMFA {}", ec.message());
            continuation(ec, false);
            return;
        }
        if (bypass ==
            "xyz.openbmc_project.User.MultiFactorAuthConfiguration.Type.GoogleAuthenticator")
        {
            BMCWEB_LOG_DEBUG("BypassMFA is enabled");
            continuation(ec, false);
            return;
        }

        checkAuthentication();
    }
    void checkBypass()
    {
        std::string objectPathStr =
            std::format("/xyz/openbmc_project/user/{}", userName);
        sdbusplus::asio::getProperty<std::string>(
            *crow::connections::systemBus, "xyz.openbmc_project.User.Manager",
            objectPathStr.data(), "xyz.openbmc_project.User.TOTPAuthenticator",
            "BypassedProtocol",
            std::bind_front(&GoogleAuthenticator::checkBypassCallback,
                            this->shared_from_this()));
    }
    void checkAuthenticationCallback(const boost::system::error_code& ec,
                                     bool present)
    {
        if (ec)
        {
            BMCWEB_LOG_ERROR(
                "Failed to get Google Authenticator SecretKeyIsValidProperty {}",
                ec.message());
            continuation(ec, false);
            return;
        }

        if (!present)
        {
            BMCWEB_LOG_DEBUG("Google Authenticator secret key is not setup");
            continuation(ec, true);
            return;
        }
        continuation(ec, false);
    }
    void checkAuthentication()
    {
        std::string objectPathStr =
            std::format("/xyz/openbmc_project/user/{}", userName);
        sdbusplus::asio::getProperty<bool>(
            *crow::connections::systemBus, "xyz.openbmc_project.User.Manager",
            objectPathStr.data(), "xyz.openbmc_project.User.TOTPAuthenticator",
            "SecretKeyIsValid",
            std::bind_front(&GoogleAuthenticator::checkAuthenticationCallback,
                            this->shared_from_this()));
    }
    void checkMfaCallback(const boost::system::error_code& ec,
                          const std::string& authTypes)
    {
        if (ec)
        {
            BMCWEB_LOG_ERROR("Failed to get Google Authenticator status {}",
                             ec.message());
            continuation(ec, false);
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
            continuation(ec, false);
            return;
        }
        BMCWEB_LOG_DEBUG(
            "Google Authenticator is enabled check for secret key setup");
        checkBypass();
    }
    void checkMfa()
    {
        sdbusplus::asio::getProperty<std::string>(
            *crow::connections::systemBus, "xyz.openbmc_project.User.Manager",
            "/xyz/openbmc_project/user",
            "xyz.openbmc_project.User.MultiFactorAuthConfiguration", "Enabled",
            std::bind_front(&GoogleAuthenticator::checkMfaCallback,
                            this->shared_from_this()));
    }
};
} // namespace bmcweb
