#include <http_client.hpp>

#include <optional>

namespace crow
{
std::optional<HttpClient> HttpClient::httpClientSingleton;
}