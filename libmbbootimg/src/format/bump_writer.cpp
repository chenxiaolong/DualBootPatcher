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
 * \param biw MbBiWriter
 *
 * \return
 *   * #RET_OK if the format is successfully set
 *   * \<= #RET_WARN if an error occurs
 */
int writer_set_format_bump(MbBiWriter *biw)
{
    using namespace android;

    std::unique_ptr<FormatWriter> format{new AndroidFormatWriter(biw, true)};
    return _writer_register_format(biw, std::move(format));
}

}
}
