#pragma once
#include <boost/asio/ssl/verify_context.hpp>

extern "C"
{
#include <nghttp2/nghttp2.h>
#include <openssl/asn1.h>
#include <openssl/bio.h>
#include <openssl/ec.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/obj_mac.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/tls1.h>
#include <openssl/types.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <openssl/x509v3.h>
}

class OpenSSLBIO
{
  public:
    OpenSSLBIO() : ptr(BIO_new(BIO_s_mem())) {}
    OpenSSLBIO(std::string& data) :
        ptr(BIO_new_mem_buf(static_cast<void*>(data.data()),
                            static_cast<int>(data.size())))
    {}
    OpenSSLBIO(const std::filesystem::path& filepath) :
        ptr(BIO_new_file(filepath.c_str(), "rb"))
    {
        if (ptr == nullptr)
        {
            throw std::runtime_error("Failed to open file");
        }
    }

    OpenSSLBIO(const OpenSSLBIO&) = delete;
    OpenSSLBIO(OpenSSLBIO&& other) noexcept : ptr(other.ptr)
    {
        other.ptr = nullptr;
    }
    OpenSSLBIO& operator=(const OpenSSLBIO&) = delete;
    OpenSSLBIO& operator=(OpenSSLBIO&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }
        ptr = other.ptr;
        other.ptr = nullptr;
        return *this;
    }

    ~OpenSSLBIO()
    {
        BIO_free(ptr);
    }

    std::string_view getMemData()
    {
        char* data = nullptr;
        std::string_view result;
        long dataLen = BIO_get_mem_data(ptr, &data);
        if (dataLen > 0)
        {
            result = std::string_view(data, static_cast<size_t>(dataLen));
        }
        return result;
    }

    BIO* get()
    {
        return ptr;
    }

  private:
    BIO* ptr;
};

class OpenSSLX509Extension
{
  public:
    OpenSSLX509Extension(X509V3_CTX* ctx, int nid, const char* value) :
        ptr(X509V3_EXT_conf_nid(nullptr, ctx, nid, value))
    {
        if (ptr == nullptr)
        {
            throw std::runtime_error(
                "Error: In X509V3_EXT_conf_nid: " + std::string(value));
        }
    }
    OpenSSLX509Extension(const OpenSSLX509Extension&) = delete;
    OpenSSLX509Extension(OpenSSLX509Extension&& other) noexcept = delete;
    OpenSSLX509Extension& operator=(const OpenSSLX509Extension&) = delete;
    OpenSSLX509Extension& operator=(OpenSSLX509Extension&& other) noexcept =
        delete;
    X509_EXTENSION* get() const
    {
        return ptr;
    }
    ~OpenSSLX509Extension()
    {
        X509_EXTENSION_free(ptr);
    }
    X509_EXTENSION* ptr;
};

class OpenSSLX509;

struct OpenSSLEVPKey
{
    friend OpenSSLX509;

  private:
    OpenSSLEVPKey(EVP_PKEY* ptrIn) : ptr(ptrIn) {}

  public:
    OpenSSLEVPKey() : ptr(EVP_PKEY_new()) {}

    static std::optional<OpenSSLEVPKey> readFromPrivateKey(std::string& data)
    {
        OpenSSLBIO bufio(data);
        EVP_PKEY* retPtr =
            PEM_read_bio_PrivateKey(bufio.get(), nullptr, nullptr, nullptr);
        if (retPtr == nullptr)
        {
            return std::nullopt;
        }
        return {OpenSSLEVPKey(retPtr)};
    }

    OpenSSLEVPKey(const OpenSSLEVPKey&) = delete;
    OpenSSLEVPKey(OpenSSLEVPKey&& other) noexcept : ptr(other.ptr)
    {
        other.ptr = nullptr;
    }
    OpenSSLEVPKey& operator=(const OpenSSLEVPKey&) = delete;
    OpenSSLEVPKey& operator=(OpenSSLEVPKey&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }
        ptr = other.ptr;
        other.ptr = nullptr;
        return *this;
    }

    static OpenSSLEVPKey fromKeygen(EVP_PKEY_CTX* pctx)
    {
        OpenSSLEVPKey key;
        if (EVP_PKEY_keygen(pctx, &key.ptr) != 1)
        {
            throw std::runtime_error("Failed to generate key");
        }
        return key;
    }

    ~OpenSSLEVPKey()
    {
        EVP_PKEY_free(ptr);
    }
    EVP_PKEY* ptr;
};

class OpenSSLASN1String
{
  public:
    OpenSSLASN1String(std::string_view data) : ptr(ASN1_STRING_new())
    {
        if (ptr == nullptr)
        {
            throw std::runtime_error("Failed to create ASN1_STRING");
        }
        if (ASN1_STRING_set(ptr, data.data(), static_cast<int>(data.size())) !=
            1)
        {
            throw std::runtime_error("Failed to set ASN1_STRING");
        }
    }
    OpenSSLASN1String(ASN1_STRING* ptrIn) : ptr(ptrIn) {}

    OpenSSLASN1String(const OpenSSLASN1String&) = delete;
    OpenSSLASN1String(OpenSSLASN1String&& other) noexcept : ptr(other.ptr)
    {
        other.ptr = nullptr;
    }

