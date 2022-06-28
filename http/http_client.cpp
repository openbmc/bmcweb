#include <http_client.hpp>

#include <optional>

namespace crow
{
// NOLINTNEXTLINE
std::optional<HttpClient> HttpClient::httpClientSingleton;
} // namespace crow
