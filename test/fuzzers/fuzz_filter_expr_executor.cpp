// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026, Oracle and/or its affiliates. All rights reserved.
//
// LibFuzzer harness for exercising Redfish filter expression parsing +
// execution against a small synthetic JSON collection.

#include "filter_expr_executor.hpp"
#include "filter_expr_printer.hpp"

#include <cstddef>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>

namespace
{
nlohmann::json makeBaseCollection()
{
    return {
        {"Members",
         nlohmann::json::array({
             nlohmann::json::object(
                 {{"Count", 2},
                  {"SerialNumber", "1234"},
                  {"Created", "2021-11-30T22:41:35.123+00:00"}}),
             nlohmann::json::object(
                 {{"Count", 2.0},
                  {"Version", "20.0.2"},
                  {"Created", "2021-11-30T22:41:35.124+00:00"}}),
             nlohmann::json::object(
                 {{"Oem",
                   {{"OEM",
                     {{"@odata.type", "#OEMLogEntry.v1_1_0.OEMLogEntry"},
                      {"Key", "Switch_1"},
                      {"ErrorId", "SWITCH_EC_STRAP_MISMATCH"}}}}}}),
         })}};
}
} // namespace

// LibFuzzer entry point for fuzzing filter expression execution:
// - Parses input bytes as a filter expression (string)
// - If parsing succeeds, applies the filter to a small synthetic collection
//   to exercise comparison and visitor logic paths.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    try
    {
        // Treat fuzzer input as UTF-8-ish filter expression. Even invalid UTF-8
        // is fine; parseFilter will fail gracefully and we bail out.
        std::string expr(reinterpret_cast<const char*>(data), size);

        nlohmann::json base = makeBaseCollection();

        std::optional<redfish::filter_ast::LogicalAnd> ast =
            redfish::parseFilter(std::string_view(expr));
        if (ast)
        {
            // applyFilterToCollection mutates its input; work on a copy so that
            // repeated executions on the same seed do not shrink the base.
            nlohmann::json body = base;
            (void)redfish::applyFilterToCollection(body, *ast);
        }
    }
    catch (...)
    {
        // Ignore any exceptions during fuzzing to continue exploration
    }
    return 0;
}