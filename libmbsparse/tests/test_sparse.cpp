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
#include "mbcommon/error/error_handler.h"
#include "mbcommon/file/memory.h"
#include "mbcommon/file_error.h"

#include "mbsparse/sparse_error.h"
#include "mbsparse/sparse_p.h"


using namespace mb;

class CustomMemoryFile : public MemoryFile
{
public:
    void set_seekability(sparse::Seekability seekability)
    {
        _seekability = seekability;
    }

protected:
    Expected<uint64_t> on_seek(int64_t offset, int whence) override
    {
        switch (_seekability) {
        case sparse::Seekability::CAN_SEEK:
            return MemoryFile::on_seek(offset, whence);
        case sparse::Seekability::CAN_SKIP:
            if (whence == SEEK_CUR && offset >= 0) {
                return MemoryFile::on_seek(offset, whence);
            }
            break;
        case sparse::Seekability::CAN_READ:
            break;
        default:
            MB_UNREACHABLE("Invalid seekability");
        }

        return make_error<FileError>(FileErrorType::UnsupportedSeek);
    }

private:
    sparse::Seekability _seekability = sparse::Seekability::CAN_SEEK;
};

struct SparseTest : testing::Test
{
    CustomMemoryFile _source_file;
    sparse::SparseFile _file;
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
        ASSERT_TRUE(!!_source_file.open(&_data, &_size));
    }

    void build_valid_data()
    {
        sparse::SparseHeader shdr = {};
        shdr.magic = sparse::SPARSE_HEADER_MAGIC;
        shdr.major_version = sparse::SPARSE_HEADER_MAJOR_VER;
        shdr.minor_version = 0;
        shdr.file_hdr_sz = sizeof(sparse::SparseHeader);
        shdr.chunk_hdr_sz = sizeof(sparse::ChunkHeader);
        shdr.blk_sz = 4;
        shdr.total_blks = 12;
        shdr.total_chunks = 4;
        shdr.image_checksum = 0;
        fix_sparse_header_byte_order(shdr);

        ASSERT_TRUE(!!_source_file.write(&shdr, sizeof(shdr)));

        sparse::ChunkHeader chdr;

        // [1/4] Add raw chunk header
        chdr = {};
        chdr.chunk_type = sparse::CHUNK_TYPE_RAW;
        chdr.chunk_sz = 4; // 16 bytes
        chdr.total_sz = shdr.chunk_hdr_sz + chdr.chunk_sz * shdr.blk_sz;
        fix_chunk_header_byte_order(chdr);

        ASSERT_TRUE(!!_source_file.write(&chdr, sizeof(chdr)));
        ASSERT_TRUE(!!_source_file.write("0123456789abcdef", 16));

        // [2/4] Add fill chunk header
        chdr = {};
        chdr.chunk_type = sparse::CHUNK_TYPE_FILL;
        chdr.chunk_sz = 4; // 16 bytes
        chdr.total_sz = shdr.chunk_hdr_sz + sizeof(uint32_t);
        fix_chunk_header_byte_order(chdr);

        ASSERT_TRUE(!!_source_file.write(&chdr, sizeof(chdr)));

        uint32_t fill_val = mb_htole32(0x12345678);
        ASSERT_TRUE(!!_source_file.write(&fill_val, sizeof(fill_val)));

        // [3/4] Add skip chunk header
        chdr = {};
        chdr.chunk_type = sparse::CHUNK_TYPE_DONT_CARE;
        chdr.chunk_sz = 4; // 16 bytes
        chdr.total_sz = shdr.chunk_hdr_sz;
        fix_chunk_header_byte_order(chdr);

        ASSERT_TRUE(!!_source_file.write(&chdr, sizeof(chdr)));

        // [4/4] Add CRC32 chunk header
        chdr = {};
        chdr.chunk_type = sparse::CHUNK_TYPE_CRC32;
        chdr.chunk_sz = 0;
        chdr.total_sz = shdr.chunk_hdr_sz + sizeof(uint32_t);
        fix_chunk_header_byte_order(chdr);

        ASSERT_TRUE(!!_source_file.write(&chdr, sizeof(chdr)));
        ASSERT_TRUE(!!_source_file.write("\x00\x00\x00\x00", 4));

        // Move back to beginning of the file
        ASSERT_TRUE(!!_source_file.seek(0, SEEK_SET));
    }

    static void fix_sparse_header_byte_order(sparse::SparseHeader &header)
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

    static void fix_chunk_header_byte_order(sparse::ChunkHeader &header)
    {
        header.chunk_type = mb_htole16(header.chunk_type);
        header.reserved1 = mb_htole16(header.reserved1);
        header.chunk_sz = mb_htole32(header.chunk_sz);
        header.total_sz = mb_htole32(header.total_sz);
    }
};

