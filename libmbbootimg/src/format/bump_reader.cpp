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

MB_BEGIN_C_DECLS

/*!
 * \brief Enable support for Bump boot image format
 *
 * \param bir MbBiReader
 *
 * \return
 *   * #MB_BI_OK if the format is successfully enabled
 *   * #MB_BI_WARN if the format is already enabled
 *   * \<= #MB_BI_FAILED if an error occurs
 */
int mb_bi_reader_enable_format_bump(MbBiReader *bir)
{
    AndroidReaderCtx *const ctx = static_cast<AndroidReaderCtx *>(
            calloc(1, sizeof(AndroidReaderCtx)));
    if (!ctx) {
        mb_bi_reader_set_error(bir, -errno,
                               "Failed to allocate AndroidReaderCtx: %s",
                               strerror(errno));
        return MB_BI_FAILED;
    }

    _segment_reader_init(&ctx->segctx);

    ctx->is_bump = true;

    return _mb_bi_reader_register_format(bir,
                                         ctx,
                                         MB_BI_FORMAT_BUMP,
                                         MB_BI_FORMAT_NAME_BUMP,
                                         &bump_reader_bid,
                                         &android_reader_set_option,
                                         &android_reader_read_header,
                                         &android_reader_read_entry,
                                         &android_reader_go_to_entry,
                                         &android_reader_read_data,
                                         &android_reader_free);
}

MB_END_C_DECLS
