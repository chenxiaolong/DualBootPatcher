/*
 * Copyright (C) 2015-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
 * Copyright (C) 2010 The Android Open Source Project
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

#include "mbsparse/sparse_p.h"

namespace mb::sparse
{

class MB_EXPORT SparseFile : public File
{
public:
    SparseFile();
    SparseFile(File *file);
    virtual ~SparseFile();

    SparseFile(SparseFile &&other) noexcept;
    SparseFile & operator=(SparseFile &&rhs) noexcept;

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(SparseFile)

    // File open
    oc::result<void> open(File *file);

    oc::result<void> close() override;

    oc::result<size_t> read(void *buf, size_t size) override;
    oc::result<size_t> write(const void *buf, size_t size) override;
    oc::result<uint64_t> seek(int64_t offset, int whence) override;
    oc::result<void> truncate(uint64_t size) override;

    bool is_open() override;

    // File size
    uint64_t size() noexcept;

private:
    void clear() noexcept;

    oc::result<void> wread(void *buf, size_t size) noexcept;
    oc::result<void> wseek(int64_t offset) noexcept;
    oc::result<void> skip_bytes(uint64_t bytes) noexcept;

    oc::result<void>
    process_sparse_header(const void *preread_data, size_t preread_size)
        noexcept;

    oc::result<detail::ChunkInfo>
    process_raw_chunk(const detail::ChunkHeader &chdr, uint64_t tgt_offset)
        noexcept;
    oc::result<detail::ChunkInfo>
    process_fill_chunk(const detail::ChunkHeader &chdr, uint64_t tgt_offset)
        noexcept;
    oc::result<detail::ChunkInfo>
    process_skip_chunk(const detail::ChunkHeader &chdr, uint64_t tgt_offset)
        noexcept;
    oc::result<detail::ChunkInfo>
    process_crc32_chunk(const detail::ChunkHeader &chdr, uint64_t tgt_offset)
        noexcept;
    oc::result<detail::ChunkInfo>
    process_chunk(const detail::ChunkHeader &chdr, uint64_t tgt_offset)
        noexcept;

    oc::result<void> move_to_chunk(uint64_t offset) noexcept;

    File *m_file;
    detail::Seekability m_seekability;

    // Expected CRC32 checksum. We currently do *not* validate this. It would
    // only work if the entire file was read sequentially anyway.
    uint32_t m_expected_crc32;
    // Relative offset in input file
    uint64_t m_cur_src_offset;
    // Absolute offset in output file
    uint64_t m_cur_tgt_offset;
    // Expected file size
    uint64_t m_file_size;

    detail::SparseHeader m_shdr;

    std::vector<detail::ChunkInfo> m_chunks;
    decltype(m_chunks)::iterator m_chunk;
};

}
