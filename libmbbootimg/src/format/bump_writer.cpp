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

#include <cerrno>
#include <cstdlib>
#include <cstring>

#include "mbbootimg/writer_p.h"

MB_BEGIN_C_DECLS

/*!
 * \brief Set Bump boot image output format
 *
 * \param biw MbBiWriter
 *
 * \return
 *   * #MB_BI_OK if the format is successfully enabled
 *   * #MB_BI_WARN if the format is already enabled
 *   * \<= #MB_BI_FAILED if an error occurs
 */
int mb_bi_writer_set_format_bump(MbBiWriter *biw)
{
    AndroidWriterCtx *const ctx = static_cast<AndroidWriterCtx *>(
            calloc(1, sizeof(AndroidWriterCtx)));
    if (!ctx) {
        mb_bi_writer_set_error(biw, -errno,
                               "Failed to allocate AndroidWriterCtx: %s",
                               strerror(errno));
        return MB_BI_FAILED;
    }

    if (!SHA1_Init(&ctx->sha_ctx)) {
        mb_bi_writer_set_error(biw, MB_BI_ERROR_INTERNAL_ERROR,
                               "Failed to initialize SHA_CTX");
        free(ctx);
        return false;
    }

    _segment_writer_init(&ctx->segctx);

    ctx->is_bump = true;

    return _mb_bi_writer_register_format(biw,
                                         ctx,
                                         MB_BI_FORMAT_BUMP,
                                         MB_BI_FORMAT_NAME_BUMP,
                                         nullptr,
                                         &android_writer_get_header,
                                         &android_writer_write_header,
                                         &android_writer_get_entry,
                                         &android_writer_write_entry,
                                         &android_writer_write_data,
                                         &android_writer_finish_entry,
                                         &android_writer_close,
                                         &android_writer_free);
}


MB_END_C_DECLS