constexpr unsigned char SparseTest::expected_valid_data[];

TEST_F(SparseTest, CheckOpeningUnopenedFileFails)
{
    ASSERT_TRUE(!!_source_file.close());
    auto ret = _file.open(&_source_file);
    ASSERT_FALSE(ret);
    ASSERT_FALSE(handle_errors(
        ret.take_error(),
        [](const FileError &err) {
            ASSERT_EQ(err.type(), FileErrorType::InvalidState);
            ASSERT_NE(err.details().find("not open"), std::string::npos);
        }
    ));
}

TEST_F(SparseTest, CheckSparseHeaderInvalidMagicFailure)
{
    sparse::SparseHeader shdr = {};
    shdr.magic = 0xaabbccdd;
    shdr.major_version = sparse::SPARSE_HEADER_MAJOR_VER;
    shdr.minor_version = 0;
    shdr.file_hdr_sz = sizeof(sparse::SparseHeader);
    shdr.chunk_hdr_sz = sizeof(sparse::ChunkHeader);
    shdr.blk_sz = 4096;
    shdr.total_blks = 1;
    shdr.total_chunks = 0;
    shdr.image_checksum = 0;
    fix_sparse_header_byte_order(shdr);

    ASSERT_TRUE(!!_source_file.write(&shdr, sizeof(shdr)));
    ASSERT_TRUE(!!_source_file.truncate(shdr.total_blks * shdr.blk_sz));
    ASSERT_TRUE(!!_source_file.seek(0, SEEK_SET));

    auto ret = _file.open(&_source_file);
    ASSERT_FALSE(ret);
    ASSERT_FALSE(handle_errors(
        ret.take_error(),
        [](const SparseFileError &err) {
            ASSERT_EQ(err.type(), SparseFileErrorType::InvalidSparseMagic);
        }
    ));
}

TEST_F(SparseTest, CheckSparseHeaderInvalidMajorVersionFailure)
{
    sparse::SparseHeader shdr = {};
    shdr.magic = sparse::SPARSE_HEADER_MAGIC;
    shdr.major_version = 0xaabb;
    shdr.minor_version = 0;
    shdr.file_hdr_sz = sizeof(sparse::SparseHeader);
    shdr.chunk_hdr_sz = sizeof(sparse::ChunkHeader);
    shdr.blk_sz = 4096;
    shdr.total_blks = 1;
    shdr.total_chunks = 0;
    shdr.image_checksum = 0;
    fix_sparse_header_byte_order(shdr);

    ASSERT_TRUE(!!_source_file.write(&shdr, sizeof(shdr)));
    ASSERT_TRUE(!!_source_file.truncate(shdr.total_blks * shdr.blk_sz));
    ASSERT_TRUE(!!_source_file.seek(0, SEEK_SET));

    auto ret = _file.open(&_source_file);
    ASSERT_FALSE(ret);
    ASSERT_FALSE(handle_errors(
        ret.take_error(),
        [](const SparseFileError &err) {
            ASSERT_EQ(err.type(), SparseFileErrorType::InvalidSparseMajorVersion);
        }
    ));
}

