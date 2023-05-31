#include "http_response.hpp"
#include <string>
#include <nlohmann/json.hpp>

namespace json_html_util
{

void dumpHtml(std::string& out, const nlohmann::json& json);
void prettyPrintJson(crow::Response& res);

} // namespace json_html_util
