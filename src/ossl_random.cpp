#include "ossl_random.hpp"

extern "C"
{
#include <openssl/rand.h>
}

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <array>
#include <random>
#include <string>

namespace bmcweb
{
uint8_t OpenSSLGenerator::operator()()
{
    uint8_t index = 0;
    int rc = RAND_bytes(&index, sizeof(index));
    if (rc != opensslSuccess)
    {
        BMCWEB_LOG_ERROR("Cannot get random number");
        err = true;
    }

    return index;
}

std::string getRandomUUID()
{
    using bmcweb::OpenSSLGenerator;
    OpenSSLGenerator ossl;
    return boost::uuids::to_string(
        boost::uuids::basic_random_generator<OpenSSLGenerator>(ossl)());
}

std::string getRandomIdOfLength(size_t length)
{
    static constexpr std::array<char, 62> alphanum = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C',
        'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
        'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c',
        'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
        'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};

    std::string token;
    token.resize(length, '0');
    std::uniform_int_distribution<size_t> dist(0, alphanum.size() - 1);

    bmcweb::OpenSSLGenerator gen;

    for (char& tokenChar : token)
    {
        tokenChar = alphanum[dist(gen)];
        if (gen.error())
        {
            return "";
        }
    }
    return token;
}
} // namespace bmcweb