TEST_F(SparseTest, CheckSparseHeaderInvalidMinorVersionOk)
{
    sparse::SparseHeader shdr = {};
    shdr.magic = sparse::SPARSE_HEADER_MAGIC;
    shdr.major_version = sparse::SPARSE_HEADER_MAJOR_VER;
    shdr.minor_version = 0xaabb;
    shdr.file_hdr_sz = sizeof(sparse::SparseHeader);
    shdr.chunk_hdr_sz = sizeof(sparse::ChunkHeader);
    shdr.blk_sz = 4096;
    shdr.total_blks = 1;
    shdr.total_chunks = 0;
    shdr.image_checksum = 0;
    fix_sparse_header_byte_order(shdr);

    ASSERT_TRUE(!!_source_file.write(&shdr, sizeof(shdr)));
    ASSERT_TRUE(!!_source_file.truncate(shdr.total_blks * shdr.blk_sz));
    ASSERT_TRUE(!!_source_file.seek(0, SEEK_SET));

    ASSERT_TRUE(!!_file.open(&_source_file));
}

TEST_F(SparseTest, CheckSparseHeaderUndersizedSparseHeaderSizeFailure)
{
    sparse::SparseHeader shdr = {};
    shdr.magic = sparse::SPARSE_HEADER_MAGIC;
    shdr.major_version = sparse::SPARSE_HEADER_MAJOR_VER;
    shdr.minor_version = 0;
    shdr.file_hdr_sz = sizeof(sparse::SparseHeader) - 1;
    shdr.chunk_hdr_sz = sizeof(sparse::ChunkHeader);
    shdr.blk_sz = 4096;
    shdr.total_blks = 1;
    shdr.total_chunks = 0;
    shdr.image_checksum = 0;
    fix_sparse_header_byte_order(shdr);

    ASSERT_TRUE(!!_source_file.write(&shdr, sizeof(shdr)));
    ASSERT_TRUE(!!_source_file.truncate(shdr.total_blks * shdr.blk_sz));
    ASSERT_TRUE(!!_source_file.seek(0, SEEK_SET));

    auto ret = _file.open(&_source_file);
    ASSERT_FALSE(ret);
    ASSERT_FALSE(handle_errors(
        ret.take_error(),
        [](const SparseFileError &err) {
            ASSERT_EQ(err.type(), SparseFileErrorType::InvalidSparseHeaderSize);
        }
    ));
}

TEST_F(SparseTest, CheckSparseHeaderUndersizedChunkHeaderSizeFailure)
{
    sparse::SparseHeader shdr = {};
    shdr.magic = sparse::SPARSE_HEADER_MAGIC;
    shdr.major_version = sparse::SPARSE_HEADER_MAJOR_VER;
    shdr.minor_version = 0;
    shdr.file_hdr_sz = sizeof(sparse::SparseHeader);
    shdr.chunk_hdr_sz = sizeof(sparse::ChunkHeader) - 1;
    shdr.blk_sz = 4096;
    shdr.total_blks = 1;
    shdr.total_chunks = 0;
    shdr.image_checksum = 0;
    fix_sparse_header_byte_order(shdr);

    ASSERT_TRUE(!!_source_file.write(&shdr, sizeof(shdr)));
    ASSERT_TRUE(!!_source_file.truncate(shdr.total_blks * shdr.blk_sz));
    ASSERT_TRUE(!!_source_file.seek(0, SEEK_SET));

    auto ret = _file.open(&_source_file);
    ASSERT_FALSE(ret);
    ASSERT_FALSE(handle_errors(
        ret.take_error(),
        [](const SparseFileError &err) {
            ASSERT_EQ(err.type(), SparseFileErrorType::InvalidChunkHeaderSize);
        }
    ));
}

