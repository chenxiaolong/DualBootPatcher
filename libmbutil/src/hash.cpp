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
#include <cstring>

#include "mblog/logging.h"

#define LOG_TAG "mbutil/hash"

namespace mb::util
{

using ScopedFILE = std::unique_ptr<FILE, decltype(fclose) *>;

/*!
 * \brief Compute SHA512 hash of a file
 *
 * \param path Path to file
 * \param digest Byte array to store computed hash value
 *
 * \return true on success, false on failure and errno set appropriately
 */
bool sha512_hash(const std::string &path, Sha512Digest &digest)
{
    ScopedFILE fp(fopen(path.c_str(), "rb"), fclose);
    if (!fp) {
        LOGE("%s: Failed to open: %s", path.c_str(), strerror(errno));
        return false;
    }

    std::array<unsigned char, 10240> buf;
    size_t n;

    SHA512_CTX ctx;
    if (!SHA512_Init(&ctx)) {
        LOGE("openssl: SHA512_Init() failed");
        return false;
    }

    while ((n = fread(buf.data(), 1, buf.size(), fp.get())) > 0) {
        if (!SHA512_Update(&ctx, buf.data(), n)) {
            LOGE("openssl: SHA512_Update() failed");
            return false;
        }
        if (n < buf.size()) {
            break;
        }
    }

    if (ferror(fp.get())) {
        LOGE("%s: Failed to read file", path.c_str());
        errno = EIO;
        return false;
    }

    if (!SHA512_Final(digest.data(), &ctx)) {
        LOGE("openssl: SHA512_Final() failed");
        return false;
    }

    return true;
}

}
