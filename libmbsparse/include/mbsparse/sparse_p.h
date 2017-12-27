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

#include <cstdint>

namespace mb::sparse::detail
{

/*! \cond INTERNAL */

constexpr uint32_t SPARSE_HEADER_MAGIC =    0xed26ff3a;
constexpr int SPARSE_HEADER_MAJOR_VER =     1;

constexpr uint16_t CHUNK_TYPE_RAW =         0xCAC1;
constexpr uint16_t CHUNK_TYPE_FILL =        0xCAC2;
constexpr uint16_t CHUNK_TYPE_DONT_CARE =   0xCAC3;
constexpr uint16_t CHUNK_TYPE_CRC32 =       0xCAC4;

struct SparseHeader
{
    uint32_t magic;          // 0xed26ff3a
    uint16_t major_version;  // (0x1) - reject images with higher major versions
    uint16_t minor_version;  // (0x0) - allow images with higer minor versions
    uint16_t file_hdr_sz;    // 28 bytes for first revision of the file format
    uint16_t chunk_hdr_sz;   // 12 bytes for first revision of the file format
    uint32_t blk_sz;         // block size in bytes, must be a multiple of 4 (4096)
    uint32_t total_blks;     // total blocks in the non-sparse output image
    uint32_t total_chunks;   // total chunks in the sparse input image
    uint32_t image_checksum; // CRC32 checksum of the original data, counting "don't care"
                             // as 0. Standard 802.3 polynomial, use a Public Domain
                             // table implementation
};

struct ChunkHeader
{
    uint16_t chunk_type;     // 0xCAC1 -> raw; 0xCAC2 -> fill; 0xCAC3 -> don't care
    uint16_t reserved1;
    uint32_t chunk_sz;       // in blocks in output image
    uint32_t total_sz;       // in bytes of chunk input file including chunk header and data
};

/*
 * Following a Raw or Fill or CRC32 chunk is data.
 * - For a Raw chunk, it's the data in chunk_sz * blk_sz.
 * - For a Fill chunk, it's 4 bytes of the fill data.
 * - For a CRC32 chunk, it's 4 bytes of CRC32
 */

/*! \brief Minimum information we need from the chunk headers while reading */
struct ChunkInfo
{
    /*! \brief Same as ChunkHeader::chunk_type */
    uint16_t type;

    /*! \brief Start of byte range in output file that this chunk represents */
    uint64_t begin;
    /*! \brief End of byte range in output file that this chunk represents */
    uint64_t end;

    /*! \brief Start of byte range for the entire chunk in the source file */
    uint64_t src_begin;
    /*! \brief End of byte range for the entire chunk in the source file */
    uint64_t src_end;

    /*! \brief [CHUNK_TYPE_RAW only] Start of raw bytes in input file */
    uint64_t raw_begin;
    /*! \brief [CHUNK_TYPE_RAW only] End of raw bytes in input file */
    uint64_t raw_end;

    /*! \brief [CHUNK_TYPE_FILL only] Filler value for the chunk */
    uint32_t fill_val;
};

enum class Seekability : uint8_t
{
    CanSeek,
    CanSkip,
    CanRead,
};

/*! \endcond */

}
