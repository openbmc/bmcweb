// hostname_utils.hpp
#pragma once
#include <boost/asio/ip/address.hpp>
#include <boost/system/error_code.hpp>

#include <algorithm>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace redfish
{
namespace hostname_utils
{

inline bool isHostnameValid(const std::string& hostname)
{
    // A valid host name can never have the dotted-decimal form (RFC 1123)
    if (std::ranges::all_of(hostname, ::isdigit))
    {
        return false;
    }
    // Each label(hostname/subdomains) within a valid FQDN
    // MUST handle host names of up to 63 characters (RFC 1123)
    // labels cannot start or end with hyphens (RFC 952)
    // labels can start with numbers (RFC 1123)
    const static std::regex pattern(
        "^([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9])$");

    return std::regex_match(hostname, pattern);
}

inline bool isDomainnameValid(const std::string& domainname)
{
    // Can have multiple subdomains
    // Top Level Domain's min length is 2 character
    const static std::regex pattern(
        "^([A-Za-z0-9][a-zA-Z0-9\\-]{1,61}|[a-zA-Z0-9]{1,30}\\.)*[a-zA-Z]{2,}$");

    return std::regex_match(domainname, pattern);
}

inline bool isValidNtpServer(const std::string& ntpServer)
{
    // Validate empty string and maximum length (RFC 1035)
    if (ntpServer.empty() || ntpServer.length() > 255)
    {
        return false;
    }

    // Try to parse as IP address FIRST (IPv4 or IPv6)
    boost::system::error_code ec;
    boost::asio::ip::make_address(ntpServer, ec);
    if (!ec)
    {
        return true; // Valid IP address
    }

    // NOT an IP address, check for invalid characters in hostname/FQDN
    // Note: Colon is NOT in this list to allow IPv6 parsing above
    const std::string invalidChars = " !@#$%^&*()+=[]{}|\\;'\",<>?_";
    if (ntpServer.find_first_of(invalidChars) != std::string::npos)
    {
        return false;
    }

    // Check for invalid leading/trailing dots
    if (ntpServer.front() == '.' || ntpServer.back() == '.')
    {
        return false;
    }

    // Check for consecutive dots
    if (ntpServer.find("..") != std::string::npos)
    {
        return false;
    }

    // Split FQDN by dots and validate each label
    std::vector<std::string> labels;
    std::stringstream ss(ntpServer);
    std::string label;

    while (std::getline(ss, label, '.'))
    {
        if (label.empty() || !isHostnameValid(label))
        {
            return false;
        }
        labels.push_back(label);
    }

    // At least one valid label required
    return !labels.empty();
}

} // namespace hostname_utils
} // namespace redfish
