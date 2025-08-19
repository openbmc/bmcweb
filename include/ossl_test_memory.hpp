#include <gtest/gtest.h>

extern "C"
{
#include <openssl/crypto.h>
}

static void* ossl_malloc(size_t num, const char* /*file*/, int /*line*/)
{
    // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
    return ::malloc(num);
}
static void* ossl_realloc(void* ptr, size_t num, const char* /*file*/,
                     int /*line*/)
{
    // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
    return ::realloc(ptr, num);
}
static void ossl_free(void* ptr, const char* /*file*/, int /*line*/)
{
    // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
    ::free(ptr);
}

class OpenSSLTestMemory {
    public:
     ~OpenSSLTestMemory() {
        if (CRYPTO_set_mem_functions(ossl_malloc, ossl_realloc, ossl_free) != 1)
        {
            BMCWEB_LOG_ERROR("Failed to set OpenSSL memory functions");
        }
     }
   };