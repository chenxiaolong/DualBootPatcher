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

#pragma once

#include "mbcommon/common.h"

#include <memory>

#include <openssl/evp.h>

#include "mbsign/error.h"

namespace mb::sign
{

using ScopedEVP_PKEY = std::unique_ptr<EVP_PKEY, decltype(EVP_PKEY_free) *>;

enum class KeyFormat
{
    Pem,
    Pkcs12,
};

MB_EXPORT Result<ScopedEVP_PKEY>
load_private_key(BIO &bio_key, KeyFormat format, const char *pass);
MB_EXPORT Result<ScopedEVP_PKEY>
load_private_key_from_file(const char *file, KeyFormat format, const char *pass);

MB_EXPORT Result<ScopedEVP_PKEY>
load_public_key(BIO &bio_key, KeyFormat format, const char *pass);
MB_EXPORT Result<ScopedEVP_PKEY>
load_public_key_from_file(const char *file, KeyFormat format, const char *pass);

MB_EXPORT Result<void>
sign_data(BIO &bio_data_in, BIO &bio_sig_out, EVP_PKEY &pkey);
MB_EXPORT Result<void>
verify_data(BIO &bio_data_in, BIO &bio_sig_in, EVP_PKEY &pkey);

}
