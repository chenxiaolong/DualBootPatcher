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

struct SparseTest : testing::Test
{
    SparseCtx *_ctx;
    std::vector<unsigned char> _data;
    size_t _pos = 0;

    SparseTest()
    {
        _ctx = sparseCtxNew();
    }

    virtual ~SparseTest()
    {
        sparseCtxFree(_ctx);
    }

    static bool cbRead(void *buf, uint64_t size, uint64_t *bytesRead,
                       void *userData)
    {
        SparseTest *test = static_cast<SparseTest *>(userData);
        if (test->_pos > test->_data.size()) {
            *bytesRead = 0;
        } else {
            uint64_t canRead = std::min<uint64_t>(
                    size, test->_data.size() - test->_pos);
            memcpy(buf, test->_data.data() + test->_pos, canRead);
            test->_pos += canRead;
            *bytesRead = canRead;
        }
        return true;
    }

    static bool cbSeek(int64_t offset, int whence, void *userData)
    {
        SparseTest *test = static_cast<SparseTest *>(userData);
        switch (whence) {
        case SEEK_SET:
            if (offset < 0) {
                return false;
            }
            test->_pos = offset;
            break;
        case SEEK_CUR:
            if ((offset < 0 && (uint64_t) -offset > test->_pos)
                    || (offset > 0 && test->_pos >= UINT64_MAX - offset)) {
                return false;
            }
            test->_pos += offset;
            break;
        case SEEK_END:
            if ((offset < 0 && (uint64_t) -offset > test->_data.size())
                    || (offset > 0 && test->_data.size() >= UINT64_MAX - offset)) {
                return false;
            }
            test->_pos = test->_data.size() + offset;
            break;
        default:
            return false;
        }
        return true;
    }

    bool sparseOpen()
    {
        return ::sparseOpen(_ctx, nullptr, nullptr, &cbRead, &cbSeek, nullptr,
                            this);
    }

    bool sparseOpenNoSeek()
    {
        return ::sparseOpen(_ctx, nullptr, nullptr, &cbRead, nullptr, nullptr,
                            this);
    }

    bool sparseClose()
    {
        return ::sparseClose(_ctx);
    }

    bool sparseRead(void *buf, uint64_t size, uint64_t *bytesRead)
    {
        return ::sparseRead(_ctx, buf, size, bytesRead);
    }

    bool sparseSeek(int64_t offset, int whence)
    {
        return ::sparseSeek(_ctx, offset, whence);
    }

    bool sparseTell(uint64_t *offset)
    {
        return ::sparseTell(_ctx, offset);
    }

    void buildDataHeaderProperSized()
    {
        SparseHeader hdr;
        memset(&hdr, 0, sizeof(SparseHeader));
        hdr.magic = SPARSE_HEADER_MAGIC;
        hdr.major_version = SPARSE_HEADER_MAJOR_VER;
        hdr.minor_version = 0;
        hdr.file_hdr_sz = sizeof(SparseHeader);
        hdr.chunk_hdr_sz = sizeof(ChunkHeader);
        hdr.blk_sz = 4096;
        hdr.total_blks = 1;
        hdr.total_chunks = 0;
        hdr.image_checksum = 0;

        _data.resize(std::max<size_t>(hdr.blk_sz, sizeof(SparseHeader)));
        memcpy(_data.data(), &hdr, sizeof(SparseHeader));
    }

    void buildDataHeaderExtraSized()
    {
        SparseHeader hdr;
        memset(&hdr, 0, sizeof(SparseHeader));
        hdr.magic = SPARSE_HEADER_MAGIC;
        hdr.major_version = SPARSE_HEADER_MAJOR_VER;
        hdr.minor_version = 0;
        hdr.file_hdr_sz = sizeof(SparseHeader) + 100;
        hdr.chunk_hdr_sz = sizeof(ChunkHeader) + 100;
        hdr.blk_sz = 4096;
        hdr.total_blks = 1;
        hdr.total_chunks = 0;
        hdr.image_checksum = 0;

        _data.resize(std::max<size_t>(hdr.blk_sz, sizeof(SparseHeader) + 100));
        memcpy(_data.data(), &hdr, sizeof(SparseHeader));
    }

    void buildDataHeaderUnderSized()
    {
        // Build sparse header
        SparseHeader hdr;
        memset(&hdr, 0, sizeof(SparseHeader));
        hdr.magic = SPARSE_HEADER_MAGIC;
        hdr.major_version = SPARSE_HEADER_MAJOR_VER;
        hdr.minor_version = 0;
        hdr.file_hdr_sz = sizeof(SparseHeader) - 4;
        hdr.chunk_hdr_sz = sizeof(ChunkHeader);
        hdr.blk_sz = 4096;
        hdr.total_blks = 0;
        hdr.total_chunks = 0;
        hdr.image_checksum = 0;

        _data.resize(sizeof(SparseHeader));
        memcpy(_data.data(), &hdr, sizeof(SparseHeader));
    }

