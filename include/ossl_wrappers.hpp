// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#pragma once
#include "logging.hpp"

#include <boost/asio/ssl/verify_context.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <filesystem>

extern "C"
{
#include <openssl/asn1.h>
#include <openssl/bio.h>
#include <openssl/ec.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/obj_mac.h>
#include <openssl/objects.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/tls1.h>
#include <openssl/types.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <openssl/x509v3.h>
}

class OpenSSLEVPKey;
class OpenSSLX509;

class OpenSSLBIO
{
  public:
    friend OpenSSLEVPKey;
    friend OpenSSLX509;
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
        BIO_free(ptr);
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

  private:
    BIO* get()
    {
        return ptr;
    }

    BIO* ptr;
};

class OpenSSLX509Extension
{
  public:
    friend OpenSSLX509;

    OpenSSLX509Extension(X509V3_CTX* ctx, int nid, std::string_view value)
    {
        std::string valueStr(value);
        ptr = X509V3_EXT_conf_nid(nullptr, ctx, nid, valueStr.c_str());
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

    ~OpenSSLX509Extension()
    {
        X509_EXTENSION_free(ptr);
    }

  private:
    X509_EXTENSION* get()
    {
        return ptr;
    }

    X509_EXTENSION* ptr;
};

class OpenSSLEVPKeyCTX;
class OpenSSLSSLCtx;
class OpenSSLSSL;

class OpenSSLEVPKey
{
  public:
    friend OpenSSLX509;
    friend OpenSSLEVPKeyCTX;
    friend OpenSSLSSLCtx;
    friend OpenSSLSSL;

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

    bool pemWriteBioPrivateKey(OpenSSLBIO& bufio) const
    {
        return PEM_write_bio_PrivateKey(bufio.get(), ptr, nullptr, nullptr, 0,
                                        nullptr, nullptr) > 0;
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
        EVP_PKEY_free(ptr);
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

  private:
    EVP_PKEY* get() const
    {
        return ptr;
    }

    OpenSSLEVPKey(EVP_PKEY* ptrIn) : ptr(ptrIn) {}

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
        ASN1_STRING_free(ptr);
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

    ASN1_STRING* release()
    {
        ASN1_STRING* temp = ptr;
        ptr = nullptr;
        return temp;
    }

  private:
    ASN1_STRING* get() const
    {
        return ptr;
    }

    ASN1_STRING* ptr;
};

class OpenSSLGeneralNames;

struct OpenSSLGeneralName
{
  public:
    friend OpenSSLGeneralNames;
    OpenSSLGeneralName() : ptr(GENERAL_NAME_new()) {}

    OpenSSLGeneralName(int type, OpenSSLASN1String& value) :
        ptr(GENERAL_NAME_new())
    {
        if (type >= 0 && type <= GEN_RID)
        {
            GENERAL_NAME_set0_value(ptr, type, value.release());
        }
        else
        {
            // type is a NID: build an otherName SAN with that OID
            ASN1_OBJECT* oid = OBJ_dup(OBJ_nid2obj(type));
            ASN1_STRING* str = value.release();
            str->type = V_ASN1_UTF8STRING;
            ASN1_TYPE* atype = ASN1_TYPE_new();
            ASN1_TYPE_set(atype, V_ASN1_UTF8STRING, str);
            GENERAL_NAME_set0_othername(ptr, oid, atype);
        }
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

  private:
    GENERAL_NAME* ptr;
};

class OpenSSLGeneralNames
{
  private:
    OpenSSLGeneralNames(GENERAL_NAMES* ptrIn) : ptr(ptrIn) {}

  public:
    friend OpenSSLX509;
    OpenSSLGeneralNames() : ptr(GENERAL_NAMES_new()) {}

    // This parameter isn't "moved" but it is released, so clang-tidy flags
    // that it shoudln't be an rvalue.  rvalue keeps the API clear that the
    // element has been effectively moved out of even if std::move isn't
    // used.
    // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
    bool push(OpenSSLGeneralName&& name) const
    {
        return sk_GENERAL_NAME_push(ptr, name.release()) > 0;
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
        GENERAL_NAMES_free(ptr);
        ptr = other.ptr;
        other.ptr = nullptr;
        return *this;
    }

    class Iterator
    {
      public:
        GENERAL_NAMES* names;
        int index;

        GENERAL_NAME& operator*() const
        {
            return *sk_GENERAL_NAME_value(names, index);
        }

        auto operator<=>(const Iterator& other) const
        {
            return index <=> other.index;
        }

