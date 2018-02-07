/*
 * Copyright (C) 2016-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of DualBootPatcher
 *
 * DualBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DualBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DualBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mbsign/sign.h"

#include <memory>
#include <string>

#include <cstring>

#ifdef __clang__
#  pragma GCC diagnostic push
#  if __has_warning("-Wold-style-cast")
#    pragma GCC diagnostic ignored "-Wold-style-cast"
#  endif
#endif

#ifdef OPENSSL_IS_BORINGSSL
#include <openssl/mem.h>
#endif
#include <openssl/pem.h>
#include <openssl/pkcs12.h>

#ifdef __clang__
#  pragma GCC diagnostic pop
#endif

#include "mbcommon/finally.h"

constexpr char MAGIC[]                      = "!MBSIGN!";
constexpr size_t MAGIC_SIZE                 = 8;

constexpr uint32_t VERSION_1_SHA512_DGST    = 1u;
constexpr uint32_t VERSION_LATEST           = VERSION_1_SHA512_DGST;

// NOTE: All integers are stored in little endian form
struct SigHeader
{
    char magic[MAGIC_SIZE];
    uint32_t version;
    uint32_t flags;
    uint32_t unused;
};

namespace mb::sign
{

template <typename T>
using ScopedMallocable = std::unique_ptr<T, decltype(free) *>;

using ScopedBIO = std::unique_ptr<BIO, decltype(BIO_free) *>;
using ScopedPKCS12 = std::unique_ptr<PKCS12, decltype(PKCS12_free) *>;

/*!
 * \brief Password callback
 *
 * \param buf Password output buffer
 * \param size Password output buffer size
 * \param rwflag Read/write flag (0 when reading, 1 when writing)
 * \param userdata User-provided callback data
 *
 * \return Number of characters in password (excluding NULL-terminator) or 0 on
 *         error (\a userdata is null)
 */
static int password_callback(char *buf, int size, int rwflag, void *userdata)
{
    (void) rwflag;

    if (userdata) {
        const char *password = static_cast<const char *>(userdata);

        int res = static_cast<int>(strlen(password));
        if (res > size) {
            res = size;
        }
        memcpy(buf, password, static_cast<size_t>(res));
        return res;
    }
    return 0;
}

static void openssl_free_wrapper(void *ptr) noexcept
{
    OPENSSL_free(ptr);
}

/*!
 * \brief Load PKCS12 structure from a BIO stream
 *
 * \note \a pkey, \a cert, and \a ca must be freed with the appropriate
 *       functions if this function returns success.
 *
 * \param[in] bio_pkcs12 Input stream for PKCS12 structure
 * \param[in] pem_cb Password callback
 * \param[in] cb_data User-provided data to pass to password callback
 * \param[out] pkey Output private key
 * \param[out] cert Output certificate corresponding to \a pkey
 * \param[out] ca Output pointer for list of additional certificates
 *
 * \return Whether the PKCS12 structure was successfully loaded
 */
static Result<void>
load_pkcs12(BIO &bio_pkcs12, pem_password_cb &pem_cb, void *cb_data,
            EVP_PKEY *&pkey, X509 *&cert, STACK_OF(X509) **ca)
{
    // Load PKCS12 from stream
    ScopedPKCS12 p12(d2i_PKCS12_bio(&bio_pkcs12, nullptr), PKCS12_free);
    if (!p12) {
        return ErrorInfo{Error::Pkcs12LoadError, true};
    }

    char buf[PEM_BUFSIZE];
    const char *pass;

    // Try empty password
    if (PKCS12_verify_mac(p12.get(), "", 0)
            || PKCS12_verify_mac(p12.get(), nullptr, 0)) {
        pass = "";
    } else {
        // Otherwise, get password from callback
        int len = pem_cb(buf, sizeof(buf), 0, cb_data);
        if (len < 0) {
            return ErrorInfo{Error::InternalError, false};
        }
        if (len < static_cast<int>(sizeof(buf))) {
            buf[len] = '\0';
        }
        if (!PKCS12_verify_mac(p12.get(), buf, len)) {
            return ErrorInfo{Error::Pkcs12MacVerifyError, true};
        }
        pass = buf;
    }

    if (!PKCS12_parse(p12.get(), pass, &pkey, &cert, ca)) {
        return ErrorInfo{Error::Pkcs12LoadError, true};
    }

    return oc::success();
}

/*!
 * \brief Load private key from BIO stream
 *
 * \note The return value must be freed with EVP_PKEY_free()
 *
 * \param bio_key Input stream for private key
 * \param format Format of input stream
 * \param pass Passphrase (can be null)
 *
 * \return EVP_PKEY object for the private key
 */
