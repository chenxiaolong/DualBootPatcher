/*
 * Copyright (C) 2016-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <memory>
#include <string>

#include <cstddef>
#include <cstdint>

#include "mbcommon/file_error.h"
#include "mbcommon/file_p.h"
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
    oc::result<void> close();

    // File operations
    oc::result<size_t> read(void *buf, size_t size);
    oc::result<size_t> write(const void *buf, size_t size);
    oc::result<uint64_t> seek(int64_t offset, int whence);
    oc::result<void> truncate(uint64_t size);

    // File state
    bool is_open();

protected:
    File(File &&other) noexcept;
    File & operator=(File &&rhs) noexcept;

    // File open
    oc::result<void> open();

    detail::FileState state();
    void set_state(detail::FileState state);

    virtual oc::result<void> on_open();
    virtual oc::result<void> on_close();
    virtual oc::result<size_t> on_read(void *buf, size_t size);
    virtual oc::result<size_t> on_write(const void *buf, size_t size);
    virtual oc::result<uint64_t> on_seek(int64_t offset, int whence);
    virtual oc::result<void> on_truncate(uint64_t size);

private:
    /*! \cond INTERNAL */
    detail::FileState m_state;
    /*! \endcond */
};

}
