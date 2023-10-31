#pragma once

#include "logging.hpp"

#include <regex>

// Regex matching the Subject CommonName format used by internal Meta client
// certs
const std::string sslUserRegexStr =
    "^(user|svc|host):"  // userType
    "([A-Za-z0-9._-]*)"  // userName
    "(/([A-Za-z0-9.-]+)" // userHost (optional if userType != "host")
    "[.](facebook[.]com|tfbnw[.]net|thefacebook[.]com)" // host suffix
    ")?$";

inline int mtlsMetaParseSslUser(std::string& sslUser)
{
    // Parses a Meta internal TLS client certificate user name in
    // '<type>:<username or service name>[/<hostname>]' format and writes the
    // resulting POSIX-compatible user name in sslUser (followed by zero-byte so
    // it's similar to X509_get_subject_name() behaviour). Returns 0 on success,
    // -1 otherwise.
    //
    // Examples:
    // "user:a_username/hostname" -> "user_a_username"
    // "svc:an_internal_service_name" -> "svc_an_internal_service_name"
    // "host:/hostname.facebook.com" -> "host_hostname" (note the stripped
    // hostname suffix)
    const static std::regex sslUserRegex(sslUserRegexStr);

    std::smatch sm;
    if (!std::regex_match(sslUser, sm, sslUserRegex))
    {
        BMCWEB_LOG_DEBUG(
            "Invalid Meta TLS user name: '{}', expected it to match: /{}/",
            sslUser, sslUserRegexStr);
        return -1;
    }

    std::string userType = sm[1]; // e.g. "user" or "svc"
    std::string userName = sm[2]; // e.g. "a_username"
    std::string userHost = sm[4]; // e.g. "hostname"

    // Use the hostname as user name if userType == "host"
    // e.g. "host:/hostname.facebook.com" -> "host_hostname"
    if (userType == "host")
        userName = userHost;

    if (userType.empty() || userName.empty())
    {
        BMCWEB_LOG_DEBUG("Invalid Meta TLS user name: '{}'", sslUser);
        return -1;
    }

    sslUser = userType + "_" + userName;

    return 0;
}
