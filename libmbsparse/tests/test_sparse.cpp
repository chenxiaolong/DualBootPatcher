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

#include <gtest/gtest.h>

#include "mbsparse/sparse.h"

#include "mbcommon/endian.h"
#include "mbcommon/file/memory.h"
#include "mbcommon/file_error.h"

#include "mbsparse/sparse_error.h"

using namespace mb;
using namespace mb::sparse;
using namespace mb::sparse::detail;

class CustomMemoryFile : public MemoryFile
{
public:
    void set_seekability(Seekability seekability)
    {
        _seekability = seekability;
    }

    oc::result<uint64_t> seek(int64_t offset, int whence) override
    {
        switch (_seekability) {
        case Seekability::CanSeek:
            return MemoryFile::seek(offset, whence);
        case Seekability::CanSkip:
            if (whence == SEEK_CUR && offset >= 0) {
                return MemoryFile::seek(offset, whence);
            }
            break;
        case Seekability::CanRead:
            break;
        default:
            MB_UNREACHABLE("Invalid seekability");
        }

        return FileError::UnsupportedSeek;
    }

private:
    Seekability _seekability = Seekability::CanSeek;
};

struct SparseTest : testing::Test
{
    CustomMemoryFile _source_file;
    SparseFile _file;
    void *_data = nullptr;
    size_t _size = 0;

    static constexpr unsigned char expected_valid_data[] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'a', 'b', 'c', 'd', 'e', 'f',
        0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12,
        0x78, 0x56, 0x34, 0x12,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    virtual ~SparseTest()
    {
        free(_data);
    }

    void SetUp() override
    {
        ASSERT_TRUE(_source_file.open(&_data, &_size));
    }

    void build_valid_data(bool oversized)
    {
        SparseHeader shdr = {};
        shdr.magic = SPARSE_HEADER_MAGIC;
        shdr.major_version = SPARSE_HEADER_MAJOR_VER;
        shdr.minor_version = 0;
        shdr.file_hdr_sz = sizeof(SparseHeader) + (oversized ? 4 : 0);
        shdr.chunk_hdr_sz = sizeof(ChunkHeader) + (oversized ? 4 : 0);
        shdr.blk_sz = 4;
        shdr.total_blks = 12;
        shdr.total_chunks = 4;
        shdr.image_checksum = 0;
        fix_sparse_header_byte_order(shdr);

        ASSERT_TRUE(_source_file.write(&shdr, sizeof(shdr)));
        if (oversized) {
            ASSERT_TRUE(_source_file.write("\xaa\xbb\xcc\xdd", 4));
        }

        ChunkHeader chdr;

        // [1/4] Add raw chunk header
        chdr = {};
        chdr.chunk_type = CHUNK_TYPE_RAW;
        chdr.chunk_sz = 4; // 16 bytes
        chdr.total_sz = shdr.chunk_hdr_sz + chdr.chunk_sz * shdr.blk_sz;
        fix_chunk_header_byte_order(chdr);

        ASSERT_TRUE(_source_file.write(&chdr, sizeof(chdr)));
        if (oversized) {
            ASSERT_TRUE(_source_file.write("\xaa\xbb\xcc\xdd", 4));
        }
        ASSERT_TRUE(_source_file.write("0123456789abcdef", 16));

        // [2/4] Add fill chunk header
        chdr = {};
        chdr.chunk_type = CHUNK_TYPE_FILL;
        chdr.chunk_sz = 4; // 16 bytes
        chdr.total_sz = static_cast<uint32_t>(shdr.chunk_hdr_sz)
                + static_cast<uint32_t>(sizeof(uint32_t));
        fix_chunk_header_byte_order(chdr);

        ASSERT_TRUE(_source_file.write(&chdr, sizeof(chdr)));
        if (oversized) {
            ASSERT_TRUE(_source_file.write("\xaa\xbb\xcc\xdd", 4));
        }

        uint32_t fill_val = mb_htole32(0x12345678);
        ASSERT_TRUE(_source_file.write(&fill_val, sizeof(fill_val)));

        // [3/4] Add skip chunk header
        chdr = {};
        chdr.chunk_type = CHUNK_TYPE_DONT_CARE;
        chdr.chunk_sz = 4; // 16 bytes
        chdr.total_sz = shdr.chunk_hdr_sz;
        fix_chunk_header_byte_order(chdr);

        ASSERT_TRUE(_source_file.write(&chdr, sizeof(chdr)));
        if (oversized) {
            ASSERT_TRUE(_source_file.write("\xaa\xbb\xcc\xdd", 4));
        }

        // [4/4] Add CRC32 chunk header
        chdr = {};
        chdr.chunk_type = CHUNK_TYPE_CRC32;
        chdr.chunk_sz = 0;
        chdr.total_sz = static_cast<uint32_t>(shdr.chunk_hdr_sz)
                + static_cast<uint16_t>(sizeof(uint32_t));
        fix_chunk_header_byte_order(chdr);

        ASSERT_TRUE(_source_file.write(&chdr, sizeof(chdr)));
        if (oversized) {
            ASSERT_TRUE(_source_file.write("\xaa\xbb\xcc\xdd", 4));
        }
        ASSERT_TRUE(_source_file.write("\x00\x00\x00\x00", 4));

        // Move back to beginning of the file
        ASSERT_TRUE(_source_file.seek(0, SEEK_SET));
    }

