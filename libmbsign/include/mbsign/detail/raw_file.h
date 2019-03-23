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

#pragma once

#include "mbcommon/span.h"

#include "mbsign/sign.h"

namespace mb::sign::detail
{

oc::result<void>
save_raw_file(File &file, span<const unsigned char> payload,
              const UntrustedComment &untrusted,
              const TrustedComment * const trusted,
              const RawSignature * const global_sig) noexcept;
oc::result<void>
load_raw_file(File &file, span<unsigned char> payload,
              UntrustedComment &untrusted, TrustedComment *trusted,
              RawSignature *global_sig) noexcept;

}