        Iterator& operator++()
        {
            index++;
            return *this;
        }

        bool operator!=(const Iterator& other) const
        {
            return index != other.index;
        }
    };

    Iterator begin()
    {
        return Iterator(ptr, 0);
    }

    Iterator end()
    {
        return Iterator(ptr, sk_GENERAL_NAME_num(ptr));
    }

    ~OpenSSLGeneralNames()
    {
        GENERAL_NAMES_free(ptr);
    }

  private:
    GENERAL_NAMES* get() const
    {
        return ptr;
    }

    GENERAL_NAMES* ptr;
};

class OpenSSLEVPKeyCTX
{
  private:
    OpenSSLEVPKeyCTX(EVP_PKEY_CTX* ptrIn) : ptr(ptrIn) {}

  public:
    OpenSSLEVPKeyCTX(const OpenSSLEVPKeyCTX&) = delete;
    OpenSSLEVPKeyCTX(OpenSSLEVPKeyCTX&& other) noexcept : ptr(other.ptr)
    {
        other.ptr = nullptr;
    }
    OpenSSLEVPKeyCTX& operator=(const OpenSSLEVPKeyCTX&) = delete;
    OpenSSLEVPKeyCTX& operator=(OpenSSLEVPKeyCTX&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }
        EVP_PKEY_CTX_free(ptr);
        ptr = other.ptr;
        other.ptr = nullptr;
        return *this;
    }

    static std::optional<OpenSSLEVPKeyCTX> newId(int id)
    {
        EVP_PKEY_CTX* ptr = EVP_PKEY_CTX_new_id(id, nullptr);
        if (ptr == nullptr)
        {
            return std::nullopt;
        }
        return {OpenSSLEVPKeyCTX(ptr)};
    }

    static std::optional<OpenSSLEVPKeyCTX> newFromPkey(OpenSSLEVPKey& pkey)
    {
        EVP_PKEY_CTX* ptr =
            EVP_PKEY_CTX_new_from_pkey(nullptr, pkey.get(), nullptr);
        if (ptr == nullptr)
        {
            return std::nullopt;
        }
        return {OpenSSLEVPKeyCTX(ptr)};
    }

    static std::optional<OpenSSLEVPKey> createPKey()
    {
        std::optional<OpenSSLEVPKeyCTX> ctx = newId(EVP_PKEY_EC);
        if (!ctx)
        {
            BMCWEB_LOG_ERROR("Failed new id");
            return std::nullopt;
        }

        if (!ctx->paramgenInit())
        {
            BMCWEB_LOG_ERROR("Failed to initialize paramgen");
            return std::nullopt;
        }

        // Set up curve parameters.
        if (!ctx->setEcParamEnc(OPENSSL_EC_NAMED_CURVE))
        {
            BMCWEB_LOG_ERROR("Failed to set EC parameter encoding");
            return std::nullopt;
        }

        if (!ctx->setEcParamGenCurveNid(NID_secp384r1))
        {
            BMCWEB_LOG_ERROR("Failed to set EC parameter generation curve NID");
            return std::nullopt;
        }

        std::optional<OpenSSLEVPKey> pkey = ctx->paramgen();
        if (!pkey)
        {
            BMCWEB_LOG_ERROR("Failed to generate EC key");
            return std::nullopt;
        }

        // Set new context for key generation, using curve parameters.
        // Don't check here; pkey only has parameters, not a full key yet.
        ctx = OpenSSLEVPKeyCTX::newFromPkey(*pkey);
        if (!ctx)
        {
            BMCWEB_LOG_ERROR("Failed to create new context for key generation");
            return std::nullopt;
        }

        if (!ctx->keygenInit())
        {
            BMCWEB_LOG_ERROR("Failed to initialize keygen");
            return std::nullopt;
        }

        // Generate key.
        if (!ctx->keygen(*pkey))
        {
            BMCWEB_LOG_ERROR("Failed to generate key");
            return std::nullopt;
        }

        if (!ctx->check())
        {
            BMCWEB_LOG_ERROR("Generated EC key failed validation");
            return std::nullopt;
        }

        return pkey;
    }

    bool keygenInit()
    {
        return EVP_PKEY_keygen_init(ptr) == 1;
    }

    bool paramgenInit()
    {
        return EVP_PKEY_paramgen_init(ptr) == 1;
    }

    bool check()
    {
        int r = EVP_PKEY_check(ptr);
        if (r != 1)
        {
            BMCWEB_LOG_ERROR("Failed to check private key {}", r);
        }
        return r == 1;
    }