    static void fix_sparse_header_byte_order(SparseHeader &header) noexcept
    {
        header.magic = mb_htole32(header.magic);
        header.major_version = mb_htole16(header.major_version);
        header.minor_version = mb_htole16(header.minor_version);
        header.file_hdr_sz = mb_htole16(header.file_hdr_sz);
        header.chunk_hdr_sz = mb_htole16(header.chunk_hdr_sz);
        header.blk_sz = mb_htole32(header.blk_sz);
        header.total_blks = mb_htole32(header.total_blks);
        header.total_chunks = mb_htole32(header.total_chunks);
        header.image_checksum = mb_htole32(header.image_checksum);
    }

    static void fix_chunk_header_byte_order(ChunkHeader &header) noexcept
    {
        header.chunk_type = mb_htole16(header.chunk_type);
        header.reserved1 = mb_htole16(header.reserved1);
        header.chunk_sz = mb_htole32(header.chunk_sz);
        header.total_sz = mb_htole32(header.total_sz);
    }
};

constexpr unsigned char SparseTest::expected_valid_data[];

TEST_F(SparseTest, CheckInvalidStates)
{
    auto error = oc::failure(FileError::InvalidState);

    ASSERT_EQ(_file.close(), error);
    ASSERT_EQ(_file.read(nullptr, 0), error);
    ASSERT_EQ(_file.seek(0, SEEK_SET), error);

    build_valid_data(true);

    ASSERT_TRUE(_file.open(&_source_file));
    ASSERT_EQ(_file.open(&_source_file), error);
}

TEST_F(SparseTest, CheckUnsupportedWriteTruncate)
{
    ASSERT_EQ(_file.write(nullptr, 0),
              oc::failure(FileError::UnsupportedWrite));
    ASSERT_EQ(_file.truncate(1024),
              oc::failure(FileError::UnsupportedTruncate));
}

TEST_F(SparseTest, CheckOpeningUnopenedFileFails)
{
    ASSERT_TRUE(_source_file.close());
    ASSERT_EQ(_file.open(&_source_file), oc::failure(FileError::InvalidState));
}

TEST_F(SparseTest, CheckSparseHeaderInvalidMagicFailure)
{
    SparseHeader shdr = {};
    shdr.magic = 0xaabbccdd;
    shdr.major_version = SPARSE_HEADER_MAJOR_VER;
    shdr.minor_version = 0;
    shdr.file_hdr_sz = sizeof(SparseHeader);
    shdr.chunk_hdr_sz = sizeof(ChunkHeader);
    shdr.blk_sz = 4096;
    shdr.total_blks = 1;
    shdr.total_chunks = 0;
    shdr.image_checksum = 0;
    fix_sparse_header_byte_order(shdr);

    ASSERT_TRUE(_source_file.write(&shdr, sizeof(shdr)));
    ASSERT_TRUE(_source_file.truncate(shdr.total_blks * shdr.blk_sz));
    ASSERT_TRUE(_source_file.seek(0, SEEK_SET));

    ASSERT_EQ(_file.open(&_source_file),
              oc::failure(SparseFileError::InvalidSparseMagic));
}

