#pragma once

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/address_v6.hpp>

#include <string>

namespace redfish
{
namespace ip_util
{

/**
 * @brief Converts boost::asio::ip::address to string
 * Will automatically convert IPv4-mapped IPv6 address back to IPv4.
 *
 * @param[in] ipAddr IP address to convert
 *
 * @return IP address string
 */
inline std::string toString(const boost::asio::ip::address& ipAddr)
{
    if (ipAddr.is_v6() && ipAddr.to_v6().is_v4_mapped())
    {
        return boost::asio::ip::make_address_v4(boost::asio::ip::v4_mapped,
                                                ipAddr.to_v6())
            .to_string();
    }
    return ipAddr.to_string();
}

/**
 * @brief Helper function that verifies IP address to check if it is in
 *        proper format. If bits pointer is provided, also calculates active
 *        bit count for Subnet Mask.
 *
 * @param[in]  ip     IP that will be verified
 * @param[out] bits   Calculated mask in bits notation
 *
 * @return true in case of success, false otherwise
 */
inline bool ipv4VerifyIpAndGetBitcount(const std::string& ip,
                                       uint8_t* bits = nullptr)
{
    std::vector<std::string> bytesInMask;

    boost::split(bytesInMask, ip, boost::is_any_of("."));

    static const constexpr int ipV4AddressSectionsCount = 4;
    if (bytesInMask.size() != ipV4AddressSectionsCount)
    {
        return false;
    }

    if (bits != nullptr)
    {
        *bits = 0;
    }

    char* endPtr = nullptr;
    long previousValue = 255;
    bool firstZeroInByteHit = false;
    for (const std::string& byte : bytesInMask)
    {
        if (byte.empty())
        {
            return false;
        }

        // Use strtol instead of stroi to avoid exceptions
        long value = std::strtol(byte.c_str(), &endPtr, 10);

        // endPtr should point to the end of the string, otherwise given string
        // is not 100% number
        if (*endPtr != '\0')
        {
            return false;
        }

        // Value should be contained in byte
        if (value < 0 || value > 255)
        {
            return false;
        }

        if (bits != nullptr)
        {
            // Mask has to be continuous between bytes
            if (previousValue != 255 && value != 0)
            {
                return false;
            }

            // Mask has to be continuous inside bytes
            firstZeroInByteHit = false;

            // Count bits
            for (long bitIdx = 7; bitIdx >= 0; bitIdx--)
            {
                if ((value & (1L << bitIdx)) != 0)
                {
                    if (firstZeroInByteHit)
                    {
                        // Continuity not preserved
                        return false;
                    }
                    (*bits)++;
                }
                else
                {
                    firstZeroInByteHit = true;
                }
            }
        }

        previousValue = value;
    }

    return true;
}

} // namespace ip_util
} // namespace redfish
