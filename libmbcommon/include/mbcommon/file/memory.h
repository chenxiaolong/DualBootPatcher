/*
 * Copyright (C) 2017-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/file.h"

namespace mb
{

class MB_EXPORT MemoryFile : public File
{
public:
    MemoryFile();
    MemoryFile(span<std::byte> buf);
    MemoryFile(void **buf_ptr, size_t *size_ptr);
    virtual ~MemoryFile();

    MemoryFile(MemoryFile &&other) noexcept;
    MemoryFile & operator=(MemoryFile &&rhs) noexcept;

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(MemoryFile)

    oc::result<void> open(span<std::byte> buf);
    oc::result<void> open(void **buf_ptr, size_t *size_ptr);

    oc::result<void> close() override;

    oc::result<size_t> read(span<std::byte> buf) override;
    oc::result<size_t> write(span<const std::byte> buf) override;
    oc::result<uint64_t> seek(int64_t offset, int whence) override;
    oc::result<void> truncate(uint64_t size) override;

    bool is_open() override;

private:
    /*! \cond INTERNAL */
    void clear() noexcept;

    bool m_is_open;

    span<std::byte> m_data;

    void **m_data_ptr;
    size_t *m_size_ptr;

    size_t m_pos;

    bool m_fixed_size;
    /*! \endcond */
};

}