TEST_F(SparseTest, CheckSparseHeaderInvalidMajorVersionFailure)
{
    SparseHeader shdr = {};
    shdr.magic = SPARSE_HEADER_MAGIC;
    shdr.major_version = 0xaabb;
    shdr.minor_version = 0;
    shdr.file_hdr_sz = sizeof(SparseHeader);
    shdr.chunk_hdr_sz = sizeof(ChunkHeader);
    shdr.blk_sz = 4096;
    shdr.total_blks = 1;
    shdr.total_chunks = 0;
    shdr.image_checksum = 0;
    fix_sparse_header_byte_order(shdr);

    ASSERT_TRUE(_source_file.write(&shdr, sizeof(shdr)));
    ASSERT_TRUE(_source_file.truncate(shdr.total_blks * shdr.blk_sz));
    ASSERT_TRUE(_source_file.seek(0, SEEK_SET));

    ASSERT_EQ(_file.open(&_source_file),
              oc::failure(SparseFileError::InvalidSparseMajorVersion));
}

TEST_F(SparseTest, CheckSparseHeaderInvalidMinorVersionOk)
{
    SparseHeader shdr = {};
    shdr.magic = SPARSE_HEADER_MAGIC;
    shdr.major_version = SPARSE_HEADER_MAJOR_VER;
    shdr.minor_version = 0xaabb;
    shdr.file_hdr_sz = sizeof(SparseHeader);
    shdr.chunk_hdr_sz = sizeof(ChunkHeader);
    shdr.blk_sz = 4096;
    shdr.total_blks = 1;
    shdr.total_chunks = 0;
    shdr.image_checksum = 0;
    fix_sparse_header_byte_order(shdr);

    ASSERT_TRUE(_source_file.write(&shdr, sizeof(shdr)));
    ASSERT_TRUE(_source_file.truncate(shdr.total_blks * shdr.blk_sz));
    ASSERT_TRUE(_source_file.seek(0, SEEK_SET));

    ASSERT_TRUE(_file.open(&_source_file));
}

TEST_F(SparseTest, CheckSparseHeaderUndersizedSparseHeaderSizeFailure)
{
    SparseHeader shdr = {};
    shdr.magic = SPARSE_HEADER_MAGIC;
    shdr.major_version = SPARSE_HEADER_MAJOR_VER;
    shdr.minor_version = 0;
    shdr.file_hdr_sz = sizeof(SparseHeader) - 1;
    shdr.chunk_hdr_sz = sizeof(ChunkHeader);
    shdr.blk_sz = 4096;
    shdr.total_blks = 1;
    shdr.total_chunks = 0;
    shdr.image_checksum = 0;
    fix_sparse_header_byte_order(shdr);

    ASSERT_TRUE(_source_file.write(&shdr, sizeof(shdr)));
    ASSERT_TRUE(_source_file.truncate(shdr.total_blks * shdr.blk_sz));
    ASSERT_TRUE(_source_file.seek(0, SEEK_SET));

    ASSERT_EQ(_file.open(&_source_file),
              oc::failure(SparseFileError::InvalidSparseHeaderSize));
}

TEST_F(SparseTest, CheckSparseHeaderUndersizedChunkHeaderSizeFailure)
{
    SparseHeader shdr = {};
    shdr.magic = SPARSE_HEADER_MAGIC;
    shdr.major_version = SPARSE_HEADER_MAJOR_VER;
    shdr.minor_version = 0;
    shdr.file_hdr_sz = sizeof(SparseHeader);
    shdr.chunk_hdr_sz = sizeof(ChunkHeader) - 1;
    shdr.blk_sz = 4096;
    shdr.total_blks = 1;
    shdr.total_chunks = 0;
    shdr.image_checksum = 0;
    fix_sparse_header_byte_order(shdr);

    ASSERT_TRUE(_source_file.write(&shdr, sizeof(shdr)));
    ASSERT_TRUE(_source_file.truncate(shdr.total_blks * shdr.blk_sz));
    ASSERT_TRUE(_source_file.seek(0, SEEK_SET));

    ASSERT_EQ(_file.open(&_source_file),
              oc::failure(SparseFileError::InvalidChunkHeaderSize));
}

