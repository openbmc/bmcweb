// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026, Oracle and/or its affiliates. All rights reserved.
//
// Fuzzer for json_html_util::dumpHtml() string escaping path.
//
// This harness drives escaping via the public dumpHtml() API by placing the
// fuzz input in a JSON string value.

#include "json_html_serializer.hpp"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <cstdint>
#include <string>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Treat input bytes as an arbitrary string. The escaping routine should be
    // robust to any byte values.
    std::string in(reinterpret_cast<const char*>(data), size);

    nlohmann::json payload;
    payload["value"] = in;

    std::string out;
    json_html_util::dumpHtml(out, payload);

    return 0;
}