TEST_F(SparseTest, CheckInvalidRawChunkFatal)
{
    sparse::SparseHeader shdr = {};
    shdr.magic = sparse::SPARSE_HEADER_MAGIC;
    shdr.major_version = sparse::SPARSE_HEADER_MAJOR_VER;
    shdr.minor_version = 0;
    shdr.file_hdr_sz = sizeof(sparse::SparseHeader);
    shdr.chunk_hdr_sz = sizeof(sparse::ChunkHeader);
    shdr.blk_sz = 4;
    shdr.total_blks = 1;
    shdr.total_chunks = 1;
    shdr.image_checksum = 0;
    fix_sparse_header_byte_order(shdr);

    char buf[1024];

    ASSERT_TRUE(!!_source_file.write(&shdr, sizeof(shdr)));

    sparse::ChunkHeader chdr = {};
    chdr.chunk_type = sparse::CHUNK_TYPE_RAW;
    chdr.chunk_sz = 1; // 4 bytes
    chdr.total_sz = shdr.chunk_hdr_sz + chdr.chunk_sz * shdr.blk_sz + 1;
    fix_chunk_header_byte_order(chdr);

    ASSERT_TRUE(!!_source_file.write(&chdr, sizeof(chdr)));
    ASSERT_TRUE(!!_source_file.write("1234", 4));
    ASSERT_TRUE(!!_source_file.seek(0, SEEK_SET));

    ASSERT_TRUE(!!_file.open(&_source_file));
    auto ret = _file.read(buf, sizeof(buf));
    ASSERT_FALSE(ret);
    ASSERT_TRUE(_file.is_fatal());
    ASSERT_FALSE(handle_errors(
        ret.take_error(),
        [](const SparseFileError &err) {
            ASSERT_EQ(err.type(), SparseFileErrorType::InvalidRawChunk);
        }
    ));
}

TEST_F(SparseTest, CheckInvalidFillChunkFatal)
{
    sparse::SparseHeader shdr = {};
    shdr.magic = sparse::SPARSE_HEADER_MAGIC;
    shdr.major_version = sparse::SPARSE_HEADER_MAJOR_VER;
    shdr.minor_version = 0;
    shdr.file_hdr_sz = sizeof(sparse::SparseHeader);
    shdr.chunk_hdr_sz = sizeof(sparse::ChunkHeader);
    shdr.blk_sz = 4;
    shdr.total_blks = 1;
    shdr.total_chunks = 1;
    shdr.image_checksum = 0;
    fix_sparse_header_byte_order(shdr);

    char buf[1024];

    ASSERT_TRUE(!!_source_file.write(&shdr, sizeof(shdr)));

    sparse::ChunkHeader chdr = {};
    chdr.chunk_type = sparse::CHUNK_TYPE_FILL;
    chdr.chunk_sz = 1; // 4 bytes
    chdr.total_sz = shdr.chunk_hdr_sz + 0;
    fix_chunk_header_byte_order(chdr);

    ASSERT_TRUE(!!_source_file.write(&chdr, sizeof(chdr)));
    ASSERT_TRUE(!!_source_file.seek(0, SEEK_SET));

    ASSERT_TRUE(!!_file.open(&_source_file));
    auto ret = _file.read(buf, sizeof(buf));
    ASSERT_FALSE(ret);
    ASSERT_TRUE(_file.is_fatal());
    ASSERT_FALSE(handle_errors(
        ret.take_error(),
        [](const SparseFileError &err) {
            ASSERT_EQ(err.type(), SparseFileErrorType::InvalidFillChunk);
        }
    ));
}

TEST_F(SparseTest, CheckInvalidSkipChunkFatal)
{
    sparse::SparseHeader shdr = {};
    shdr.magic = sparse::SPARSE_HEADER_MAGIC;
    shdr.major_version = sparse::SPARSE_HEADER_MAJOR_VER;
    shdr.minor_version = 0;
    shdr.file_hdr_sz = sizeof(sparse::SparseHeader);
    shdr.chunk_hdr_sz = sizeof(sparse::ChunkHeader);
    shdr.blk_sz = 4;
    shdr.total_blks = 1;
    shdr.total_chunks = 1;
    shdr.image_checksum = 0;
    fix_sparse_header_byte_order(shdr);

    char buf[1024];

    ASSERT_TRUE(!!_source_file.write(&shdr, sizeof(shdr)));

    sparse::ChunkHeader chdr = {};
    chdr.chunk_type = sparse::CHUNK_TYPE_DONT_CARE;
    chdr.chunk_sz = 1; // 4 bytes
    chdr.total_sz = shdr.chunk_hdr_sz + 1;
    fix_chunk_header_byte_order(chdr);

    ASSERT_TRUE(!!_source_file.write(&chdr, sizeof(chdr)));
    ASSERT_TRUE(!!_source_file.seek(0, SEEK_SET));

    ASSERT_TRUE(!!_file.open(&_source_file));
    auto ret = _file.read(buf, sizeof(buf));
    ASSERT_FALSE(ret);
    ASSERT_TRUE(_file.is_fatal());
    ASSERT_FALSE(handle_errors(
        ret.take_error(),
        [](const SparseFileError &err) {
            ASSERT_EQ(err.type(), SparseFileErrorType::InvalidSkipChunk);
        }
    ));
}