TEST_F(SparseTest, CheckInvalidRawChunkFailure)
{
    SparseHeader shdr = {};
    shdr.magic = SPARSE_HEADER_MAGIC;
    shdr.major_version = SPARSE_HEADER_MAJOR_VER;
    shdr.minor_version = 0;
    shdr.file_hdr_sz = sizeof(SparseHeader);
    shdr.chunk_hdr_sz = sizeof(ChunkHeader);
    shdr.blk_sz = 4;
    shdr.total_blks = 1;
    shdr.total_chunks = 1;
    shdr.image_checksum = 0;
    fix_sparse_header_byte_order(shdr);

    char buf[1024];

    ASSERT_TRUE(_source_file.write(&shdr, sizeof(shdr)));

    ChunkHeader chdr = {};
    chdr.chunk_type = CHUNK_TYPE_RAW;
    chdr.chunk_sz = 1; // 4 bytes
    chdr.total_sz = shdr.chunk_hdr_sz + chdr.chunk_sz * shdr.blk_sz + 1;
    fix_chunk_header_byte_order(chdr);

    ASSERT_TRUE(_source_file.write(&chdr, sizeof(chdr)));
    ASSERT_TRUE(_source_file.write("1234", 4));
    ASSERT_TRUE(_source_file.seek(0, SEEK_SET));

    ASSERT_TRUE(_file.open(&_source_file));
    ASSERT_EQ(_file.read(buf, sizeof(buf)),
              oc::failure(SparseFileError::InvalidRawChunk));
}

TEST_F(SparseTest, CheckInvalidFillChunkFailure)
{
    SparseHeader shdr = {};
    shdr.magic = SPARSE_HEADER_MAGIC;
    shdr.major_version = SPARSE_HEADER_MAJOR_VER;
    shdr.minor_version = 0;
    shdr.file_hdr_sz = sizeof(SparseHeader);
    shdr.chunk_hdr_sz = sizeof(ChunkHeader);
    shdr.blk_sz = 4;
    shdr.total_blks = 1;
    shdr.total_chunks = 1;
    shdr.image_checksum = 0;
    fix_sparse_header_byte_order(shdr);

    char buf[1024];

    ASSERT_TRUE(_source_file.write(&shdr, sizeof(shdr)));

    ChunkHeader chdr = {};
    chdr.chunk_type = CHUNK_TYPE_FILL;
    chdr.chunk_sz = 1; // 4 bytes
    chdr.total_sz = shdr.chunk_hdr_sz + 0;
    fix_chunk_header_byte_order(chdr);

    ASSERT_TRUE(_source_file.write(&chdr, sizeof(chdr)));
    ASSERT_TRUE(_source_file.seek(0, SEEK_SET));

    ASSERT_TRUE(_file.open(&_source_file));
    ASSERT_EQ(_file.read(buf, sizeof(buf)),
              oc::failure(SparseFileError::InvalidFillChunk));
}

TEST_F(SparseTest, CheckInvalidSkipChunkFailure)
{
    SparseHeader shdr = {};
    shdr.magic = SPARSE_HEADER_MAGIC;
    shdr.major_version = SPARSE_HEADER_MAJOR_VER;
    shdr.minor_version = 0;
    shdr.file_hdr_sz = sizeof(SparseHeader);
    shdr.chunk_hdr_sz = sizeof(ChunkHeader);
    shdr.blk_sz = 4;
    shdr.total_blks = 1;
    shdr.total_chunks = 1;
    shdr.image_checksum = 0;
    fix_sparse_header_byte_order(shdr);

    char buf[1024];

    ASSERT_TRUE(_source_file.write(&shdr, sizeof(shdr)));

    ChunkHeader chdr = {};
    chdr.chunk_type = CHUNK_TYPE_DONT_CARE;
    chdr.chunk_sz = 1; // 4 bytes
    chdr.total_sz = shdr.chunk_hdr_sz + 1u;
    fix_chunk_header_byte_order(chdr);

    ASSERT_TRUE(_source_file.write(&chdr, sizeof(chdr)));
    ASSERT_TRUE(_source_file.seek(0, SEEK_SET));

    ASSERT_TRUE(_file.open(&_source_file));
    ASSERT_EQ(_file.read(buf, sizeof(buf)),
              oc::failure(SparseFileError::InvalidSkipChunk));
}

