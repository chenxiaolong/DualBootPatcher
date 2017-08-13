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

#include "mbbootimg/format/android_reader_p.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>

#include "mbbootimg/reader_p.h"

namespace mb
{
namespace bootimg
{

/*!
 * \brief Enable support for Bump boot image format
 *
 * \param bir MbBiReader
 *
 * \return
 *   * #RET_OK if the format is successfully enabled
 *   * #RET_WARN if the format is already enabled
 *   * \<= #RET_FAILED if an error occurs
 */
int mb_bi_reader_enable_format_bump(MbBiReader *bir)
{
    using namespace android;

    AndroidReaderCtx *const ctx = new AndroidReaderCtx();

    ctx->is_bump = true;

    return _mb_bi_reader_register_format(bir,
                                         ctx,
                                         FORMAT_BUMP,
                                         FORMAT_NAME_BUMP,
                                         &bump_reader_bid,
                                         &android_reader_set_option,
                                         &android_reader_read_header,
                                         &android_reader_read_entry,
                                         &android_reader_go_to_entry,
                                         &android_reader_read_data,
                                         &android_reader_free);
}

}
}