    void buildDataCompleteValid()
    {
        SparseHeader hdr;
        auto const *hdrPtr = reinterpret_cast<const unsigned char *>(&hdr);

        memset(&hdr, 0, sizeof(SparseHeader));
        hdr.magic = SPARSE_HEADER_MAGIC;
        hdr.major_version = SPARSE_HEADER_MAJOR_VER;
        hdr.minor_version = 0;
        hdr.file_hdr_sz = sizeof(SparseHeader);
        hdr.chunk_hdr_sz = sizeof(ChunkHeader);
        hdr.blk_sz = 4;
        hdr.total_blks = 12;
        hdr.total_chunks = 4;
        hdr.image_checksum = 0;
        _data.insert(_data.end(), hdrPtr, hdrPtr + sizeof(SparseHeader));

        ChunkHeader chdr;
        auto const *chdrPtr = reinterpret_cast<const unsigned char *>(&chdr);

        // [1/4] Add raw chunk header
        memset(&chdr, 0, sizeof(ChunkHeader));
        chdr.chunk_type = CHUNK_TYPE_RAW;
        chdr.chunk_sz = 4; // 16 bytes
        chdr.total_sz = hdr.chunk_hdr_sz + chdr.chunk_sz * hdr.blk_sz;
        _data.insert(_data.end(), chdrPtr, chdrPtr + sizeof(ChunkHeader));
        // [1/4] Add raw chunk data
        const char *hexDigits = "0123456789abcdef";
        _data.insert(_data.end(), hexDigits, hexDigits + 16);

        // [2/4] Add fill chunk header
        memset(&chdr, 0, sizeof(ChunkHeader));
        chdr.chunk_type = CHUNK_TYPE_FILL;
        chdr.chunk_sz = 4; // 16 bytes
        chdr.total_sz = hdr.chunk_hdr_sz + sizeof(uint32_t);
        _data.insert(_data.end(), chdrPtr, chdrPtr + sizeof(ChunkHeader));
        // [2/4] Add fill chunk data
        uint32_t fillVal = 0x12345678;
        auto const *fillValPtr =
                reinterpret_cast<const unsigned char *>(&fillVal);
        _data.insert(_data.end(), fillValPtr, fillValPtr + sizeof(uint32_t));

        // [3/4] Add skip chunk header
        memset(&chdr, 0, sizeof(ChunkHeader));
        chdr.chunk_type = CHUNK_TYPE_DONT_CARE;
        chdr.chunk_sz = 4; // 16 bytes
        chdr.total_sz = hdr.chunk_hdr_sz;
        _data.insert(_data.end(), chdrPtr, chdrPtr + sizeof(ChunkHeader));

        // [4/4] Add CRC32 chunk header
        memset(&chdr, 0, sizeof(ChunkHeader));
        chdr.chunk_type = CHUNK_TYPE_CRC32;
        chdr.chunk_sz = 0;
        chdr.total_sz = hdr.chunk_hdr_sz + sizeof(uint32_t);
        _data.insert(_data.end(), chdrPtr, chdrPtr + sizeof(ChunkHeader));
        // [4/4] Add CRC32 chunk data
        uint32_t crc32 = 0;
        auto const *crc32Ptr = reinterpret_cast<const unsigned char *>(&crc32);
        _data.insert(_data.end(), crc32Ptr, crc32Ptr + sizeof(uint32_t));
    }
};

TEST_F(SparseTest, ReadPerfectlySizedHeader)
{
    char buf[1024];
    uint64_t bytesRead;
    uint64_t pos;
    buildDataHeaderProperSized();
    ASSERT_TRUE(sparseOpen());
    ASSERT_TRUE(sparseRead(buf, sizeof(buf), &bytesRead));
    ASSERT_EQ(bytesRead, 0);
    ASSERT_TRUE(sparseSeek(1000, SEEK_SET));
    ASSERT_TRUE(sparseSeek(1000, SEEK_CUR));
    ASSERT_TRUE(sparseTell(&pos));
    ASSERT_EQ(pos, 2000);
    ASSERT_TRUE(sparseRead(buf, sizeof(buf), &bytesRead));
    ASSERT_EQ(bytesRead, 0);
    ASSERT_TRUE(sparseClose());
}

