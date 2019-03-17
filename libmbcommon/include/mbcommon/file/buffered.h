/*
 * Copyright (C) 2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <vector>

#include "mbcommon/file.h"
#include "mbcommon/span.h"

namespace mb
{

class MB_EXPORT BufferedFile : public File
{
public:
    static inline size_t DEFAULT_BUFFER_SIZE = 8 * 1024;

    BufferedFile(size_t buf_size = DEFAULT_BUFFER_SIZE);
    BufferedFile(File &file, size_t buf_size = DEFAULT_BUFFER_SIZE);
    virtual ~BufferedFile();

    BufferedFile(BufferedFile &&other) noexcept;
    BufferedFile & operator=(BufferedFile &&rhs) noexcept;

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(BufferedFile)

    oc::result<void> open(File &file);

    oc::result<void> close() override;

    oc::result<size_t> read(void *buf, size_t size) override;
    oc::result<size_t> write(const void *buf, size_t size) override;
    oc::result<uint64_t> seek(int64_t offset, int whence) override;
    oc::result<void> truncate(uint64_t size) override;

    bool is_open() override;

    oc::result<void> sync_pos();
    oc::result<void> flush();

    bool set_buf_size(size_t size);
    oc::result<void> fill_rbuf();
    void consume_rbuf(size_t n);
    span<unsigned char> rbuf();
    span<unsigned char> wbuf();

    // TODO: read_until
    // TODO: read_line
    // TODO: read_delim

private:
    /*! \cond INTERNAL */
    oc::result<size_t> read_underlying(void *buf, size_t size);
    oc::result<size_t> write_underlying(const void *buf, size_t size);
    oc::result<void> write_exact_underlying(const void *buf, size_t size);
    oc::result<uint64_t> seek_underlying(int64_t offset, int whence);

    void clear() noexcept;

    File *m_file;
    uint64_t m_fpos;
    std::vector<unsigned char> m_rbuf;
    size_t m_rpos;
    size_t m_rcap;
    std::vector<unsigned char> m_wbuf;
    /*! \endcond */
};

}
