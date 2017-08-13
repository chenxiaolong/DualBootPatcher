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
 *   * #RET_OK if the format is successfully enabled
 *   * #RET_WARN if the format is already enabled
 *   * \<= #RET_FAILED if an error occurs
 */
int writer_set_format_bump(MbBiWriter *biw)
{
    using namespace android;

    AndroidWriterCtx *const ctx = new AndroidWriterCtx();

    if (!SHA1_Init(&ctx->sha_ctx)) {
        writer_set_error(biw, ERROR_INTERNAL_ERROR,
                         "Failed to initialize SHA_CTX");
        delete ctx;
        return false;
    }

    ctx->is_bump = true;

    return _writer_register_format(biw,
                                   ctx,
                                   FORMAT_BUMP,
                                   FORMAT_NAME_BUMP,
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

}
}
