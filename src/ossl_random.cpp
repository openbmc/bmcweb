#include "ossl_random.hpp"

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

std::string bmcweb::getRandomUUID()
{
    using bmcweb::OpenSSLGenerator;
    OpenSSLGenerator ossl;
    return boost::uuids::to_string(
        boost::uuids::basic_random_generator<OpenSSLGenerator>(ossl)());
}