    OpenSSLASN1String& operator=(const OpenSSLASN1String&) = delete;
    OpenSSLASN1String& operator=(OpenSSLASN1String&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }
        ptr = other.ptr;
        other.ptr = nullptr;
        return *this;
    }

    ~OpenSSLASN1String()
    {
        ASN1_STRING_free(ptr);
    }
    std::string_view getAsString() const
    {
        return {std::bit_cast<const char*>(ptr->data),
                static_cast<size_t>(ptr->length)};
    }

    ASN1_STRING* get() const
    {
        return ptr;
    }
    ASN1_STRING* release()
    {
        ASN1_STRING* temp = ptr;
        ptr = nullptr;
        return temp;
    }
    ASN1_STRING* ptr;
};

struct OpenSSLGeneralName
{
  public:
    OpenSSLGeneralName() : ptr(GENERAL_NAME_new()) {}

    OpenSSLGeneralName(int type, OpenSSLASN1String& value) :
        ptr(GENERAL_NAME_new())
    {
        GENERAL_NAME_set0_value(ptr, type, value.release());
    }

    OpenSSLGeneralName(const OpenSSLGeneralName&) = delete;
    OpenSSLGeneralName(OpenSSLGeneralName&& other) noexcept = delete;
    OpenSSLGeneralName& operator=(const OpenSSLGeneralName&) = delete;
    OpenSSLGeneralName& operator=(OpenSSLGeneralName&& other) noexcept = delete;

    GENERAL_NAME* release()
    {
        GENERAL_NAME* temp = ptr;
        ptr = nullptr;
        return temp;
    }

    ~OpenSSLGeneralName()
    {
        GENERAL_NAME_free(ptr);
    }

    GENERAL_NAME* ptr;
};

struct OpenSSLGeneralNames
{
  private:
    OpenSSLGeneralNames(GENERAL_NAMES* ptrIn) : ptr(ptrIn) {}

  public:
    OpenSSLGeneralNames() : ptr(GENERAL_NAMES_new()) {}

    static OpenSSLGeneralNames fromExt(X509* cert, int nid)
    {
        OpenSSLGeneralNames names(static_cast<GENERAL_NAMES*>(
            X509_get_ext_d2i(cert, nid, nullptr, nullptr)));
        return names;
    }

    // This parameter isn't "moved" but it is released, so clang-tidy flags
    // that it shoudln't be an rvalue.  rvalue keeps the API clear that the
    // element has been effectively moved out of even if std::move isn't
    // used.
    // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
    bool push(OpenSSLGeneralName&& name) const
    {
        return sk_GENERAL_NAME_push(ptr, name.release()) == 1;
    }

    OpenSSLGeneralNames(const OpenSSLGeneralNames&) = delete;
    OpenSSLGeneralNames(OpenSSLGeneralNames&& other) noexcept : ptr(other.ptr)
    {
        other.ptr = nullptr;
    }
    OpenSSLGeneralNames& operator=(const OpenSSLGeneralNames&) = delete;
    OpenSSLGeneralNames& operator=(OpenSSLGeneralNames&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }
        ptr = other.ptr;
        other.ptr = nullptr;
        return *this;
    }

    ~OpenSSLGeneralNames()
    {
        GENERAL_NAMES_free(ptr);
    }

    GENERAL_NAMES* get() const
    {
        return ptr;
    }

    GENERAL_NAMES* ptr;
};

class OpenSSLX509StoreCTX;

class OpenSSLX509
{
  private:
    OpenSSLX509(X509* ptrIn) : ptr(ptrIn) {}