TEST_F(SparseTest, CheckInvalidCrc32ChunkFailure)
{
    SparseHeader shdr = {};
    shdr.magic = SPARSE_HEADER_MAGIC;
    shdr.major_version = SPARSE_HEADER_MAJOR_VER;
    shdr.minor_version = 0;
    shdr.file_hdr_sz = sizeof(SparseHeader);
    shdr.chunk_hdr_sz = sizeof(ChunkHeader);
    shdr.blk_sz = 4;
    shdr.total_blks = 1;
    shdr.total_chunks = 1;
    shdr.image_checksum = 0;
    fix_sparse_header_byte_order(shdr);

    char buf[1024];

    ASSERT_TRUE(_source_file.write(&shdr, sizeof(shdr)));

    ChunkHeader chdr = {};
    chdr.chunk_type = CHUNK_TYPE_CRC32;
    chdr.chunk_sz = 1; // 4 bytes
    chdr.total_sz = shdr.chunk_hdr_sz;
    fix_chunk_header_byte_order(chdr);

    ASSERT_TRUE(_source_file.write(&chdr, sizeof(chdr)));
    ASSERT_TRUE(_source_file.seek(0, SEEK_SET));

    ASSERT_TRUE(_file.open(&_source_file));
    ASSERT_EQ(_file.read(buf, sizeof(buf)),
              oc::failure(SparseFileError::InvalidCrc32Chunk));
}

TEST_F(SparseTest, CheckInvalidChunkTotalSizeFailure)
{
    SparseHeader shdr = {};
    shdr.magic = SPARSE_HEADER_MAGIC;
    shdr.major_version = SPARSE_HEADER_MAJOR_VER;
    shdr.minor_version = 0;
    shdr.file_hdr_sz = sizeof(SparseHeader);
    shdr.chunk_hdr_sz = sizeof(ChunkHeader);
    shdr.blk_sz = 4;
    shdr.total_blks = 1;
    shdr.total_chunks = 1;
    shdr.image_checksum = 0;
    fix_sparse_header_byte_order(shdr);

    char buf[1024];

    ASSERT_TRUE(_source_file.write(&shdr, sizeof(shdr)));

    ChunkHeader chdr = {};
    chdr.chunk_type = CHUNK_TYPE_FILL;
    chdr.chunk_sz = 1; // 4 bytes
    chdr.total_sz = shdr.chunk_hdr_sz - 1u;
    fix_chunk_header_byte_order(chdr);

    ASSERT_TRUE(_source_file.write(&chdr, sizeof(chdr)));
    ASSERT_TRUE(_source_file.seek(0, SEEK_SET));

    ASSERT_TRUE(_file.open(&_source_file));
    ASSERT_EQ(_file.read(buf, sizeof(buf)),
              oc::failure(SparseFileError::InvalidChunkSize));
}

TEST_F(SparseTest, CheckInvalidChunkTypeFailure)
{
    SparseHeader shdr = {};
    shdr.magic = SPARSE_HEADER_MAGIC;
    shdr.major_version = SPARSE_HEADER_MAJOR_VER;
    shdr.minor_version = 0;
    shdr.file_hdr_sz = sizeof(SparseHeader);
    shdr.chunk_hdr_sz = sizeof(ChunkHeader);
    shdr.blk_sz = 4;
    shdr.total_blks = 1;
    shdr.total_chunks = 1;
    shdr.image_checksum = 0;
    fix_sparse_header_byte_order(shdr);

    char buf[1024];

    ASSERT_TRUE(_source_file.write(&shdr, sizeof(shdr)));

    ChunkHeader chdr = {};
    chdr.chunk_type = 0xaabb;
    chdr.chunk_sz = 1; // 4 bytes
    chdr.total_sz = shdr.chunk_hdr_sz;
    fix_chunk_header_byte_order(chdr);

    ASSERT_TRUE(_source_file.write(&chdr, sizeof(chdr)));
    ASSERT_TRUE(_source_file.seek(0, SEEK_SET));

    ASSERT_TRUE(_file.open(&_source_file));
    ASSERT_EQ(_file.read(buf, sizeof(buf)),
              oc::failure(SparseFileError::InvalidChunkType));
}