TEST_F(SparseTest, OpenExtraSizedHeader)
{
    char buf[1024];
    uint64_t bytesRead;
    uint64_t pos;
    buildDataHeaderExtraSized();
    ASSERT_TRUE(sparseOpen());
    ASSERT_TRUE(sparseRead(buf, sizeof(buf), &bytesRead));
    ASSERT_EQ(bytesRead, 0);
    ASSERT_TRUE(sparseSeek(1000, SEEK_SET));
    ASSERT_TRUE(sparseSeek(1000, SEEK_CUR));
    ASSERT_TRUE(sparseTell(&pos));
    ASSERT_EQ(pos, 2000);
    ASSERT_TRUE(sparseRead(buf, sizeof(buf), &bytesRead));
    ASSERT_EQ(bytesRead, 0);
    ASSERT_TRUE(sparseClose());
}

TEST_F(SparseTest, OpenUnderSizedHeader)
{
    char buf[1024];
    uint64_t bytesRead;
    uint64_t pos;
    buildDataHeaderUnderSized();
    ASSERT_FALSE(sparseOpen());
    ASSERT_FALSE(sparseRead(buf, sizeof(buf), &bytesRead));
    ASSERT_FALSE(sparseSeek(1000, SEEK_SET));
    ASSERT_FALSE(sparseTell(&pos));
    ASSERT_FALSE(sparseClose());
}

TEST_F(SparseTest, ReadValidSparseFile)
{
    char expected[48] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'a', 'b', 'c', 'd', 'e', 'f',
        0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12,
        0x78, 0x56, 0x34, 0x12,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    char buf[1024];
    uint64_t bytesRead;
    uint64_t pos;
    buildDataCompleteValid();

    // Check that valid sparse header can be opened
    ASSERT_TRUE(sparseOpen());

    // Check that the entire file could be read and that the contents are
    // correct
    ASSERT_TRUE(sparseRead(buf, sizeof(buf), &bytesRead));
    ASSERT_EQ(bytesRead, 48);
    ASSERT_EQ(memcmp(buf, expected, 48), 0);

    // Check that partial read on chunk boundary works
    ASSERT_TRUE(sparseSeek(-16, SEEK_END));
    ASSERT_TRUE(sparseRead(buf, sizeof(buf), &bytesRead));
    ASSERT_EQ(bytesRead, 16);
    ASSERT_EQ(memcmp(buf, expected + 32, 16), 0);

    // Check that partial read not on chunk boundary works
    ASSERT_TRUE(sparseSeek(33, SEEK_SET));
    ASSERT_TRUE(sparseRead(buf, sizeof(buf), &bytesRead));
    ASSERT_EQ(bytesRead, 15);
    ASSERT_EQ(memcmp(buf, expected + 33, 15), 0);

    // Check that seeking past EOF is allowed and doing so returns no data
    ASSERT_TRUE(sparseSeek(1000, SEEK_SET));
    ASSERT_TRUE(sparseSeek(1000, SEEK_CUR));
    ASSERT_TRUE(sparseTell(&pos));
    ASSERT_EQ(pos, 2000);
    ASSERT_TRUE(sparseRead(buf, sizeof(buf), &bytesRead));
    ASSERT_EQ(bytesRead, 0);

    // Check that seeking to the end of the sparse file works
    ASSERT_TRUE(sparseSeek(0, SEEK_END));
    ASSERT_TRUE(sparseTell(&pos));
    ASSERT_EQ(pos, 48);

    ASSERT_TRUE(sparseClose());
}

TEST_F(SparseTest, ReadValidSparseFileNoSeek)
{
    char expected[48] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'a', 'b', 'c', 'd', 'e', 'f',
        0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12,
        0x78, 0x56, 0x34, 0x12,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    char buf[1024];
    char *ptr = buf;
    size_t remaining = sizeof(buf);
    uint64_t bytesRead;
    uint64_t total = 0;
    bool ret;
    buildDataCompleteValid();

    // Check that valid sparse header can be opened
    ASSERT_TRUE(sparseOpenNoSeek());

    // Check that the entire file could be read and that the contents are
    // correct. Read one byte at a time to test that a read while the sparse
    // file pointer is in the middle of a raw chunk works (this normally
    // requires seeking when a seek callback is provided).
    while (remaining > 0
            && (ret = sparseRead(ptr, 1, &bytesRead))
            && bytesRead > 0) {
        ptr += bytesRead;
        total += bytesRead;
        remaining -= bytesRead;
    }
    ASSERT_TRUE(ret);
    ASSERT_EQ(total, 48);
    ASSERT_EQ(remaining, sizeof(buf) - 48);
    ASSERT_EQ(bytesRead, 0);
    ASSERT_EQ(memcmp(buf, expected, 48), 0);

    // Check that seeking fails
    ASSERT_FALSE(sparseSeek(0, SEEK_SET));

    ASSERT_TRUE(sparseClose());
}