  public:
    OpenSSLX509() : ptr(X509_new()) {}
    OpenSSLX509(const OpenSSLX509&) = delete;
    OpenSSLX509(OpenSSLX509&& other) noexcept : ptr(other.ptr)
    {
        other.ptr = nullptr;
    }
    OpenSSLX509& operator=(const OpenSSLX509&) = delete;
    OpenSSLX509& operator=(OpenSSLX509&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }
        ptr = other.ptr;
        other.ptr = nullptr;
        return *this;
    }

    static std::optional<OpenSSLX509> loadFromPEMData(std::string& data)
    {
        OpenSSLBIO bufio(data);
        X509* ptr = PEM_read_bio_X509(bufio.get(), nullptr, nullptr, nullptr);
        if (ptr == nullptr)
        {
            return std::nullopt;
        }
        return {OpenSSLX509(ptr)};
    }

    void setSubjectName() const
    {
        X509_NAME* name = X509_get_subject_name(ptr);
        std::array<unsigned char, 4> user = {'u', 's', 'e', 'r'};
        X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, user.data(),
                                   user.size(), -1, 0);
    }

    int add1ExtI2d(int nid, OpenSSLGeneralNames& names) const
    {
        return X509_add1_ext_i2d(ptr, nid, names.get(), 0, 0);
    }

    bool sign() const
    {
        // Generate test key
        EVP_PKEY_CTX* pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr);
        if (EVP_PKEY_keygen_init(pctx) != 1)
        {
            return false;
        }
        if (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pctx,
                                                   NID_X9_62_prime256v1) != 1)
        {
            return false;
        }
        OpenSSLEVPKey key = OpenSSLEVPKey::fromKeygen(pctx);
        EVP_PKEY_CTX_free(pctx);

        // Sign cert with key
        if (X509_set_pubkey(ptr, key.ptr) != 1)
        {
            return false;
        }
        if (X509_sign(ptr, key.ptr, EVP_sha256()) <= 0)
        {
            return false;
        }
        return true;
    }

    std::optional<OpenSSLEVPKey> getPubKey() const
    {
        EVP_PKEY* pkey = X509_get_pubkey(ptr);
        if (pkey == nullptr)
        {
            return std::nullopt;
        }
        return {OpenSSLEVPKey(pkey)};
    }

    int verify(OpenSSLEVPKey& key) const
    {
        return X509_verify(ptr, key.ptr);
    }

    int addExt(int nid, const char* value) const
    {
        X509V3_CTX ctx{};
        X509V3_set_ctx(&ctx, ptr, ptr, nullptr, nullptr, 0);
        OpenSSLX509Extension ex(&ctx, nid, value);
        X509_add_ext(ptr, ex.get(), -1);
        return 0;
    }

    std::optional<OpenSSLASN1String> getExt(int nid) const
    {
        ASN1_STRING* out = static_cast<ASN1_STRING*>(
            X509_get_ext_d2i(ptr, nid, nullptr, nullptr));
        if (out == nullptr)
        {
            return std::nullopt;
        }
        return {OpenSSLASN1String(out)};
    }

    static std::optional<OpenSSLX509> fromPEMData(std::string& data)
    {
        OpenSSLBIO bufio(data);
        return fromBioFile(bufio);
    }

    static std::optional<OpenSSLX509> fromPEMFile(
        const std::filesystem::path& filepath)
    {
        OpenSSLBIO bufio(filepath);
        return fromBioFile(bufio);
    }

    static std::optional<OpenSSLX509> fromBioFile(OpenSSLBIO& bufio)
    {
        X509* ptr = PEM_read_bio_X509(bufio.get(), nullptr, nullptr, nullptr);
        if (ptr == nullptr)
        {
            return std::nullopt;
        }
        return {OpenSSLX509(ptr)};
    }

    ~OpenSSLX509()
    {
        X509_free(ptr);
    }

    // TODO: Remove this, as it's breaking the abstraction
    X509* get() const
    {
        return ptr;
    }

  private:
    X509* ptr = nullptr;
};

class OpenSSLX509Store
{
  public:
    friend OpenSSLX509StoreCTX;

    OpenSSLX509Store() : ptr(X509_STORE_new()) {}

    OpenSSLX509Store(const OpenSSLX509Store&) = delete;
    OpenSSLX509Store(OpenSSLX509Store&& other) noexcept : ptr(other.ptr)
    {
        other.ptr = nullptr;
    }
    OpenSSLX509Store& operator=(const OpenSSLX509Store&) = delete;
    OpenSSLX509Store& operator=(OpenSSLX509Store&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }
        ptr = other.ptr;
        other.ptr = nullptr;
        return *this;
    }
    ~OpenSSLX509Store()
    {
        X509_STORE_free(ptr);
    }

  private:
    X509_STORE* get() const
    {
        return ptr;
    }
    X509_STORE* ptr;
};

class OpenSSLX509StoreCTX
{
  public:
    OpenSSLX509StoreCTX() : ptr(X509_STORE_CTX_new()) {}

    OpenSSLX509StoreCTX(const OpenSSLX509StoreCTX&) = delete;
    OpenSSLX509StoreCTX(OpenSSLX509StoreCTX&& other) noexcept : ptr(other.ptr)
    {
        other.ptr = nullptr;
    }
    OpenSSLX509StoreCTX& operator=(const OpenSSLX509StoreCTX&) = delete;
    OpenSSLX509StoreCTX& operator=(OpenSSLX509StoreCTX&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }
        ptr = other.ptr;
        other.ptr = nullptr;
        return *this;
    }

    int init(OpenSSLX509Store& store, OpenSSLX509& cert)
    {
        return X509_STORE_CTX_init(ptr, store.get(), cert.get(), nullptr);
    }

    int verifyCert()
    {
        return X509_verify_cert(ptr);
    }

    ~OpenSSLX509StoreCTX()
    {
        X509_STORE_CTX_free(ptr);
    }

    static std::optional<OpenSSLX509StoreCTX> fromCert(OpenSSLX509& cert)
    {
        OpenSSLX509StoreCTX os;
        X509_STORE_CTX_set_current_cert(os.get(), cert.get());
        return {std::move(os)};
    }

    int getError() const
    {
        return X509_STORE_CTX_get_error(ptr);
    }

    boost::asio::ssl::verify_context releaseToVerifyContext()
    {
        X509_STORE_CTX* temp = ptr;
        ptr = nullptr;
        return boost::asio::ssl::verify_context(temp);
    }

  private:
    X509_STORE_CTX* get() const
    {
        return ptr;
    }

    X509_STORE_CTX* ptr;
};
