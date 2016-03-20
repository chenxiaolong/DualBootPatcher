/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mbsign/mbsign.h"

#include <cassert>
#include <cstring>

#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/pkcs12.h>

#include "mblog/logging.h"

#undef BUFSIZE
#define BUFSIZE 1024 * 8

namespace mb
{
namespace sign
{

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
        const char *password = (const char *) userdata;

        int res = strlen(password);
        if (res > size) {
            res = size;
        }
        memcpy(buf, password, res);
        return res;
    }
    return 0;
}

/*!
 * \brief Logging callback
 *
 * \param str Log message
 * \param len strlen(str)
 * \param userdata User-provided callback data
 *
 * \return Positive integer for normal return. Non-positive integer to stop
 *         processing further lines in the error buffer
 */
static int log_callback(const char *str, size_t len, void *userdata)
{
    (void) userdata;
    char *copy = strdup(str);
    if (copy) {
        // Strip newline
        copy[len - 1] = '\0';
        LOGE("%s", copy);
        free(copy);
    }
    return len;
}

static void openssl_log_errors()
{
    ERR_print_errors_cb(&log_callback, nullptr);
}

/*!
 * \brief Load PKCS12 structure from a BIO stream
 *
 * \note \a pkey, \a cert, and \a ca must be freed with the appropriate
 *       functions if this function returns success.
 *
 * \param bio_pkcs12 Input stream for PKCS12 structure
 * \param pem_cb Password callback
 * \param cb_data User-provided data to pass to password callback
 * \param pkey Output pointer for private key
 * \param cert Output pointer for certificate corresponding to \a pkey
 * \param ca Output pointer for list of additional certificates
 *
 * \return Whether the PKCS12 structure was successfully loaded
 */
