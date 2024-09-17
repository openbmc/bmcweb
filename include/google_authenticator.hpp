#pragma once

#include "registries/privilege_registry.hpp"

#include <functional>
#include <string>
namespace bmcweb
{

struct GoogleAuthenticator
{
    static constexpr std::string_view service =
        "xyz.openbmc_project.User.Manager";
    static constexpr std::string_view TOTPAuthenticatorIface =
        "xyz.openbmc_project.User.TOTPAuthenticator";
    static constexpr std::string_view objectUserPath =
        "/xyz/openbmc_project/user/{}";
    static constexpr std::string_view SecretKeyIsValidProperty =
        "SecretKeyIsValid";

    static constexpr std::string_view MFABypassProperty = "BypassedProtocol";

    static constexpr std::string_view MultiFactorAuthConfigurationIface =
        "xyz.openbmc_project.User.MultiFactorAuthConfiguration";
    static constexpr std::string_view objectManagerPath =
        "/xyz/openbmc_project/user";
    static constexpr std::string_view multiFactorProperty = "Enabled";

    static constexpr std::string_view googleAuthKey =
        "xyz.openbmc_project.User.MultiFactorAuthConfiguration.Type.GoogleAuthenticator";

    std::function<void(bool)> onSuccessHandler;
    std::function<void()> onErrorHandler;
    std::string userName;
    std::optional<std::string> token;
    GoogleAuthenticator& withUser(const std::string& user)
    {
        userName = user;
        return *this;
    }
    GoogleAuthenticator& withToken(const std::optional<std::string>& tok)
    {
        token = tok;
        return *this;
    }
    GoogleAuthenticator& tryAuthentication(std::function<void(bool)> handler)
    {
        onSuccessHandler = std::move(handler);
        return *this;
    }
    GoogleAuthenticator& orElse(std::function<void()> handler)
    {
        onErrorHandler = std::move(handler);
        return *this;
    }
    void checkBypass(std::shared_ptr<bmcweb::AsyncResp> asyncResp)
    {
        std::string objectPathStr = std::format(objectUserPath, userName);
        sdbusplus::asio::getProperty<std::string>(
            *crow::connections::systemBus, service.data(), objectPathStr.data(),
            TOTPAuthenticatorIface.data(), MFABypassProperty.data(),
            [asyncResp,
             thisp = std::move(*this)](const boost::system::error_code& ec,
                                       const std::string& bypass) mutable {
                if (ec)
                {
                    BMCWEB_LOG_ERROR("Failed to get BypassMFA {}",
                                     ec.message());
                    thisp.onErrorHandler();
                    return;
                }
                if (bypass == googleAuthKey.data())
                {
                    BMCWEB_LOG_DEBUG("BypassMFA is enabled");
                    thisp.onSuccessHandler(false);
                    return;
                }
                BMCWEB_LOG_DEBUG("BypassMFA is disabled");
                thisp.checkAuthentication(asyncResp);
            });
    }
    void checkAuthentication(std::shared_ptr<bmcweb::AsyncResp> asyncResp)
    {
        std::string objectPathStr = std::format(objectUserPath, userName);
        sdbusplus::asio::getProperty<bool>(
            *crow::connections::systemBus, service.data(), objectPathStr.data(),
            TOTPAuthenticatorIface.data(), SecretKeyIsValidProperty.data(),
            [asyncResp, thisp = std::move(*this)](
                const boost::system::error_code& ec, bool present) mutable {
                if (ec)
                {
                    BMCWEB_LOG_ERROR(
                        "Failed to get Google Authenticator SecretKeyIsValidProperty {}",
                        ec.message());
                    thisp.onErrorHandler();
                    return;
                }

                if (!present)
                {
                    BMCWEB_LOG_DEBUG(
                        "Google Authenticator secret key is not setup");
                    thisp.onSuccessHandler(true);
                    return;
                }
                thisp.onSuccessHandler(false);
            });
    }
    void checkMfa(std::shared_ptr<bmcweb::AsyncResp> asyncResp)
    {
        sdbusplus::asio::getProperty<std::string>(
            *crow::connections::systemBus, service.data(),
            objectManagerPath.data(), MultiFactorAuthConfigurationIface.data(),
            multiFactorProperty.data(),
            [asyncResp,
             thisp = std::move(*this)](const boost::system::error_code& ec,
                                       const std::string& authTypes) mutable {
                if (ec)
                {
                    BMCWEB_LOG_ERROR(
                        "Failed to get Google Authenticator status {}",
                        ec.message());
                    thisp.onErrorHandler();
                    return;
                }
                bool enabled = authTypes.find(googleAuthKey.data()) !=
                               std::string::npos;

                if (!enabled)
                {
                    BMCWEB_LOG_DEBUG(
                        "Google Authenticator is not enabled proceed with session creation");
                    thisp.onSuccessHandler(false);
                    return;
                }
                BMCWEB_LOG_DEBUG(
                    "Google Authenticator is enabled check for secret key setup");
                thisp.checkBypass(asyncResp);
            });
    }
    void run(std::shared_ptr<bmcweb::AsyncResp> asyncResp)
    {
        checkMfa(asyncResp);
    }
};
} // namespace bmcweb
