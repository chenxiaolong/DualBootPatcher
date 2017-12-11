/*
 * Copyright (C) 2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbbootimg/format/android_writer_p.h"

#include "mbbootimg/writer_p.h"

namespace mb
{
namespace bootimg
{

/*!
 * \brief Set Bump boot image output format
 *
 * \return Nothing if the format is successfully set. Otherwise, the error code.
 */
oc::result<void> Writer::set_format_bump()
{
    return register_format(
            std::make_unique<android::AndroidFormatWriter>(*this, true));
}

}
}