TEST_F(SparseTest, CheckReadTruncatedChunkHeaderFailure)
{
    SparseHeader shdr = {};
    shdr.magic = SPARSE_HEADER_MAGIC;
    shdr.major_version = SPARSE_HEADER_MAJOR_VER;
    shdr.minor_version = 0;
    shdr.file_hdr_sz = sizeof(SparseHeader);
    shdr.chunk_hdr_sz = sizeof(ChunkHeader);
    shdr.blk_sz = 4;
    shdr.total_blks = 1;
    shdr.total_chunks = 1;
    shdr.image_checksum = 0;
    fix_sparse_header_byte_order(shdr);

    char buf[1024];

    ASSERT_TRUE(_source_file.write(&shdr, sizeof(shdr)));

    ChunkHeader chdr = {};
    chdr.chunk_type = CHUNK_TYPE_FILL;
    chdr.chunk_sz = 1; // 4 bytes
    chdr.total_sz = shdr.chunk_hdr_sz;
    fix_chunk_header_byte_order(chdr);

    ASSERT_TRUE(_source_file.write(&chdr, sizeof(chdr) / 2));
    ASSERT_TRUE(_source_file.seek(0, SEEK_SET));

    ASSERT_TRUE(_file.open(&_source_file));
    ASSERT_EQ(_file.read(buf, sizeof(buf)),
              oc::failure(FileError::UnexpectedEof));
}

TEST_F(SparseTest, CheckReadOversizedChunkDataFailure)
{
    SparseHeader shdr = {};
    shdr.magic = SPARSE_HEADER_MAGIC;
    shdr.major_version = SPARSE_HEADER_MAJOR_VER;
    shdr.minor_version = 0;
    shdr.file_hdr_sz = sizeof(SparseHeader);
    shdr.chunk_hdr_sz = sizeof(ChunkHeader);
    shdr.blk_sz = 4;
    shdr.total_blks = 1;
    shdr.total_chunks = 1;
    shdr.image_checksum = 0;
    fix_sparse_header_byte_order(shdr);

    char buf[1024];

    ASSERT_TRUE(_source_file.write(&shdr, sizeof(shdr)));

    ChunkHeader chdr = {};
    chdr.chunk_type = CHUNK_TYPE_RAW;
    chdr.chunk_sz = 2; // 8 bytes
    chdr.total_sz = shdr.chunk_hdr_sz + chdr.chunk_sz * shdr.blk_sz;
    fix_chunk_header_byte_order(chdr);

    ASSERT_TRUE(_source_file.write(&chdr, sizeof(chdr)));
    ASSERT_TRUE(_source_file.write("01234567", 8));
    ASSERT_TRUE(_source_file.seek(0, SEEK_SET));

    ASSERT_TRUE(_file.open(&_source_file));
    ASSERT_EQ(_file.read(buf, sizeof(buf)),
              oc::failure(SparseFileError::InvalidChunkBounds));
}

TEST_F(SparseTest, CheckReadUndersizedChunkDataFailure)
{
    SparseHeader shdr = {};
    shdr.magic = SPARSE_HEADER_MAGIC;
    shdr.major_version = SPARSE_HEADER_MAJOR_VER;
    shdr.minor_version = 0;
    shdr.file_hdr_sz = sizeof(SparseHeader);
    shdr.chunk_hdr_sz = sizeof(ChunkHeader);
    shdr.blk_sz = 4;
    shdr.total_blks = 2;
    shdr.total_chunks = 1;
    shdr.image_checksum = 0;
    fix_sparse_header_byte_order(shdr);

    char buf[1024];

    ASSERT_TRUE(_source_file.write(&shdr, sizeof(shdr)));

    ChunkHeader chdr = {};
    chdr.chunk_type = CHUNK_TYPE_RAW;
    chdr.chunk_sz = 1; // 4 bytes
    chdr.total_sz = shdr.chunk_hdr_sz + chdr.chunk_sz * shdr.blk_sz;
    fix_chunk_header_byte_order(chdr);

    ASSERT_TRUE(_source_file.write(&chdr, sizeof(chdr)));
    ASSERT_TRUE(_source_file.write("0123", 1));
    ASSERT_TRUE(_source_file.seek(0, SEEK_SET));

    ASSERT_TRUE(_file.open(&_source_file));
    ASSERT_EQ(_file.read(buf, sizeof(buf)),
              oc::failure(SparseFileError::InvalidChunkBounds));
}

