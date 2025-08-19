// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#pragma once

#include <gtest/gtest.h>

extern "C"
{
#include <openssl/crypto.h>
}

static void* osslMalloc(size_t num, const char* /*file*/, int /*line*/)
{
    // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
    return ::malloc(num);
}
static void* osslRealloc(void* ptr, size_t num, const char* /*file*/,
                         int /*line*/)
{
    // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
    return ::realloc(ptr, num);
}
static void osslFree(void* ptr, const char* /*file*/, int /*line*/)
{
    // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
    ::free(ptr);
}

class OpenSSLTestMemory
{
  public:
    OpenSSLTestMemory() noexcept
    {
        if (CRYPTO_set_mem_functions(osslMalloc, osslRealloc, osslFree) != 1)
        {
            BMCWEB_LOG_ERROR("Failed to set OpenSSL memory functions");
        }
    }
};