    bool keygen(OpenSSLEVPKey& pkey) const
    {
        return EVP_PKEY_keygen(get(), &pkey.ptr) == 1;
    }

    bool setEcParamEnc(int enc) const
    {
        return EVP_PKEY_CTX_set_ec_param_enc(get(), enc) == 1;
    }

    bool setEcParamGenCurveNid(int nid) const
    {
        return EVP_PKEY_CTX_set_ec_paramgen_curve_nid(get(), nid) == 1;
    }

    std::optional<OpenSSLEVPKey> paramgen() const
    {
        OpenSSLEVPKey ret(nullptr);
        if (EVP_PKEY_paramgen(get(), &ret.ptr) != 1)
        {
            return std::nullopt;
        }
        return {std::move(ret)};
    }

    ~OpenSSLEVPKeyCTX()
    {
        EVP_PKEY_CTX_free(ptr);
    }

    EVP_PKEY_CTX* get() const
    {
        return ptr;
    }

  private:
    EVP_PKEY_CTX* ptr;
};

class OpenSSLX509StoreCTX;
class OpenSSLSSL;

class OpenSSLX509
{
    friend OpenSSLX509StoreCTX;
    friend OpenSSLEVPKeyCTX;
    friend OpenSSLSSLCtx;
    friend OpenSSLSSL;

  private:
    OpenSSLX509(X509* ptrIn) : ptr(ptrIn), ptrOwned(false) {}

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
        X509_free(ptr);
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

    bool pemWriteBioX509(OpenSSLBIO& bufio) const
    {
        return PEM_write_bio_X509(bufio.get(), ptr) > 0;
    }

    void setSubjectName(std::string_view cn) const
    {
        X509_NAME* name = X509_get_subject_name(ptr);
        const unsigned char* cnPtr =
            std::bit_cast<const unsigned char*>(cn.data());
        int cnLength = static_cast<int>(cn.size());
        X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, cnPtr, cnLength,
                                   -1, 0);
    }

    void setCountry(std::string_view country) const
    {
        X509_NAME* name = X509_get_subject_name(ptr);
        const unsigned char* countryPtr =
            std::bit_cast<const unsigned char*>(country.data());
        int countryLength = static_cast<int>(country.size());
        X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, countryPtr,
                                   countryLength, -1, 0);
    }

