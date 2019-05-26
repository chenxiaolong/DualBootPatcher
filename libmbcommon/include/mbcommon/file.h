/*
 * Copyright (C) 2016-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <cstddef>
#include <cstdint>

#include "mbcommon/outcome.h"

namespace mb
{

class MB_EXPORT File
{
public:
    File();
    virtual ~File();

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(File)

    // File close
    virtual oc::result<void> close() = 0;

    // File operations
    virtual oc::result<size_t> read(void *buf, size_t size) = 0;
    virtual oc::result<size_t> write(const void *buf, size_t size) = 0;
    virtual oc::result<uint64_t> seek(int64_t offset, int whence) = 0;
    virtual oc::result<void> truncate(uint64_t size) = 0;

    // File state
    virtual bool is_open() = 0;
};

}