Result<ScopedEVP_PKEY>
load_private_key(BIO &bio_key, KeyFormat format, const char *pass)
{
    EVP_PKEY *pkey;

    switch (format) {
    case KeyFormat::Pem:
        pkey = PEM_read_bio_PrivateKey(&bio_key, nullptr, &password_callback,
                                       const_cast<char *>(pass));
        break;
    case KeyFormat::Pkcs12: {
        X509 *x509 = nullptr;

        auto free_x509 = finally([&x509] {
            X509_free(x509);
        });

        OUTCOME_TRYV(load_pkcs12(bio_key, password_callback,
                                 const_cast<char *>(pass), pkey, x509,
                                 nullptr));

        break;
    }
    default:
        return ErrorInfo{Error::InvalidKeyFormat, false};
    }

    if (!pkey) {
        return ErrorInfo{Error::PrivateKeyLoadError, true};
    }
    return {pkey, EVP_PKEY_free};
}

/*!
 * \brief Load private key from file
 *
 * \note The return value must be freed with EVP_PKEY_free()
 *
 * \param file Private key file
 * \param format Format of key file
 * \param pass Passphrase (can be null)
 *
 * \return EVP_PKEY object for the private key
 */
Result<ScopedEVP_PKEY>
load_private_key_from_file(const char *file, KeyFormat format, const char *pass)
{
    ScopedBIO bio_key(BIO_new_file(file, "rb"), BIO_free);
    if (!bio_key) {
        return ErrorInfo{Error::IoError, true};
    }

    return load_private_key(*bio_key, format, pass);
}

/*!
 * \brief Load public key from BIO stream
 *
 * \note The return value must be freed with EVP_PKEY_free()
 *
 * \param bio_key Input stream for public key
 * \param format Format of input stream
 * \param pass Passphrase (can be null)
 *
 * \return EVP_PKEY object for the public key
 */
Result<ScopedEVP_PKEY>
load_public_key(BIO &bio_key, KeyFormat format, const char *pass)
{
    EVP_PKEY *pkey = nullptr;

    switch (format) {
    case KeyFormat::Pem:
        pkey = PEM_read_bio_PUBKEY(&bio_key, nullptr, &password_callback,
                                   const_cast<char *>(pass));
        break;
    case KeyFormat::Pkcs12: {
        EVP_PKEY *private_key = nullptr;

        auto free_public_key = finally([&private_key] {
            EVP_PKEY_free(private_key);
        });

        X509 *x509 = nullptr;

        auto free_x509 = finally([&x509] {
            X509_free(x509);
        });

        OUTCOME_TRYV(load_pkcs12(bio_key, password_callback,
                                 const_cast<char *>(pass), private_key, x509,
                                 nullptr));

        if (x509) {
            pkey = X509_get_pubkey(x509);
        }

        break;
    }
    default:
        return ErrorInfo{Error::InvalidKeyFormat, false};
    }

    if (!pkey) {
        return ErrorInfo{Error::PublicKeyLoadError, true};
    }
    return {pkey, EVP_PKEY_free};
}

/*!
 * \brief Load public key from file
 *
 * \note The return value must be freed with EVP_PKEY_free()
 *
 * \param file Public key file
 * \param format Format of key file
 * \param pass Passphrase (can be null)
 *
 * \return EVP_PKEY object for the public key
 */
Result<ScopedEVP_PKEY>
load_public_key_from_file(const char *file, KeyFormat format, const char *pass)
{
    ScopedBIO bio_key(BIO_new_file(file, "rb"), BIO_free);
    if (!bio_key) {
        return ErrorInfo{Error::IoError, true};
    }

    return load_public_key(*bio_key, format, pass);
}

/*!
 * \brief Sign data from stream
 *
 * \param bio_data_in Input stream for data
 * \param bio_sig_out Output stream for signature
 * \param pkey Private key
 *
 * \return Whether the signing operation was successful
 */
Result<void> sign_data(BIO &bio_data_in, BIO &bio_sig_out, EVP_PKEY &pkey)
{
    constexpr unsigned int version = VERSION_LATEST;
    const EVP_MD *md_type = nullptr;
    EVP_MD_CTX *mctx = nullptr;

    if (version == VERSION_1_SHA512_DGST) {
        md_type = EVP_sha512();
    } else {
        return ErrorInfo{Error::InvalidSignatureVersion, false};
    }

#ifdef OPENSSL_IS_BORINGSSL
    EVP_MD_CTX ctx;
    EVP_MD_CTX_init(&ctx);

    auto free_ctx = finally([&ctx] {
        EVP_MD_CTX_cleanup(&ctx);
    });

    mctx = &ctx;
#else
    ScopedBIO bio_md(BIO_new(BIO_f_md()), BIO_free);
    if (!bio_md) {
        return ErrorInfo{Error::OpensslError, true};
    }

    if (!BIO_get_md_ctx(bio_md.get(), &mctx)) {
        return ErrorInfo{Error::OpensslError, true};
    }
#endif

    if (!EVP_DigestSignInit(mctx, nullptr, md_type, nullptr, &pkey)) {
        return ErrorInfo{Error::OpensslError, true};
    }

    constexpr size_t buf_size = 8192;
    ScopedMallocable<unsigned char> buf(
            static_cast<unsigned char *>(OPENSSL_malloc(buf_size)),
            openssl_free_wrapper);
    if (!buf) {
        return ErrorInfo{Error::OpensslError, true};
    }

#ifdef OPENSSL_IS_BORINGSSL
    BIO *bio_input = &bio_data_in;
#else
    BIO *bio_input = BIO_push(bio_md.get(), &bio_data_in);
#endif

    while (true) {
        int n = BIO_read(bio_input, buf.get(), buf_size);
        if (n < 0) {
            return ErrorInfo{Error::IoError, true};
        }
        if (n == 0) {
            break;
        }
#ifdef OPENSSL_IS_BORINGSSL
        if (!EVP_DigestUpdate(mctx, buf.get(), static_cast<size_t>(n))) {
            return ErrorInfo{Error::OpensslError, true};
        }
#endif
    }

    size_t len = buf_size;
    if (!EVP_DigestSignFinal(mctx, buf.get(), &len)) {
        return ErrorInfo{Error::OpensslError, true};
    }

    // Write header
    SigHeader hdr = {};
    memcpy(hdr.magic, MAGIC, MAGIC_SIZE);
    hdr.version = version;

    if (BIO_write(&bio_sig_out, &hdr, static_cast<int>(sizeof(hdr)))
            != static_cast<int>(sizeof(hdr))) {
        return ErrorInfo{Error::IoError, true};
    }

    if (BIO_write(&bio_sig_out, buf.get(), static_cast<int>(len))
            != static_cast<int>(len)) {
        return ErrorInfo{Error::IoError, true};
    }

    return oc::success();
}