TEST_F(SparseTest, CheckInvalidCrc32ChunkFatal)
{
    sparse::SparseHeader shdr = {};
    shdr.magic = sparse::SPARSE_HEADER_MAGIC;
    shdr.major_version = sparse::SPARSE_HEADER_MAJOR_VER;
    shdr.minor_version = 0;
    shdr.file_hdr_sz = sizeof(sparse::SparseHeader);
    shdr.chunk_hdr_sz = sizeof(sparse::ChunkHeader);
    shdr.blk_sz = 4;
    shdr.total_blks = 1;
    shdr.total_chunks = 1;
    shdr.image_checksum = 0;
    fix_sparse_header_byte_order(shdr);

    char buf[1024];

    ASSERT_TRUE(!!_source_file.write(&shdr, sizeof(shdr)));

    sparse::ChunkHeader chdr = {};
    chdr.chunk_type = sparse::CHUNK_TYPE_CRC32;
    chdr.chunk_sz = 1; // 4 bytes
    chdr.total_sz = shdr.chunk_hdr_sz;
    fix_chunk_header_byte_order(chdr);

    ASSERT_TRUE(!!_source_file.write(&chdr, sizeof(chdr)));
    ASSERT_TRUE(!!_source_file.seek(0, SEEK_SET));

    ASSERT_TRUE(!!_file.open(&_source_file));
    auto ret = _file.read(buf, sizeof(buf));
    ASSERT_FALSE(ret);
    ASSERT_TRUE(_file.is_fatal());
    ASSERT_FALSE(handle_errors(
        ret.take_error(),
        [](const SparseFileError &err) {
            ASSERT_EQ(err.type(), SparseFileErrorType::InvalidCrc32Chunk);
        }
    ));
}

TEST_F(SparseTest, CheckInvalidChunkTotalSizeFatal)
{
    sparse::SparseHeader shdr = {};
    shdr.magic = sparse::SPARSE_HEADER_MAGIC;
    shdr.major_version = sparse::SPARSE_HEADER_MAJOR_VER;
    shdr.minor_version = 0;
    shdr.file_hdr_sz = sizeof(sparse::SparseHeader);
    shdr.chunk_hdr_sz = sizeof(sparse::ChunkHeader);
    shdr.blk_sz = 4;
    shdr.total_blks = 1;
    shdr.total_chunks = 1;
    shdr.image_checksum = 0;
    fix_sparse_header_byte_order(shdr);

    char buf[1024];

    ASSERT_TRUE(!!_source_file.write(&shdr, sizeof(shdr)));

    sparse::ChunkHeader chdr = {};
    chdr.chunk_type = sparse::CHUNK_TYPE_FILL;
    chdr.chunk_sz = 1; // 4 bytes
    chdr.total_sz = shdr.chunk_hdr_sz - 1;
    fix_chunk_header_byte_order(chdr);

    ASSERT_TRUE(!!_source_file.write(&chdr, sizeof(chdr)));
    ASSERT_TRUE(!!_source_file.seek(0, SEEK_SET));

    ASSERT_TRUE(!!_file.open(&_source_file));
    auto ret = _file.read(buf, sizeof(buf));
    ASSERT_FALSE(ret);
    ASSERT_TRUE(_file.is_fatal());
    ASSERT_FALSE(handle_errors(
        ret.take_error(),
        [](const SparseFileError &err) {
            ASSERT_EQ(err.type(), SparseFileErrorType::InvalidChunkSize);
        }
    ));
}

