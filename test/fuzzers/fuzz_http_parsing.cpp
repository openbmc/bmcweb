// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026, Oracle and/or its affiliates. All rights reserved.
// 
// LibFuzzer harness for HTTP parsing helpers in http/parsing.hpp.

#include "http/parsing.hpp"

#include <boost/beast/http/field.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <system_error>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    try
    {
        std::string input(reinterpret_cast<const char*>(data), size);

        /*
        Turn one fuzz input buffer into two logical strings: a `contentType` and a `body`.
        First look for a separator byte—preferably a NUL (`'\0'`), and if none exists, a
        newline (`'\n'`). If neither separator is found, use the entire input for both values.
        Otherwise, everything before the separator becomes `contentType`, and everything after it
        becomes `body`.

        This lets the fuzzer exercise multiple parsing paths at once with a single byte stream: the
        same input can simulate an HTTP `Content-Type` header plus a request body. Using `\0` first
        is helpful because fuzz inputs are arbitrary binary data, and `std::string` can contain
        embedded NUL bytes.
        */

        size_t split = input.find('\0');
        if (split == std::string::npos)
        {
            split = input.find('\n');
        }

        std::string contentType;
        std::string body;
        if (split == std::string::npos)
        {
            contentType = input;
            body = input;
        }
        else
        {
            contentType = input.substr(0, split);
            body = input.substr(split + 1);
        }

        (void)isJsonContentType(contentType);
        (void)parseStringAsJson(body);

        std::error_code ec;
        crow::Request req(body, ec);
        req.addHeader(boost::beast::http::field::content_type, contentType);

        nlohmann::json jsonOut;
        (void)parseRequestAsJson(req, jsonOut);
    }
    catch (...)
    {
        // Keep fuzzing on unexpected exceptions.
    }
    return 0;
}