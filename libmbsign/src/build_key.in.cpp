/*
 * Copyright (C) 2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbsign/build_key.h"

#include <optional>

#include "mbcommon/file/memory.h"

#include "mbsign/sign.h"

namespace mb::sign
{

// Public key specified at build time
static span<const unsigned char> SIGNING_KEY =
    R"(@MBP_SIGN_PUBLIC_KEY@)"_uchars;

static std::optional<PublicKey> g_public_key;

/*!
 * \brief Get public key corresponding to the keypair used to sign the binaries
 *        during the build
 *
 * \note This function will `abort()` if the key is not valid.
 *
 * \warning This function is not thread safe. The public key is initialized the
 *          first time this function is called.
 *
 * \return Public key
 */
const PublicKey & build_key() noexcept
{
    if (!g_public_key) {
        MemoryFile file;
        // TODO: Remove const_cast when MemoryFile supports read-only files
        span s(const_cast<unsigned char *>(SIGNING_KEY.data()),
               SIGNING_KEY.size());

        if (!file.open(s)) {
            abort();
        }

        auto key = load_public_key(file);
        if (!key) {
            abort();
        }

        g_public_key = std::move(key.value());
    }

    return *g_public_key;
}

}
