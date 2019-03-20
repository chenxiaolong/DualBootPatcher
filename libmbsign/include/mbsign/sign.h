/*
 * Copyright (C) 2016-2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/file.h"

#include "mbsign/defs.h"
#include "mbsign/memory.h"

namespace mb::sign
{

MB_EXPORT bool
initialize() noexcept;

MB_EXPORT oc::result<KeyPair>
generate_keypair() noexcept;

MB_EXPORT oc::result<void>
save_secret_key(File &file, const SecretKey &key, const char *passphrase,
                KdfSecurityLevel kdf_sec = KdfSecurityLevel::Sensitive) noexcept;
MB_EXPORT oc::result<SecretKey>
load_secret_key(File &file, const char *passphrase) noexcept;

MB_EXPORT oc::result<void>
save_public_key(File &file, const PublicKey &key) noexcept;
MB_EXPORT oc::result<PublicKey>
load_public_key(File &file) noexcept;

MB_EXPORT oc::result<void>
save_signature(File &file, const Signature &sig) noexcept;
MB_EXPORT oc::result<Signature>
load_signature(File &file) noexcept;

MB_EXPORT oc::result<Signature>
sign_file(File &file, const SecretKey &key) noexcept;
MB_EXPORT oc::result<void>
verify_file(File &file, const Signature &sig, const PublicKey &key) noexcept;

// Helper functions

MB_EXPORT oc::result<void>
update_global_signature(Signature &sig, const SecretKey &key) noexcept;
MB_EXPORT oc::result<void>
verify_global_signature(const Signature &sig, const PublicKey &key) noexcept;

}