TEST_F(SparseTest, CheckInvalidChunkTypeFatal)
{
    sparse::SparseHeader shdr = {};
    shdr.magic = sparse::SPARSE_HEADER_MAGIC;
    shdr.major_version = sparse::SPARSE_HEADER_MAJOR_VER;
    shdr.minor_version = 0;
    shdr.file_hdr_sz = sizeof(sparse::SparseHeader);
    shdr.chunk_hdr_sz = sizeof(sparse::ChunkHeader);
    shdr.blk_sz = 4;
    shdr.total_blks = 1;
    shdr.total_chunks = 1;
    shdr.image_checksum = 0;
    fix_sparse_header_byte_order(shdr);

    char buf[1024];

    ASSERT_TRUE(!!_source_file.write(&shdr, sizeof(shdr)));

    sparse::ChunkHeader chdr = {};
    chdr.chunk_type = 0xaabb;
    chdr.chunk_sz = 1; // 4 bytes
    chdr.total_sz = shdr.chunk_hdr_sz;
    fix_chunk_header_byte_order(chdr);

    ASSERT_TRUE(!!_source_file.write(&chdr, sizeof(chdr)));
    ASSERT_TRUE(!!_source_file.seek(0, SEEK_SET));

    ASSERT_TRUE(!!_file.open(&_source_file));
    auto ret = _file.read(buf, sizeof(buf));
    ASSERT_FALSE(ret);
    ASSERT_TRUE(_file.is_fatal());
    ASSERT_FALSE(handle_errors(
        ret.take_error(),
        [](const SparseFileError &err) {
            ASSERT_EQ(err.type(), SparseFileErrorType::InvalidChunkType);
        }
    ));
}

TEST_F(SparseTest, CheckReadTruncatedChunkHeaderFatal)
{
    sparse::SparseHeader shdr = {};
    shdr.magic = sparse::SPARSE_HEADER_MAGIC;
    shdr.major_version = sparse::SPARSE_HEADER_MAJOR_VER;
    shdr.minor_version = 0;
    shdr.file_hdr_sz = sizeof(sparse::SparseHeader);
    shdr.chunk_hdr_sz = sizeof(sparse::ChunkHeader);
    shdr.blk_sz = 4;
    shdr.total_blks = 1;
    shdr.total_chunks = 1;
    shdr.image_checksum = 0;
    fix_sparse_header_byte_order(shdr);

    char buf[1024];

    ASSERT_TRUE(!!_source_file.write(&shdr, sizeof(shdr)));

    sparse::ChunkHeader chdr = {};
    chdr.chunk_type = sparse::CHUNK_TYPE_FILL;
    chdr.chunk_sz = 1; // 4 bytes
    chdr.total_sz = shdr.chunk_hdr_sz;
    fix_chunk_header_byte_order(chdr);

    ASSERT_TRUE(!!_source_file.write(&chdr, sizeof(chdr) / 2));
    ASSERT_TRUE(!!_source_file.seek(0, SEEK_SET));

    ASSERT_TRUE(!!_file.open(&_source_file));
    auto ret = _file.read(buf, sizeof(buf));
    ASSERT_FALSE(ret);
    ASSERT_TRUE(_file.is_fatal());
    ASSERT_FALSE(handle_errors(
        ret.take_error(),
        [](const SparseFileError &err) {
            ASSERT_EQ(err.type(), SparseFileErrorType::UnexpectedEndOfFile);
        }
    ));
}

