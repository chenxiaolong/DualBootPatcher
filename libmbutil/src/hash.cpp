/*
 * Copyright (C) 2014-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbutil/hash.h"

#include <memory>

#include <cerrno>
#include <cstdio>

#include "mblog/logging.h"
#include "mbutil/autoclose/file.h"

namespace mb
{
namespace util
{

/*!
 * \brief Compute SHA512 hash of a file
 *
 * \param path Path to file
 * \param digest `unsigned char` array of size `SHA512_DIGEST_LENGTH` to store
 *               computed hash value
 *
 * \return true on success, false on failure and errno set appropriately
 */
bool sha512_hash(const std::string &path,
                 unsigned char digest[SHA512_DIGEST_LENGTH])
{
    autoclose::file fp(autoclose::fopen(path.c_str(), "rb"));
    if (!fp) {
        LOGE("%s: Failed to open: %s", path.c_str(), strerror(errno));
        return false;
    }

    unsigned char buf[10240];
    size_t n;

    SHA512_CTX ctx;
    if (!SHA512_Init(&ctx)) {
        LOGE("openssl: SHA512_Init() failed");
        return false;
    }

    while ((n = fread(buf, 1, sizeof(buf), fp.get())) > 0) {
        if (!SHA512_Update(&ctx, buf, n)) {
            LOGE("openssl: SHA512_Update() failed");
            return false;
        }
        if (n < sizeof(buf)) {
            break;
        }
    }

    if (ferror(fp.get())) {
        LOGE("%s: Failed to read file", path.c_str());
        errno = EIO;
        return false;
    }

    if (!SHA512_Final(digest, &ctx)) {
        LOGE("openssl: SHA512_Final() failed");
        return false;
    }

    return true;
}

}
}
