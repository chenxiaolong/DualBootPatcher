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

#pragma once

#include "mbcommon/guard_p.h"

#include "mbcommon/file/callbacks.h"
#include "mbcommon/file_p.h"

/*! \cond INTERNAL */
namespace mb
{

class CallbackFilePrivate : public FilePrivate
{
public:
    CallbackFilePrivate();
    virtual ~CallbackFilePrivate();

    void clear();

    CallbackFile::OpenCb open_cb;
    CallbackFile::CloseCb close_cb;
    CallbackFile::ReadCb read_cb;
    CallbackFile::WriteCb write_cb;
    CallbackFile::SeekCb seek_cb;
    CallbackFile::TruncateCb truncate_cb;
    void *userdata;
};

}
/*! \endcond */