TEST_F(SparseTest, CheckReadOversizedChunkDataFatal)
{
    sparse::SparseHeader shdr = {};
    shdr.magic = sparse::SPARSE_HEADER_MAGIC;
    shdr.major_version = sparse::SPARSE_HEADER_MAJOR_VER;
    shdr.minor_version = 0;
    shdr.file_hdr_sz = sizeof(sparse::SparseHeader);
    shdr.chunk_hdr_sz = sizeof(sparse::ChunkHeader);
    shdr.blk_sz = 4;
    shdr.total_blks = 1;
    shdr.total_chunks = 1;
    shdr.image_checksum = 0;
    fix_sparse_header_byte_order(shdr);

    char buf[1024];

    ASSERT_TRUE(!!_source_file.write(&shdr, sizeof(shdr)));

    sparse::ChunkHeader chdr = {};
    chdr.chunk_type = sparse::CHUNK_TYPE_RAW;
    chdr.chunk_sz = 2; // 8 bytes
    chdr.total_sz = shdr.chunk_hdr_sz + chdr.chunk_sz * shdr.blk_sz;
    fix_chunk_header_byte_order(chdr);

    ASSERT_TRUE(!!_source_file.write(&chdr, sizeof(chdr)));
    ASSERT_TRUE(!!_source_file.write("01234567", 8));
    ASSERT_TRUE(!!_source_file.seek(0, SEEK_SET));

    ASSERT_TRUE(!!_file.open(&_source_file));
    auto ret = _file.read(buf, sizeof(buf));
    ASSERT_FALSE(ret);
    ASSERT_TRUE(_file.is_fatal());
    ASSERT_FALSE(handle_errors(
        ret.take_error(),
        [](const SparseFileError &err) {
            ASSERT_EQ(err.type(), SparseFileErrorType::InvalidChunkBounds);
        }
    ));
}

TEST_F(SparseTest, CheckReadUndersizedChunkDataFatal)
{
    sparse::SparseHeader shdr = {};
    shdr.magic = sparse::SPARSE_HEADER_MAGIC;
    shdr.major_version = sparse::SPARSE_HEADER_MAJOR_VER;
    shdr.minor_version = 0;
    shdr.file_hdr_sz = sizeof(sparse::SparseHeader);
    shdr.chunk_hdr_sz = sizeof(sparse::ChunkHeader);
    shdr.blk_sz = 4;
    shdr.total_blks = 2;
    shdr.total_chunks = 1;
    shdr.image_checksum = 0;
    fix_sparse_header_byte_order(shdr);

    char buf[1024];

    ASSERT_TRUE(!!_source_file.write(&shdr, sizeof(shdr)));

    sparse::ChunkHeader chdr = {};
    chdr.chunk_type = sparse::CHUNK_TYPE_RAW;
    chdr.chunk_sz = 1; // 4 bytes
    chdr.total_sz = shdr.chunk_hdr_sz + chdr.chunk_sz * shdr.blk_sz;
    fix_chunk_header_byte_order(chdr);

    ASSERT_TRUE(!!_source_file.write(&chdr, sizeof(chdr)));
    ASSERT_TRUE(!!_source_file.write("0123", 1));
    ASSERT_TRUE(!!_source_file.seek(0, SEEK_SET));

    ASSERT_TRUE(!!_file.open(&_source_file));
    auto ret = _file.read(buf, sizeof(buf));
    ASSERT_FALSE(ret);
    ASSERT_TRUE(_file.is_fatal());
    ASSERT_FALSE(handle_errors(
        ret.take_error(),
        [](const SparseFileError &err) {
            ASSERT_EQ(err.type(), SparseFileErrorType::InvalidChunkBounds);
        }
    ));
}

// All further tests use a valid sparse file

