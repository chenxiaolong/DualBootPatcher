/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "util/hash.h"

#include <memory>
#include <cstdio>

#include "util/logging.h"

namespace mb
{
namespace util
{

typedef std::unique_ptr<std::FILE, int (*)(std::FILE *)> file_ptr;

/*!
 * \brief Compute SHA1 hash of a file
 *
 * \param path Path to file
 * \param digest `unsigned char` array of size `SHA_DIGEST_SIZE` to store
 *               computed hash value
 *
 * \return true on success, false on failure and errno set appropriately
 */
bool sha1_hash(const std::string &path, unsigned char digest[SHA_DIGEST_SIZE])
{
    file_ptr fp(std::fopen(path.c_str(), "rb"), std::fclose);
    if (!fp) {
        LOGE("%s: Failed to open: %s", path.c_str(), strerror(errno));
        return false;
    }

    unsigned char buf[10240];
    size_t n;

    SHA_CTX ctx;
    SHA_init(&ctx);

    while ((n = fread(buf, 1, sizeof(buf), fp.get())) > 0) {
        SHA_update(&ctx, buf, n);
        if (n < sizeof(buf)) {
            break;
        }
    }

    if (ferror(fp.get())) {
        LOGE("%s: Failed to read file", path.c_str());
        errno = EIO;
        return false;
    }

    memcpy(digest, SHA_final(&ctx), SHA_DIGEST_SIZE);

    return true;
}

}
}
