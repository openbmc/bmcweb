#include <http_client.hpp>

#include <optional>

namespace crow
{
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::optional<HttpClient> HttpClient::httpClientSingleton;
} // namespace crow