static bool load_pkcs12(BIO *bio_pkcs12, pem_password_cb *pem_cb, void *cb_data,
                        EVP_PKEY **pkey, X509 **cert, STACK_OF(X509) **ca)
{
    assert(bio_pkcs12 && pem_cb && pkey && cert);

    PKCS12 *p12;
    const char *pass;
    char buf[PEM_BUFSIZE];
    int len;
    int ret = 0;

    // Load PKCS12 from stream
    p12 = d2i_PKCS12_bio(bio_pkcs12, nullptr);
    if (!p12) {
        LOGE("Failed to load PKCS12 file");
        openssl_log_errors();
        goto finished;
    }

    // Try empty password
    if (PKCS12_verify_mac(p12, "", 0) || PKCS12_verify_mac(p12, nullptr, 0)) {
        pass = "";
    } else {
        // Otherwise, get password from callback
        len = pem_cb(buf, PEM_BUFSIZE, 0, cb_data);
        if (len < 0) {
            LOGE("Passphrase callback failed");
            goto finished;
        }
        if (len < PEM_BUFSIZE) {
            buf[len] = 0;
        }
        if (!PKCS12_verify_mac(p12, buf, len)) {
            LOGE("Failed to verify MAC in PKCS12 file"
                 " (possibly incorrect password)");
            openssl_log_errors();
            goto finished;
        }
        pass = buf;
    }
    ret = PKCS12_parse(p12, pass, pkey, cert, ca);

finished:
    PKCS12_free(p12);
    return ret > 0;
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
EVP_PKEY * load_private_key(BIO *bio_key, int format, const char *pass)
{
    assert(bio_key);

    EVP_PKEY *pkey;

    switch (format) {
    case KEY_FORMAT_PEM:
        pkey = PEM_read_bio_PrivateKey(
                bio_key, nullptr, &password_callback, (void *) pass);
        break;
    case KEY_FORMAT_PKCS12: {
        X509 *x509 = nullptr;

        if (!load_pkcs12(bio_key, &password_callback, (void *) pass,
                         &pkey, &x509, nullptr)) {
            return nullptr;
        }

        if (x509) {
            X509_free(x509);
        }

        break;
    }
    default:
        LOGE("Invalid key format");
        return nullptr;
    }

    if (!pkey) {
        LOGE("Failed to load private key");
        openssl_log_errors();
    }
    return pkey;
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
EVP_PKEY * load_private_key_from_file(const char *file, int format,
                                      const char *pass)
{
    BIO *bio_key;
    EVP_PKEY *pkey = nullptr;

    bio_key = BIO_new_file(file, "rb");
    if (!bio_key) {
        LOGE("%s: Failed to load private key", file);
        openssl_log_errors();
        return nullptr;
    }

    pkey = load_private_key(bio_key, format, pass);

    BIO_free(bio_key);
    return pkey;
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
EVP_PKEY * load_public_key(BIO *bio_key, int format, const char *pass)
{
    assert(bio_key);

    EVP_PKEY *pkey = nullptr;

    switch (format) {
    case KEY_FORMAT_PEM:
        pkey = PEM_read_bio_PUBKEY(
                bio_key, nullptr, &password_callback, (void *) pass);
        break;
    case KEY_FORMAT_PKCS12: {
        EVP_PKEY *public_key;
        X509 *x509 = nullptr;

        if (!load_pkcs12(bio_key, &password_callback, (void *) pass,
                         &public_key, &x509, nullptr)) {
            return nullptr;
        }

        if (x509) {
            pkey = X509_get_pubkey(x509);
            X509_free(x509);
        }
        EVP_PKEY_free(public_key);

        break;
    }
    default:
        LOGE("Invalid key format");
        return nullptr;
    }

    if (!pkey) {
        LOGE("Failed to load public key");
        openssl_log_errors();
    }
    return pkey;
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
EVP_PKEY * load_public_key_from_file(const char *file, int format,
                                     const char *pass)
{
    BIO *bio_key;
    EVP_PKEY *pkey = nullptr;

    bio_key = BIO_new_file(file, "rb");
    if (!bio_key) {
        LOGE("%s: Failed to load public key", file);
        openssl_log_errors();
        return nullptr;
    }

    pkey = load_public_key(bio_key, format, pass);

    BIO_free(bio_key);
    return pkey;
}

/*!
 * \brief Sign data from stream
 *
 * \param bio_data_in Input stream for data
 * \param bio_sig_out Output stream for signature
 * \param md_type Message digest algorithm
 * \param pkey Private key
 *
 * \return Whether the signing operation was successful
 */
bool sign_data(BIO *bio_data_in, BIO *bio_sig_out,
               const EVP_MD *md_type, EVP_PKEY *pkey)
{
    assert(bio_data_in && bio_sig_out && md_type && pkey);

    EVP_MD_CTX *mctx = nullptr;
    EVP_PKEY_CTX *pctx = nullptr;
    BIO *bio_md = nullptr;
    BIO *bio_input = nullptr;
    unsigned char *buf = nullptr;
    size_t len;
    int n;

    bio_md = BIO_new(BIO_f_md());
    if (!bio_md) {
        openssl_log_errors();
        goto error;
    }

    if (!BIO_get_md_ctx(bio_md, &mctx)) {
        LOGE("Failed to get message digest context");
        openssl_log_errors();
        goto error;
    }

    if (!EVP_DigestSignInit(mctx, &pctx, md_type, nullptr, pkey)) {
        LOGE("Failed to set message digest context");
        openssl_log_errors();
        goto error;
    }

    buf = (unsigned char *) OPENSSL_malloc(BUFSIZE);
    if (!buf) {
        LOGE("Failed to allocate I/O buffer");
        openssl_log_errors();
        goto error;
    }

    bio_input = BIO_push(bio_md, bio_data_in);

    while (true) {
        n = BIO_read(bio_input, buf, BUFSIZE);
        if (n < 0) {
            LOGE("Failed to read from input data BIO stream");
            openssl_log_errors();
            goto error;
        }
        if (n == 0) {
            break;
        }
    }

    len = BUFSIZE;
    if (!EVP_DigestSignFinal(mctx, buf, &len)) {
        LOGE("Failed to sign data");
        openssl_log_errors();
        goto error;
    }

    if (BIO_write(bio_sig_out, buf, len) != (int) len) {
        LOGE("Failed to write signature to signature BIO stream");
        openssl_log_errors();
        goto error;
    }

    OPENSSL_free(buf);
    BIO_free(bio_md);
    return true;

error:
    OPENSSL_free(buf);
    BIO_free(bio_md);
    return false;
}

/*!
 * \brief Verify signature of data from stream
 *
 * \param bio_data_in Input stream for data
 * \param bio_sig_in Input stream for signature
 * \param md_type Message digest algorithm
 * \param pkey Public key
 * \param result_out Output pointer for result of verification operation
 *
 * \return Whether the verification operation completed successfully (does not
 *         indicate whether the signature is valid)
 */
bool verify_data(BIO *bio_data_in, BIO *bio_sig_in,
                 const EVP_MD *md_type, EVP_PKEY *pkey,
                 bool *result_out)
{
    assert(bio_data_in && bio_sig_in && md_type && pkey && result_out);

    EVP_MD_CTX *mctx = nullptr;
    EVP_PKEY_CTX *pctx = nullptr;
    BIO *bio_md = nullptr;
    BIO *bio_input = nullptr;
    unsigned char *buf = nullptr;
    unsigned char *sigbuf = nullptr;
    int siglen;
    int n;

    bio_md = BIO_new(BIO_f_md());
    if (!bio_md) {
        openssl_log_errors();
        goto error;
    }

    if (!BIO_get_md_ctx(bio_md, &mctx)) {
        LOGE("Failed to get message digest context");
        openssl_log_errors();
        goto error;
    }

    if (!EVP_DigestVerifyInit(mctx, &pctx, md_type, nullptr, pkey)) {
        LOGE("Failed to set message digest context");
        openssl_log_errors();
        goto error;
    }

    buf = (unsigned char *) OPENSSL_malloc(BUFSIZE);
    if (!buf) {
        LOGE("Failed to allocate I/O buffer");
        openssl_log_errors();
        goto error;
    }

    siglen = EVP_PKEY_size(pkey);
    sigbuf = (unsigned char *) OPENSSL_malloc(siglen);
    if (!sigbuf) {
        LOGE("Failed to allocate signature buffer");
        openssl_log_errors();
        goto error;
    }
    siglen = BIO_read(bio_sig_in, sigbuf, siglen);
    if (siglen <= 0) {
        LOGE("Failed to read signature BIO stream");
        openssl_log_errors();
        goto error;
    }

    bio_input = BIO_push(bio_md, bio_data_in);

    while (true) {
        n = BIO_read(bio_input, buf, BUFSIZE);
        if (n < 0) {
            LOGE("Failed to read input data BIO stream");
            openssl_log_errors();
            goto error;
        }
        if (n == 0) {
            break;
        }
    }

    n = EVP_DigestVerifyFinal(mctx, sigbuf, siglen);
    if (n == 1) {
        *result_out = true;
    } else if (n == 0) {
        *result_out = false;
    } else {
        LOGE("Failed to verify data");
        openssl_log_errors();
        goto error;
    }

    BIO_free(bio_md);
    OPENSSL_free(sigbuf);
    OPENSSL_free(buf);
    return true;

error:
    BIO_free(bio_md);
    OPENSSL_free(sigbuf);
    OPENSSL_free(buf);
    return false;
}

}
}