/*!
 * \brief Verify signature of data from stream
 *
 * \param bio_data_in Input stream for data
 * \param bio_sig_in Input stream for signature
 * \param pkey Public key
 *
 * \return Whether the verification operation completed successfully. If the
 *         signature is invalid, the error code will be set to
 *         Error::BadSignature.
 */
Result<void>
verify_data(BIO &bio_data_in, BIO &bio_sig_in, EVP_PKEY &pkey)
{
    EVP_MD_CTX *mctx = nullptr;

#ifdef OPENSSL_IS_BORINGSSL
    EVP_MD_CTX ctx;
    EVP_MD_CTX_init(&ctx);

    auto free_ctx = finally([&ctx] {
        EVP_MD_CTX_cleanup(&ctx);
    });

    mctx = &ctx;
#else
    ScopedBIO bio_md(BIO_new(BIO_f_md()), BIO_free);
    if (!bio_md) {
        return ErrorInfo{Error::OpensslError, true};
    }

    if (!BIO_get_md_ctx(bio_md.get(), &mctx)) {
        return ErrorInfo{Error::OpensslError, true};
    }
#endif

    // Read header from signature file
    SigHeader hdr;
    if (BIO_read(&bio_sig_in, &hdr, static_cast<int>(sizeof(hdr)))
            != static_cast<int>(sizeof(hdr))) {
        return ErrorInfo{Error::IoError, true};
    }

    // Verify header
    if (memcmp(hdr.magic, MAGIC, MAGIC_SIZE) != 0) {
        return ErrorInfo{Error::InvalidSignatureMagic, false};
    }

    // Verify version
    const EVP_MD *md_type = nullptr;
    if (hdr.version == VERSION_1_SHA512_DGST) {
        md_type = EVP_sha512();
    } else {
        return ErrorInfo{Error::InvalidSignatureVersion, false};
    }

    if (!EVP_DigestVerifyInit(mctx, nullptr, md_type, nullptr, &pkey)) {
        return ErrorInfo{Error::OpensslError, true};
    }

    constexpr size_t buf_size = 8192;
    ScopedMallocable<unsigned char> buf(
            static_cast<unsigned char *>(OPENSSL_malloc(buf_size)),
            openssl_free_wrapper);
    if (!buf) {
        return ErrorInfo{Error::OpensslError, true};
    }

    int siglen = EVP_PKEY_size(&pkey);
    ScopedMallocable<unsigned char> sigbuf(static_cast<unsigned char *>(
            OPENSSL_malloc(static_cast<size_t>(siglen))),
            openssl_free_wrapper);
    if (!sigbuf) {
        return ErrorInfo{Error::OpensslError, true};
    }
    siglen = BIO_read(&bio_sig_in, sigbuf.get(), siglen);
    if (siglen <= 0) {
        return ErrorInfo{Error::IoError, true};
    }

#ifdef OPENSSL_IS_BORINGSSL
    BIO *bio_input = &bio_data_in;
#else
    BIO *bio_input = BIO_push(bio_md.get(), &bio_data_in);
#endif

    while (true) {
        int n = BIO_read(bio_input, buf.get(), buf_size);
        if (n < 0) {
            return ErrorInfo{Error::IoError, true};
        }
        if (n == 0) {
            break;
        }
#ifdef OPENSSL_IS_BORINGSSL
        if (!EVP_DigestUpdate(mctx, buf.get(), static_cast<size_t>(n))) {
            return ErrorInfo{Error::OpensslError, true};
        }
#endif
    }

    int n = EVP_DigestVerifyFinal(mctx, sigbuf.get(),
                                  static_cast<size_t>(siglen));
    if (n == 1) {
        return oc::success();
    } else if (n == 0) {
        return ErrorInfo{Error::BadSignature, false};
    } else {
        return ErrorInfo{Error::OpensslError, true};
    }
}

}
