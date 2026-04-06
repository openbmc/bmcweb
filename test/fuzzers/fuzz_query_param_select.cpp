// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026, Oracle and/or its affiliates. All rights reserved.
//
// LibFuzzer harness for Redfish $select parsing and application helpers.

#include "async_resp.hpp"
#include "utils/query_param.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

namespace
{

using nlohmann::json;

// Build a compact but nested Redfish-like document so the fuzzer can still
// exercise recursive selection logic even when the input does not contain
// valid JSON.
json makeDefaultResponse()
{
    return {
        {"@odata.id", "/redfish/v1/Systems"},
        {"Members", json::array({
            json::object({
                {"@odata.id", "/redfish/v1/Systems/1"},
                {"Name", "sys1"},
                {"Links",
                 {{"Chassis",
                   json::array({
                       json::object(
                           {{"@odata.id", "/redfish/v1/Chassis/1"}}),
                       json::object(
                           {{"@odata.id", "/redfish/v1/Chassis/2"}}),
                   })}}},
                {"Oem",
                 {{"Vendor",
                   {{"Key", "Value"}, {"Deep", {{"Inner", 42}}}}}}},
            }),
            json::object({
                {"@odata.id", "/redfish/v1/Systems/2"},
                {"Name", "sys2"},
                {"Links",
                 {{"ManagedBy",
                   json::array({json::object(
                       {{"@odata.id", "/redfish/v1/Managers/bmc"}})})}}},
                {"Oem", {{"Vendor", {{"AnotherKey", true}}}}},
            }),
        })},
    };
}

// Split one fuzz buffer into a candidate $select expression and an optional
// JSON payload. Prefer NUL as a separator because fuzz inputs are arbitrary
// bytes; fall back to newline for easier corpus authoring.
std::pair<std::string, std::string> splitInput(std::string&& input)
{
    size_t split = input.find('\0');
    if (split == std::string::npos)
    {
        split = input.find('\n');
    }

    if (split == std::string::npos)
    {
        return {std::move(input), {}};
    }

    std::string selectPart = input.substr(0, split);
    std::string jsonPart = input.substr(split + 1);
    return {std::move(selectPart), std::move(jsonPart)};
}

// Parse the JSON half of the input without exceptions. If parsing fails or the
// JSON portion is empty, fall back to a deterministic default document.
json parseOrDefaultJson(const std::string& jsonPart)
{
    if (jsonPart.empty())
    {
        return makeDefaultResponse();
    }

    json parsed = json::parse(jsonPart.begin(), jsonPart.end(), nullptr,
                              /*allow_exceptions=*/false);
    if (parsed.is_discarded())
    {
        return makeDefaultResponse();
    }
    return parsed;
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    try
    {
        // Drive both halves of the $select pipeline:
        //   1. parse a user-controlled $select string into a trie
        //   2. apply that trie to a user-controlled or fallback JSON payload
        std::string input(reinterpret_cast<const char*>(data), size);
        auto [selectPart, jsonPart] = splitInput(std::move(input));

        bmcweb::AsyncResp resp;
        resp.res.jsonValue = parseOrDefaultJson(jsonPart);

        redfish::query_param::Query query;
        (void)redfish::query_param::getSelectParam(selectPart, query);

        // Intentionally apply the same `$select` trie twice and ensure repeated
        // traversal over already-pruned JSON does not crash or misbehave. In other
        // words, the first call mutates the response, and the second call checks
        // that reprocessing the mutated structure is still safe.
        redfish::query_param::processSelect(resp.res, query.selectTrie.root);
        redfish::query_param::processSelect(resp.res, query.selectTrie.root);
    }
    catch (...)
    {
        // Do not propagate exceptions from the fuzzer entry point.
    }
    return 0;
}
