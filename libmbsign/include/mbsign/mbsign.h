/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#pragma once

#include "mbcommon/common.h"

#include <openssl/evp.h>

namespace mb
{
namespace sign
{

enum {
    KEY_FORMAT_PEM = 1,
    KEY_FORMAT_PKCS12 = 2
};

MB_EXPORT EVP_PKEY * load_private_key(BIO *bio_key, int format,
                                      const char *pass);
MB_EXPORT EVP_PKEY * load_private_key_from_file(const char *file, int format,
                                                const char *pass);
MB_EXPORT EVP_PKEY * load_public_key(BIO *bio_key, int format,
                                     const char *pass);
MB_EXPORT EVP_PKEY * load_public_key_from_file(const char *file, int format,
                                               const char *pass);
MB_EXPORT bool sign_data(BIO *bio_data_in, BIO *bio_sig_out,
                         EVP_PKEY *pkey);
MB_EXPORT bool verify_data(BIO *bio_data_in, BIO *bio_sig_in,
                           EVP_PKEY *pkey, bool *result_out);

}
}
