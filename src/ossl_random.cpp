#include "ossl_random.hpp"

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <string>

std::string bmcweb::getRandomUUID()
{
    using bmcweb::OpenSSLGenerator;
    OpenSSLGenerator ossl;
    return boost::uuids::to_string(
        boost::uuids::basic_random_generator<OpenSSLGenerator>(ossl)());
}
