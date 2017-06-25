/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/guard_p.h"

#include "mbcommon/file.h"

/*! \cond INTERNAL */
enum MbFileState : uint16_t
{
    NEW             = 1U << 1,
    OPENED          = 1U << 2,
    CLOSED          = 1U << 3,
    FATAL           = 1U << 4,
    // Grouped
    ANY_NONFATAL    = NEW | OPENED | CLOSED,
    ANY             = ANY_NONFATAL | FATAL,
};

struct MbFile
{
    uint16_t state;

    // Callbacks
    MbFileOpenCb open_cb;
    MbFileCloseCb close_cb;
    MbFileReadCb read_cb;
    MbFileWriteCb write_cb;
    MbFileSeekCb seek_cb;
    MbFileTruncateCb truncate_cb;
    void *cb_userdata;

    // Error
    int error_code;
    char *error_string;
};
/*! \endcond */