TEST_F(SparseTest, ReadValidDataWithSeekableFile)
{
    char buf[1024];
    build_valid_data();

    // Check that valid sparse header can be opened
    ASSERT_TRUE(!!_file.open(&_source_file));

    // Check that the entire file could be read and that the contents are
    // correct
    auto n = _file.read(buf, sizeof(buf));
    ASSERT_TRUE(!!n);
    ASSERT_EQ(*n, sizeof(expected_valid_data));
    ASSERT_EQ(memcmp(buf, expected_valid_data, sizeof(expected_valid_data)), 0);

    // Check that partial read on chunk boundary works
    ASSERT_TRUE(!!_file.seek(-16, SEEK_END));
    n = _file.read(buf, sizeof(buf));
    ASSERT_TRUE(!!n);
    ASSERT_EQ(*n, 16u);
    ASSERT_EQ(memcmp(buf, expected_valid_data + 32, 16), 0);

    // Check that partial read not on chunk boundary works
    ASSERT_TRUE(!!_file.seek(33, SEEK_SET));
    n = _file.read(buf, sizeof(buf));
    ASSERT_TRUE(!!n);
    ASSERT_EQ(*n, 15u);
    ASSERT_EQ(memcmp(buf, expected_valid_data + 33, 15), 0);

    // Check that seeking past EOF is allowed and doing so returns no data
    ASSERT_TRUE(!!_file.seek(1000, SEEK_SET));
    auto pos = _file.seek(1000, SEEK_CUR);
    ASSERT_TRUE(!!pos);
    ASSERT_EQ(*pos, 2000u);
    n = _file.read(buf, sizeof(buf));
    ASSERT_TRUE(!!n);
    ASSERT_EQ(*n, 0u);

    // Check that seeking to the end of the sparse file works
    pos = _file.seek(0, SEEK_END);
    ASSERT_TRUE(!!pos);
    ASSERT_EQ(*pos, 48u);

    ASSERT_TRUE(!!_file.close());
}

TEST_F(SparseTest, ReadValidDataWithSkippableFile)
{
    char buf[1024];
    char *ptr = buf;
    size_t remaining = sizeof(buf);
    uint64_t total = 0;
    build_valid_data();

    // Check that valid sparse header can be opened
    _source_file.set_seekability(sparse::Seekability::CAN_SKIP);
    ASSERT_TRUE(!!_file.open(&_source_file));

    // Check that the entire file could be read and that the contents are
    // correct. Read one byte at a time to test that a read while the sparse
    // file pointer is in the middle of a raw chunk works.
    while (remaining > 0) {
        auto n = _file.read(ptr, 1);
        ASSERT_TRUE(!!n);
        if (*n == 0) {
            break;
        }

        ptr += *n;
        total += *n;
        remaining -= *n;
    }

    ASSERT_EQ(total, sizeof(expected_valid_data));
    ASSERT_EQ(remaining, sizeof(buf) - sizeof(expected_valid_data));
    ASSERT_EQ(memcmp(buf, expected_valid_data, sizeof(expected_valid_data)), 0);

    // Check that seeking fails
    auto ret = _file.seek(0, SEEK_SET);
    ASSERT_FALSE(ret);
    ASSERT_FALSE(handle_errors(
        ret.take_error(),
        [](const FileError &err) {
            ASSERT_EQ(err.type(), FileErrorType::UnsupportedSeek);
        }
    ));

    ASSERT_TRUE(!!_file.close());
}

TEST_F(SparseTest, ReadValidDataWithUnseekableFile)
{
    char buf[1024];
    char *ptr = buf;
    size_t remaining = sizeof(buf);
    uint64_t total = 0;
    build_valid_data();

    // Check that valid sparse header can be opened
    _source_file.set_seekability(sparse::Seekability::CAN_READ);
    ASSERT_TRUE(!!_file.open(&_source_file));

    // Check that the entire file could be read and that the contents are
    // correct. Read one byte at a time to test that a read while the sparse
    // file pointer is in the middle of a raw chunk works.
    while (remaining > 0) {
        auto n = _file.read(ptr, 1);
        ASSERT_TRUE(!!n);
        if (*n == 0) {
            break;
        }

        ptr += *n;
        total += *n;
        remaining -= *n;
    }

    ASSERT_EQ(total, sizeof(expected_valid_data));
    ASSERT_EQ(remaining, sizeof(buf) - sizeof(expected_valid_data));
    ASSERT_EQ(memcmp(buf, expected_valid_data, sizeof(expected_valid_data)), 0);

    // Check that seeking fails
    auto ret = _file.seek(0, SEEK_SET);
    ASSERT_FALSE(ret);
    ASSERT_FALSE(handle_errors(
        ret.take_error(),
        [](const FileError &err) {
            ASSERT_EQ(err.type(), FileErrorType::UnsupportedSeek);
        }
    ));

    ASSERT_TRUE(!!_file.close());
}