// All further tests use a valid sparse file

TEST_F(SparseTest, ReadValidDataWithSeekableFile)
{
    char buf[1024];
    build_valid_data(true);

    // Check that valid sparse header can be opened
    ASSERT_TRUE(_file.open(&_source_file));

    // Check that the entire file could be read and that the contents are
    // correct
    ASSERT_EQ(_file.read(buf, sizeof(buf)),
              oc::success(sizeof(expected_valid_data)));
    ASSERT_EQ(memcmp(buf, expected_valid_data, sizeof(expected_valid_data)), 0);

    // Check that partial read on chunk boundary works
    ASSERT_TRUE(_file.seek(-16, SEEK_END));
    ASSERT_EQ(_file.read(buf, sizeof(buf)), oc::success(16u));
    ASSERT_EQ(memcmp(buf, expected_valid_data + 32, 16), 0);

    // Check that partial read not on chunk boundary works
    ASSERT_TRUE(_file.seek(33, SEEK_SET));
    ASSERT_EQ(_file.read(buf, sizeof(buf)), oc::success(15u));
    ASSERT_EQ(memcmp(buf, expected_valid_data + 33, 15), 0);

    // Check that seeking past EOF is allowed and doing so returns no data
    ASSERT_TRUE(_file.seek(1000, SEEK_SET));
    ASSERT_EQ(_file.seek(1000, SEEK_CUR), oc::success(2000u));
    ASSERT_EQ(_file.read(buf, sizeof(buf)), oc::success(0u));

    // Check that seeking to the end of the sparse file works
    ASSERT_EQ(_file.seek(0, SEEK_END), oc::success(48u));

    ASSERT_TRUE(_file.close());
}

TEST_F(SparseTest, ReadValidDataWithSkippableFile)
{
    char buf[1024];
    char *ptr = buf;
    size_t remaining = sizeof(buf);
    uint64_t total = 0;
    build_valid_data(true);

    // Check that valid sparse header can be opened
    _source_file.set_seekability(Seekability::CanSkip);
    ASSERT_TRUE(_file.open(&_source_file));

    // Check that the entire file could be read and that the contents are
    // correct. Read one byte at a time to test that a read while the sparse
    // file pointer is in the middle of a raw chunk works.
    while (remaining > 0) {
        auto n = _file.read(ptr, 1);
        ASSERT_TRUE(n);
        if (n.value() == 0) {
            break;
        }

        ptr += n.value();
        total += n.value();
        remaining -= n.value();
    }

    ASSERT_EQ(total, sizeof(expected_valid_data));
    ASSERT_EQ(remaining, sizeof(buf) - sizeof(expected_valid_data));
    ASSERT_EQ(memcmp(buf, expected_valid_data, sizeof(expected_valid_data)), 0);

    // Check that seeking fails
    ASSERT_EQ(_file.seek(0, SEEK_SET), oc::failure(FileError::UnsupportedSeek));

    ASSERT_TRUE(_file.close());
}

TEST_F(SparseTest, ReadValidDataWithUnseekableFile)
{
    char buf[1024];
    char *ptr = buf;
    size_t remaining = sizeof(buf);
    uint64_t total = 0;
    build_valid_data(true);

    // Check that valid sparse header can be opened
    _source_file.set_seekability(Seekability::CanRead);
    ASSERT_TRUE(_file.open(&_source_file));

    // Check that the entire file could be read and that the contents are
    // correct. Read one byte at a time to test that a read while the sparse
    // file pointer is in the middle of a raw chunk works.
    while (remaining > 0) {
        auto n = _file.read(ptr, 1);
        ASSERT_TRUE(n);
        if (n.value() == 0) {
            break;
        }

        ptr += n.value();
        total += n.value();
        remaining -= n.value();
    }

    ASSERT_EQ(total, sizeof(expected_valid_data));
    ASSERT_EQ(remaining, sizeof(buf) - sizeof(expected_valid_data));
    ASSERT_EQ(memcmp(buf, expected_valid_data, sizeof(expected_valid_data)), 0);

    // Check that seeking fails
    ASSERT_EQ(_file.seek(0, SEEK_SET), oc::failure(FileError::UnsupportedSeek));

    ASSERT_TRUE(_file.close());
}