    void setOrganization(std::string_view organization) const
    {
        X509_NAME* name = X509_get_subject_name(ptr);
        const unsigned char* organizationPtr =
            std::bit_cast<const unsigned char*>(organization.data());
        int organizationLength = static_cast<int>(organization.size());
        X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, organizationPtr,
                                   organizationLength, -1, 0);
    }

    bool setIssuerNameToSubject() const
    {
        return X509_set_issuer_name(ptr, X509_get_subject_name(ptr)) == 1;
    }

    void setSerialNumber(uint64_t n) const
    {
        ASN1_INTEGER_set_uint64(X509_get_serialNumber(ptr), n);
    }

    void setValidityPeriodFromNow(
        std::chrono::system_clock::duration duration) const
    {
        // not before this moment
        X509_gmtime_adj(X509_getm_notBefore(ptr), 0);
        using std::chrono::floor;
        using OsslSeconds = std::chrono::duration<long>;
        OsslSeconds durationSeconds = floor<OsslSeconds>(duration);
        X509_gmtime_adj(X509_getm_notAfter(ptr), durationSeconds.count());
    }

    bool setPubkey(OpenSSLEVPKey& pkey) const
    {
        return X509_set_pubkey(ptr, pkey.get()) == 1;
    }

    bool setVersion(int version) const
    {
        return X509_set_version(ptr, version) == 1;
    }

    bool addAltNameUpns(std::initializer_list<std::string_view> upns) const
    {
        return addAltNames(NID_ms_upn, upns);
    }

    bool addAltNameEmails(
        std::initializer_list<std::string_view> altNames) const
    {
        return addAltNames(GEN_EMAIL, altNames);
    }

    bool addAltNames(int gnType,
                     std::initializer_list<std::string_view> altNames) const
    {
        OpenSSLGeneralNames gens;
        for (const std::string_view altName : altNames)
        {
            OpenSSLASN1String altNameStr(altName);
            OpenSSLGeneralName gen(gnType, altNameStr);
            if (!gens.push(std::move(gen)))
            {
                return false;
            }
        }

        return add1ExtI2d(NID_subject_alt_name, gens) > 0;
    }

    int add1ExtI2d(int nid, OpenSSLGeneralNames& names) const
    {
        return X509_add1_ext_i2d(ptr, nid, names.get(), 0, 0);
    }

    bool sign() const
    {
        // Generate test key
        std::optional<OpenSSLEVPKeyCTX> ctx =
            OpenSSLEVPKeyCTX::newId(EVP_PKEY_EC);
        if (!ctx)
        {
            return false;
        }
        if (!ctx->keygenInit())
        {
            return false;
        }
        if (!ctx->setEcParamGenCurveNid(NID_X9_62_prime256v1))
        {
            return false;
        }

        OpenSSLEVPKey key = OpenSSLEVPKey::fromKeygen(ctx->get());

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

    bool sign(OpenSSLEVPKey& pkey) const
    {
        return X509_sign(ptr, pkey.get(), EVP_sha256()) > 0;
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

    std::string getCommonName() const
    {
        std::string commonName;
        // Extract username contained in CommonName
        int commonNameLenMax = 256;
        commonName.resize(static_cast<size_t>(commonNameLenMax), '\0');
        int length = X509_NAME_get_text_by_NID(
            X509_get_subject_name(ptr), NID_commonName, commonName.data(),
            commonNameLenMax);
        if (length <= 0)
        {
            BMCWEB_LOG_DEBUG("TLS cannot get common name to create session");
            length = 0;
        }
        commonName.resize(static_cast<size_t>(length));
        return commonName;
    }

    std::string getComment() const
    {
        ASN1_STRING* r = static_cast<ASN1_STRING*>(
            X509_get_ext_d2i(ptr, NID_netscape_comment, nullptr, nullptr));
        if (r == nullptr)
        {
            return "";
        }
        OpenSSLASN1String asn1(static_cast<ASN1_IA5STRING*>(r));
        return std::string(asn1.getAsString());
    }

    int verify(OpenSSLEVPKey& key) const
    {
        return X509_verify(ptr, key.ptr);
    }

    int addExt(int nid, std::string_view value) const
    {
        X509V3_CTX ctx{};
        X509V3_set_ctx(&ctx, ptr, ptr, nullptr, nullptr, 0);
        OpenSSLX509Extension ex(&ctx, nid, value);
        X509_add_ext(ptr, ex.get(), -1);
        return 0;
    }

    std::optional<OpenSSLASN1String> getExtStr(int nid) const
    {
        ASN1_STRING* out = static_cast<ASN1_STRING*>(
            X509_get_ext_d2i(ptr, nid, nullptr, nullptr));
        if (out == nullptr)
        {
            return std::nullopt;
        }
        return {OpenSSLASN1String(out)};
    }

    std::optional<OpenSSLGeneralNames> getAltNames() const
    {
        GENERAL_NAMES* ext = static_cast<GENERAL_NAMES*>(
            X509_get_ext_d2i(ptr, NID_subject_alt_name, nullptr, nullptr));
        if (ext == nullptr)
        {
            return std::nullopt;
        }
        return {OpenSSLGeneralNames(ext)};
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

    bool checkPurpose(int purpose) const
    {
        return X509_check_purpose(ptr, purpose, 0) == 1;
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
        if (ptrOwned)
        {
            X509_free(ptr);
        }
    }

  private:
    X509* get() const
    {
        return ptr;
    }

    X509* ptr;
    bool ptrOwned = true;
};

class OpenSSLX509Store
{
  public:
    friend OpenSSLX509StoreCTX;
    friend OpenSSLSSL;
    friend OpenSSLSSLCtx;

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
        X509_STORE_free(ptr);
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
        X509_STORE_CTX_free(ptr);
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

class OpenSSLSSLCtx
{
  public:
    friend OpenSSLSSL;

    static OpenSSLSSLCtx createServerCtx()
    {
        return OpenSSLSSLCtx(SSL_CTX_new(TLS_server_method()));
    }
    static OpenSSLSSLCtx createClientCtx()
    {
        return OpenSSLSSLCtx(SSL_CTX_new(TLS_client_method()));
    }

    OpenSSLSSLCtx(const OpenSSLSSLCtx&) = delete;
    OpenSSLSSLCtx& operator=(const OpenSSLSSLCtx&) = delete;
    OpenSSLSSLCtx(OpenSSLSSLCtx&& other) noexcept : ptr(other.ptr)
    {
        other.ptr = nullptr;
    }

    OpenSSLSSLCtx& operator=(OpenSSLSSLCtx&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }
        SSL_CTX_free(ptr);
        ptr = other.ptr;
        other.ptr = nullptr;
        return *this;
    }
    ~OpenSSLSSLCtx()
    {
        SSL_CTX_free(ptr);
    }

    bool valid() const
    {
        return ptr != nullptr;
    }

    bool useCertificate(const OpenSSLX509& cert) const
    {
        return SSL_CTX_use_certificate(ptr, cert.get()) == 1;
    }

    bool usePrivateKey(const OpenSSLEVPKey& pkey) const
    {
        return SSL_CTX_use_PrivateKey(ptr, pkey.get()) == 1;
    }

    void setVerifyPeer() const
    {
        SSL_CTX_set_verify(ptr, SSL_VERIFY_PEER, nullptr);
    }

    bool addCertToStore(const OpenSSLX509& cert) const
    {
        X509_STORE* store = SSL_CTX_get_cert_store(ptr);
        if (store == nullptr)
        {
            return false;
        }
        return X509_STORE_add_cert(store, cert.get()) == 1;
    }

  private:
    explicit OpenSSLSSLCtx(SSL_CTX* ptrIn) : ptr(ptrIn) {}

    SSL_CTX* get() const
    {
        return ptr;
    }

    SSL_CTX* ptr;
};

class OpenSSLSSL
{
  public:
    explicit OpenSSLSSL(const OpenSSLSSLCtx& ctx) : ptr(SSL_new(ctx.get())) {}
    explicit OpenSSLSSL(SSL* ptrIn) : ptr(ptrIn), ptrOwned(false) {}

    OpenSSLSSL(const OpenSSLSSL&) = delete;
    OpenSSLSSL& operator=(const OpenSSLSSL&) = delete;
    OpenSSLSSL(OpenSSLSSL&& other) noexcept : ptr(other.ptr)
    {
        other.ptr = nullptr;
    }
    OpenSSLSSL& operator=(OpenSSLSSL&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }
        SSL_free(ptr);
        ptr = other.ptr;
        other.ptr = nullptr;
        return *this;
    }
    ~OpenSSLSSL()
    {
        if (ptrOwned)
        {
            SSL_free(ptr);
        }
    }

    bool valid() const
    {
        return ptr != nullptr;
    }

    // Creates two memory BIOs for read/write and hands them to the SSL.
    // SSL_set_bio takes ownership, so the SSL_free in the destructor frees
    // them.
    bool setMemBio() const
    {
        BIO* rbio = BIO_new(BIO_s_mem());
        BIO* wbio = BIO_new(BIO_s_mem());
        if (rbio == nullptr || wbio == nullptr)
        {
            BIO_free(rbio);
            BIO_free(wbio);
            return false;
        }
        SSL_set_bio(ptr, rbio, wbio);
        return true;
    }

    void setAcceptState() const
    {
        SSL_set_accept_state(ptr);
    }

    void setConnectState() const
    {
        SSL_set_connect_state(ptr);
    }

    bool useCertificate(const OpenSSLX509& cert) const
    {
        return SSL_use_certificate(ptr, cert.get()) == 1;
    }

    bool usePrivateKey(const OpenSSLEVPKey& pkey) const
    {
        return SSL_use_PrivateKey(ptr, pkey.get()) == 1;
    }

    int doHandshake() const
    {
        return SSL_do_handshake(ptr);
    }

    std::optional<OpenSSLX509> getPeerCertificate() const
    {
        X509* cert = SSL_get1_peer_certificate(ptr);
        if (cert == nullptr)
        {
            return std::nullopt;
        }
        return {OpenSSLX509(cert)};
    }

    void transferTo(OpenSSLSSL& other) const
    {
        BIO* fromWbio = SSL_get_wbio(ptr);
        BIO* toRbio = SSL_get_rbio(other.ptr);
        std::array<char, 4096> buf{};
        int pending = static_cast<int>(BIO_ctrl_pending(fromWbio));
        while (pending > 0)
        {
            int n = BIO_read(fromWbio, buf.data(),
                             std::min(pending, static_cast<int>(buf.size())));
            if (n > 0)
            {
                BIO_write(toRbio, buf.data(), n);
            }
            pending = static_cast<int>(BIO_ctrl_pending(fromWbio));
        }
    }

    long getVerifyResult() const
    {
        return SSL_get_verify_result(ptr);
    }

  private:
    SSL* get() const
    {
        return ptr;
    }

    SSL* ptr;
    bool ptrOwned = true;
};
