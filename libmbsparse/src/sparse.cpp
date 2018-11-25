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

#include "mbsparse/sparse.h"

// For std::min()
#include <algorithm>
#include <iterator>
#include <vector>

#include <cassert>
#include <cinttypes>
#include <cstdint>
#include <cstring>

#include "mbcommon/algorithm.h"
#include "mbcommon/endian.h"
#include "mbcommon/file_util.h"
#include "mbcommon/string.h"

#include "mbsparse/sparse_error.h"

// Enable debug logging of headers, offsets, etc.?
#define SPARSE_DEBUG 0
// Enable debug logging of operations (warning! very verbose!)
#define SPARSE_DEBUG_OPER 0

#if SPARSE_DEBUG || SPARSE_DEBUG_OPER
#  include "mblog/logging.h"
#  define LOG_TAG "mbsparse/sparse"
#endif

#if SPARSE_DEBUG
#  define DEBUG(...) LOGD(__VA_ARGS__)
#else
#  define DEBUG(...)
#endif

#if SPARSE_DEBUG_OPER
#  define OPER(...) LOGD(__VA_ARGS__)
#else
#  define OPER(...)
#endif

namespace mb::sparse
{
using namespace mb::detail;
using namespace detail;

static void fix_sparse_header_byte_order(SparseHeader &header)
{
    header.magic = mb_le32toh(header.magic);
    header.major_version = mb_le16toh(header.major_version);
    header.minor_version = mb_le16toh(header.minor_version);
    header.file_hdr_sz = mb_le16toh(header.file_hdr_sz);
    header.chunk_hdr_sz = mb_le16toh(header.chunk_hdr_sz);
    header.blk_sz = mb_le32toh(header.blk_sz);
    header.total_blks = mb_le32toh(header.total_blks);
    header.total_chunks = mb_le32toh(header.total_chunks);
    header.image_checksum = mb_le32toh(header.image_checksum);
}

static void fix_chunk_header_byte_order(ChunkHeader &header)
{
    header.chunk_type = mb_le16toh(header.chunk_type);
    header.reserved1 = mb_le16toh(header.reserved1);
    header.chunk_sz = mb_le32toh(header.chunk_sz);
    header.total_sz = mb_le32toh(header.total_sz);
}

#if SPARSE_DEBUG
static void dump_sparse_header(const SparseHeader &header)
{
    DEBUG("Sparse header:");
    DEBUG("- magic:          0x%08" PRIx32, header.magic);
    DEBUG("- major_version:  0x%04" PRIx16, header.major_version);
    DEBUG("- minor_version:  0x%04" PRIx16, header.minor_version);
    DEBUG("- file_hdr_sz:    %" PRIu16 " (bytes)", header.file_hdr_sz);
    DEBUG("- chunk_hdr_sz:   %" PRIu16 " (bytes)", header.chunk_hdr_sz);
    DEBUG("- blk_sz:         %" PRIu32 " (bytes)", header.blk_sz);
    DEBUG("- total_blks:     %" PRIu32, header.total_blks);
    DEBUG("- total_chunks:   %" PRIu32, header.total_chunks);
    DEBUG("- image_checksum: 0x%08" PRIu32, header.image_checksum);
}

static const char * chunk_type_to_string(uint16_t chunk_type)
{
    switch (chunk_type) {
    case CHUNK_TYPE_RAW:
        return "Raw";
    case CHUNK_TYPE_FILL:
        return "Fill";
    case CHUNK_TYPE_DONT_CARE:
        return "Don't care";
    case CHUNK_TYPE_CRC32:
        return "CRC32";
    default:
        return "Unknown";
    }
}

static void dump_chunk_header(const ChunkHeader &header)
{
    DEBUG("Chunk header:");
    DEBUG("- chunk_type: 0x%04" PRIx16 " (%s)", header.chunk_type,
          chunk_type_to_string(header.chunk_type));
    DEBUG("- reserved1:  0x%04" PRIx16, header.reserved1);
    DEBUG("- chunk_sz:   %" PRIu32 " (blocks)", header.chunk_sz);
    DEBUG("- total_sz:   %" PRIu32 " (bytes)", header.total_sz);
}
#endif

/*! \cond INTERNAL */

struct OffsetComp
{
    bool operator()(uint64_t offset, const ChunkInfo &chunk) const
    {
        return offset < chunk.begin;
    }

    bool operator()(const ChunkInfo &chunk, uint64_t offset) const
    {
        return chunk.end <= offset;
    }
};

/*! \endcond */

/*!
 * \class SparseFile
 *
 * \brief Open Android sparse file image.
 */

/*!
 * \brief Construct unbound SparseFile.
 *
 * The File handle will not be bound to any file. One of the open functions will
 * need to be called to open a file.
 */
SparseFile::SparseFile()
    : File()
{
    clear();
}

/*!
 * \brief Open sparse file from File handle.
 *
 * Construct the file handle and open the file. Use is_open() to check if the
 * file was successfully opened.
 *
 * \sa open(File *)
 *
 * \param file File to open
 */
SparseFile::SparseFile(File *file)
    : SparseFile()
{
    (void) open(file);
}

SparseFile::~SparseFile()
{
    (void) close();
}

SparseFile::SparseFile(SparseFile &&other) noexcept
    : File(std::move(other))
    , m_file(other.m_file)
    , m_seekability(other.m_seekability)
    , m_expected_crc32(other.m_expected_crc32)
    , m_cur_src_offset(other.m_cur_src_offset)
    , m_cur_tgt_offset(other.m_cur_tgt_offset)
    , m_file_size(other.m_file_size)
    , m_shdr(other.m_shdr)
{
    auto distance = std::distance(other.m_chunks.begin(), other.m_chunk);
    m_chunks = std::move(other.m_chunks);
    m_chunk = m_chunks.begin() + distance;

    other.clear();
}

SparseFile & SparseFile::operator=(SparseFile &&rhs) noexcept
{
    File::operator=(std::move(rhs));

    m_file = rhs.m_file;
    m_seekability = rhs.m_seekability;
    m_expected_crc32 = rhs.m_expected_crc32;
    m_cur_src_offset = rhs.m_cur_src_offset;
    m_cur_tgt_offset = rhs.m_cur_tgt_offset;
    m_file_size = rhs.m_file_size;
    m_shdr = rhs.m_shdr;

    auto distance = std::distance(rhs.m_chunks.begin(), rhs.m_chunk);
    m_chunks.swap(rhs.m_chunks);
    m_chunk = m_chunks.begin() + distance;

    rhs.clear();

    return *this;
}

/*!
 * \brief Open sparse file from File handle.
 *
 * \note The SparseFile will *not* take ownership of \p file. The caller must
 *       ensure that it is properly closed and destroyed when it is no longer
 *       needed.
 *
 * \param file File to open
 *
 * \return Nothing if the file is successfully opened. Otherwise, the error
 *         code.
 */
oc::result<void> SparseFile::open(File *file)
{
    if (state() == FileState::New) {
        m_file = file;
    }

    return File::open();
}

/*!
 * \brief Get the expected size of the sparse file
 *
 * \return Size of the sparse file. The return value is undefined if the sparse
 *         file is not opened.
 */
uint64_t SparseFile::size()
{
    return m_file_size;
}

/*!
 * \brief Open sparse file for reading
 *
 * The file does not need to support any operation other than reading. If the
 * file supports forward skipping or random seeking, then those operations will
 * be used to speed up the read process. If the file supports random seeking,
 * then the sparse file will also support random reads.
 *
 * The following test will be run to determine if the file supports forward
 * skipping.
 *
 * \code{.cpp}
 * !!file.seek(0, SEEK_CUR) == true;
 * // Forward skipping unsupported if FileError::UnsupportedSeek is returned
 * \endcode
 *
 * The following test will be run (following the forward skipping test) to
 * determine if the file supports random seeking.
 *
 * \code{.cpp}
 * !!file.read(buf, 1) == true;
 * !!file.seek(-1, SEEK_CUR) == true;
 * // Random seeking unsupported if FileError::UnsupportedSeek is returned
 * \endcode
 *
 * \note This function will fail if the file handle is not open.
 *
 * \pre The caller should position the file at the beginning of the sparse file
 *      data. SparseFile will use that as the base offset and only perform
 *      relative seeks. This allows for opening sparse files where the data
 *      isn't at the beginning of the file.
 *
 * \return Nothing if the sparse file is successfully opened. Otherwise, the
 *         error code.
 */
oc::result<void> SparseFile::on_open()
{
    if (!m_file->is_open()) {
        DEBUG("Underlying file is not open");
        return FileError::InvalidState;
    }

    m_seekability = Seekability::CanRead;

    auto seek_ret = m_file->seek(0, SEEK_CUR);
    if (seek_ret) {
        DEBUG("File supports forward skipping");
        m_seekability = Seekability::CanSkip;
    } else if (seek_ret.error() != FileErrorC::Unsupported) {
        return seek_ret.as_failure();
    }

    unsigned char first_byte;

    OUTCOME_TRY(n, m_file->read(&first_byte, 1));
    if (n != 1) {
        DEBUG("Failed to read first byte of file");
        return FileError::UnexpectedEof;
    }
    m_cur_src_offset += n;

    seek_ret = m_file->seek(-static_cast<int64_t>(n), SEEK_CUR);
    if (seek_ret) {
        DEBUG("File supports random seeking");
        m_seekability = Seekability::CanSeek;
        m_cur_src_offset -= n;
        n = 0;
    } else if (seek_ret.error() != FileErrorC::Unsupported) {
        return seek_ret.as_failure();
    }

    return process_sparse_header(&first_byte, n);
}

/*!
 * \brief Close opened sparse file
 *
 * \note If the sparse file is open, then no matter what value is returned, the
 *       sparse file will be closed.
 *
 * \return Always returns nothing
 */
oc::result<void> SparseFile::on_close()
{
    // Reset to allow opening another file
    clear();

    return oc::success();
}

/*!
 * \brief Read sparse file
 *
 * This function will read the specified amount of bytes from the source file.
 * If this function returns a size count less than \p size, then the end of the
 * sparse file (EOF) has been reached. If any error occurs, the function will
 * return an error code and any further attempts to read or seek the sparse file
 * may produce invalid or unexpected results. It is not undefined behavior in
 * the C++ sense of the term.
 *
 * Some examples of failures include:
 * * Source reaching EOF before the end of the sparse file is reached
 * * Ran out of sparse chunks despite the main sparse header promising more
 *   chunks
 * * Chunk header is invalid
 *
 * \param buf Buffer to read data into
 * \param size Number of bytes to read
 *
 * \return Number of bytes read if the specified number of bytes were
 *         successfully read. Otherwise, the error code.
 */
oc::result<size_t> SparseFile::on_read(void *buf, size_t size)
{
    OPER("read(buf, %" MB_PRIzu ")", size);

    uint64_t total_read = 0;

    while (size > 0) {
        OUTCOME_TRYV(move_to_chunk(m_cur_tgt_offset));

        if (m_chunk == m_chunks.end()) {
            OPER("Reached EOF");
            break;
        }

        assert(m_cur_tgt_offset >= m_chunk->begin
                && m_cur_tgt_offset < m_chunk->end);

        // Read until the end of the current chunk
        uint64_t n_read = 0;
        uint64_t to_read = std::min<uint64_t>(
                size, m_chunk->end - m_cur_tgt_offset);

        OPER("Reading %" PRIu64 " bytes from chunk %" MB_PRIzu,
             to_read, m_chunk - m_chunks.begin());

        switch (m_chunk->type) {
        case CHUNK_TYPE_RAW: {
            // Figure out how much to seek in the input data
            uint64_t diff = m_cur_tgt_offset - m_chunk->begin;
            OPER("Raw data is %" PRIu64 " bytes into the raw chunk", diff);

            uint64_t raw_src_offset = m_chunk->raw_begin + diff;
            if (raw_src_offset != m_cur_src_offset) {
                assert(m_seekability == Seekability::CanSeek);

                int64_t seek_offset;
                if (raw_src_offset < m_cur_src_offset) {
                    seek_offset = -static_cast<int64_t>(
                            m_cur_src_offset - raw_src_offset);
                } else {
                    seek_offset = static_cast<int64_t>(
                            raw_src_offset - m_cur_src_offset);
                }

                OUTCOME_TRYV(wseek(seek_offset));
            }

            OUTCOME_TRYV(wread(buf, static_cast<size_t>(to_read)));

            n_read = to_read;
            break;
        }
        case CHUNK_TYPE_FILL: {
            static_assert(sizeof(m_chunk->fill_val) == sizeof(uint32_t),
                          "Mismatched fill_val size");
            auto shift = (m_cur_tgt_offset - m_chunk->begin) % sizeof(uint32_t);
            uint32_t fill_val = mb_htole32(m_chunk->fill_val);
            unsigned char shifted[4];
            for (size_t i = 0; i < sizeof(uint32_t); ++i) {
                shifted[i] = reinterpret_cast<unsigned char *>(&fill_val)
                        [(i + shift) % sizeof(uint32_t)];
            }
            unsigned char *temp_buf = reinterpret_cast<unsigned char *>(buf);
            while (to_read > 0) {
                size_t to_write = std::min<size_t>(
                        sizeof(shifted), static_cast<size_t>(to_read));
                memcpy(temp_buf, &shifted, to_write);
                n_read += to_write;
                to_read -= to_write;
                temp_buf += to_write;
            }
            break;
        }
        case CHUNK_TYPE_DONT_CARE:
            std::fill_n(reinterpret_cast<unsigned char *>(buf), to_read, 0);
            n_read = to_read;
            break;
        default:
            MB_UNREACHABLE("Invalid chunk type: %" PRIu16, m_chunk->type);
        }

        OPER("Read %" PRIu64 " bytes", n_read);
        total_read += n_read;
        m_cur_tgt_offset += n_read;
        size -= static_cast<size_t>(n_read);
        buf = reinterpret_cast<unsigned char *>(buf) + n_read;
    }

    return static_cast<size_t>(total_read);
}

/*!
 * \brief Seek sparse file
 *
 * \p whence takes the same \a SEEK_SET, \a SEEK_CUR, and \a SEEK_END values as
 * \a lseek() in `\<stdio.h\>`.
 *
 * \note Seeking will only work if the underlying file handle supports seeking.
 *
 * \param offset Offset to seek
 * \param whence \a SEEK_SET, \a SEEK_CUR, or \a SEEK_END
 *
 * \return New offset of sparse file if the seeking was successful. Otherwise,
 *         the error code.
 */
oc::result<uint64_t> SparseFile::on_seek(int64_t offset, int whence)
{
    OPER("seek(%" PRId64 ", %d)", offset, whence);

    if (m_seekability != Seekability::CanSeek) {
        DEBUG("Underlying file does not support seeking");
        return FileError::UnsupportedSeek;
    }

    uint64_t new_offset;
    switch (whence) {
    case SEEK_SET:
        if (offset < 0) {
            DEBUG("Cannot seek to negative offset");
            return FileError::ArgumentOutOfRange;
        }
        new_offset = static_cast<uint64_t>(offset);
        break;
    case SEEK_CUR:
        if ((offset < 0 && static_cast<uint64_t>(-offset) > m_cur_tgt_offset)
                || (offset > 0 && m_cur_tgt_offset
                        >= UINT64_MAX - static_cast<uint64_t>(offset))) {
            DEBUG("Offset overflows uint64_t");
            return FileError::IntegerOverflow;
        }
        new_offset = m_cur_tgt_offset + static_cast<uint64_t>(offset);
        break;
    case SEEK_END:
        if ((offset < 0 && static_cast<uint64_t>(-offset) > m_file_size)
                || (offset > 0 && m_file_size
                        >= UINT64_MAX - static_cast<uint64_t>(offset))) {
            DEBUG("Offset overflows uint64_t");
            return FileError::IntegerOverflow;
        }
        new_offset = m_file_size + static_cast<uint64_t>(offset);
        break;
    default:
        MB_UNREACHABLE("Invalid seek whence: %d", whence);
    }

    OUTCOME_TRYV(move_to_chunk(new_offset));

    // May move past EOF, which is okay (mimics lseek behavior), but read()
    m_cur_tgt_offset = new_offset;

    return new_offset;
}

void SparseFile::clear()
{
    m_file = nullptr;
    m_expected_crc32 = 0;
    m_cur_src_offset = 0;
    m_cur_tgt_offset = 0;
    m_file_size = 0;
    m_chunks.clear();
    m_chunk = m_chunks.end();
}

oc::result<void> SparseFile::wread(void *buf, size_t size)
{
    OUTCOME_TRYV(file_read_exact(*m_file, buf, size));

    m_cur_src_offset += size;
    return oc::success();
}

oc::result<void> SparseFile::wseek(int64_t offset)
{
    OUTCOME_TRYV(m_file->seek(offset, SEEK_CUR));

    m_cur_src_offset = static_cast<uint64_t>(
            static_cast<int64_t>(m_cur_src_offset) + offset);
    return oc::success();
}

/*!
 * \brief Skip a certain amount of bytes
 *
 * \param bytes Number of bytes to skip
 *
 * \return Nothing if the bytes are successfully skipped. Otherwise, the error
 *         code.
 */
oc::result<void> SparseFile::skip_bytes(uint64_t bytes)
{
    if (bytes == 0) {
        return oc::success();
    }

    switch (m_seekability) {
    case Seekability::CanSeek:
    case Seekability::CanSkip:
        return wseek(static_cast<int64_t>(bytes));
    case Seekability::CanRead:
        OUTCOME_TRY(discarded, file_read_discard(*m_file, bytes));

        if (discarded != bytes) {
            DEBUG("Reached EOF when skipping bytes");
            return FileError::UnexpectedEof;
        }

        m_cur_src_offset += bytes;
        return oc::success();
    }

    MB_UNREACHABLE("Invalid seekability: %d", static_cast<int>(m_seekability));
}

/*!
 * \brief Read and verify sparse header
 *
 * Thie function will check the following properties:
 * - Magic field matches expected value
 * - Major version is 1
 * - The sparse header size field is at least the expected size of the sparse
 *   header structure
 * - The chunk header size field is at least the expected size of the chunk
 *   header structure
 *
 * The value of the minor version field is ignored.
 *
 * \note This does not process any of the chunk headers. The chunk headers are
 *       processed on-the-fly when reading the sparse file.
 *
 * \pre The file position should be at the beginning of the sparse header.
 * \post The file position will be advanced to the byte after the sparse header.
 *       If the sparse header size, as specified by the \a file_hdr_sz field, is
 *       larger than the expected size, the extra bytes will be skipped as well.
 *
 * \param preread_data Data already read from the file (for seek checks)
 * \param preread_size Size of data already read from the file (must be less
 *                     than `sizeof(SparseHeader)`)
 *
 * \return Nothing if the sparse header is successfully read. Otherwise, the
 *         error code.
 */
oc::result<void> SparseFile::process_sparse_header(const void *preread_data,
                                                   size_t preread_size)
{
    assert(preread_size < sizeof(m_shdr));

    // Read header
    memcpy(&m_shdr, preread_data, preread_size);
    OUTCOME_TRYV(wread(reinterpret_cast<char *>(&m_shdr) + preread_size,
                 sizeof(m_shdr) - preread_size));

    fix_sparse_header_byte_order(m_shdr);

#if SPARSE_DEBUG
    dump_sparse_header(m_shdr);
#endif

    // Check magic bytes
    if (m_shdr.magic != SPARSE_HEADER_MAGIC) {
        DEBUG("Expected magic to be %08x, but got %08x",
              SPARSE_HEADER_MAGIC, m_shdr.magic);
        return SparseFileError::InvalidSparseMagic;
    }

    // Check major version
    if (m_shdr.major_version != SPARSE_HEADER_MAJOR_VER) {
        DEBUG("Expected major version to be %u, but got %u",
              SPARSE_HEADER_MAJOR_VER, m_shdr.major_version);
        return SparseFileError::InvalidSparseMajorVersion;
    }

    // Check sparse header size field
    if (m_shdr.file_hdr_sz < sizeof(SparseHeader)) {
        DEBUG("Expected sparse header size to be at least %" MB_PRIzu
              ", but have %" PRIu32, sizeof(m_shdr), m_shdr.file_hdr_sz);
        return SparseFileError::InvalidSparseHeaderSize;
    }

    // Check chunk header size field
    if (m_shdr.chunk_hdr_sz < sizeof(ChunkHeader)) {
        DEBUG("Expected chunk header size to be at least %" MB_PRIzu
              ", but have %" PRIu32, sizeof(SparseHeader), m_shdr.chunk_hdr_sz);
        return SparseFileError::InvalidChunkHeaderSize;
    }

    // Skip any extra bytes in the file header
    OUTCOME_TRYV(skip_bytes(m_shdr.file_hdr_sz - sizeof(SparseHeader)));

    m_file_size = static_cast<uint64_t>(m_shdr.total_blks) * m_shdr.blk_sz;

    return oc::success();
}

/*!
 * \brief Read and verify raw chunk header
 *
 * \param chdr Chunk header
 * \param tgt_offset Offset of the output file
 *
 * \return ChunkInfo if the chunk header is valid. Otherwise, the error code.
 */
oc::result<ChunkInfo>
SparseFile::process_raw_chunk(const ChunkHeader &chdr, uint64_t tgt_offset)
{
    uint32_t data_size = chdr.total_sz - m_shdr.chunk_hdr_sz;
    uint64_t chunk_size = static_cast<uint64_t>(chdr.chunk_sz) * m_shdr.blk_sz;

    if (data_size != chunk_size) {
        DEBUG("Number of data blocks (%" PRIu32 ") does not match"
              " number of expected blocks (%" PRIu32 ")",
              data_size / m_shdr.blk_sz, chdr.chunk_sz);
        return SparseFileError::InvalidRawChunk;
    }

    ChunkInfo ci;

    ci.type = chdr.chunk_type;
    ci.begin = tgt_offset;
    ci.end = tgt_offset + data_size;
    ci.src_begin = m_cur_src_offset - m_shdr.chunk_hdr_sz;
    ci.src_end = m_cur_src_offset + data_size;
    ci.raw_begin = m_cur_src_offset;
    ci.raw_end = m_cur_src_offset + data_size;

    return std::move(ci);
}

/*!
 * \brief Read and verify fill chunk header
 *
 * This function will check the following properties:
 *   * Chunk data size is 4 bytes (`sizeof(uint32_t)`)
 *
 * \param chdr Chunk header
 * \param tgt_offset Offset of the output file
 *
 * \return ChunkInfo if the chunk header is valid. Otherwise, the error code.
 */
oc::result<ChunkInfo>
SparseFile::process_fill_chunk(const ChunkHeader &chdr, uint64_t tgt_offset)
{
    uint32_t data_size = chdr.total_sz - m_shdr.chunk_hdr_sz;
    uint64_t chunk_size = static_cast<uint64_t>(chdr.chunk_sz) * m_shdr.blk_sz;
    uint32_t fill_val;

    if (data_size != sizeof(fill_val)) {
        DEBUG("Data size (%" PRIu32 ") does not match size of 32-bit integer",
              data_size);
        return SparseFileError::InvalidFillChunk;
    }

    uint64_t src_begin = m_cur_src_offset - m_shdr.chunk_hdr_sz;

    OUTCOME_TRYV(wread(&fill_val, sizeof(fill_val)));

    uint64_t src_end = m_cur_src_offset;

    ChunkInfo ci;

    ci.type = chdr.chunk_type;
    ci.begin = tgt_offset;
    ci.end = tgt_offset + chunk_size;
    ci.src_begin = src_begin;
    ci.src_end = src_end;
    ci.fill_val = fill_val;

    return std::move(ci);
}

/*!
 * \brief Read and verify skip chunk header
 *
 * This function will check the following properties:
 *   * Chunk data size is 0 bytes
 *
 * \param chdr Chunk header
 * \param tgt_offset Offset of the output file
 *
 * \return ChunkInfo if the chunk header is valid. Otherwise, the error code.
 */
oc::result<ChunkInfo>
SparseFile::process_skip_chunk(const ChunkHeader &chdr, uint64_t tgt_offset)
{
    uint32_t data_size = chdr.total_sz - m_shdr.chunk_hdr_sz;
    uint64_t chunk_size = static_cast<uint64_t>(chdr.chunk_sz) * m_shdr.blk_sz;

    if (data_size != 0) {
        DEBUG("Data size (%" PRIu32 ") is not 0", data_size);
        return SparseFileError::InvalidSkipChunk;
    }

    ChunkInfo ci;

    ci.type = chdr.chunk_type;
    ci.begin = tgt_offset;
    ci.end = tgt_offset + chunk_size;
    ci.src_begin = m_cur_src_offset - m_shdr.chunk_hdr_sz;
    ci.src_end = m_cur_src_offset + data_size;

    return std::move(ci);
}

/*!
 * \brief Read and verify CRC32 chunk header
 *
 * This function will check the following properties:
 *   * Chunk data size is 4 bytes (`sizeof(uint32_t)`)
 *
 * \param chdr Chunk header
 * \param tgt_offset Offset of the output file
 *
 * \return ChunkInfo if the chunk header is valid. Otherwise, the error code.
 */
oc::result<ChunkInfo>
SparseFile::process_crc32_chunk(const ChunkHeader &chdr, uint64_t tgt_offset)
{
    uint32_t data_size = chdr.total_sz - m_shdr.chunk_hdr_sz;
    uint64_t chunk_size = static_cast<uint64_t>(chdr.chunk_sz) * m_shdr.blk_sz;
    uint32_t crc32;

    if (chunk_size != 0) {
        DEBUG("Chunk data size (%" PRIu64 ") is not 0", chunk_size);
        return SparseFileError::InvalidCrc32Chunk;
    }

    if (data_size != sizeof(crc32)) {
        DEBUG("Data size (%" PRIu32 ") does not match size of 32-bit integer",
              data_size);
        return SparseFileError::InvalidCrc32Chunk;
    }

    OUTCOME_TRYV(wread(&crc32, sizeof(crc32)));

    m_expected_crc32 = mb_le32toh(crc32);

    ChunkInfo ci;

    ci.type = chdr.chunk_type;
    ci.begin = tgt_offset;
    ci.end = tgt_offset;
    ci.src_begin = tgt_offset - m_shdr.chunk_hdr_sz;
    ci.src_end = tgt_offset + data_size;

    return std::move(ci);
}

/*!
 * \brief Read and verify chunk header
 *
 * This function will check the following properties:
 *   * Number of chunk data blocks matches expected number of blocks
 *
 * \pre The file position should be at the byte immediately after the chunk
 *      header
 * \pre \p tgt_offset should be set to the beginning of the range in the output
 *      file that this chunk represents
 * \post If the chunk is a raw chunk, the source file position will be advanced
 *       to the first byte of the raw data. Otherwise, the source file position
 *       will advanced to the byte after the entire chunk.
 * \post \a cur_src_offset will be set to the new source file position
 *
 * \param chdr Chunk header
 * \param tgt_offset Offset of the output file
 *
 * \return Nothing if the chunk header is valid. Otherwise, the error code.
 */
oc::result<ChunkInfo>
SparseFile::process_chunk(const ChunkHeader &chdr, uint64_t tgt_offset)
{
    if (chdr.total_sz < m_shdr.chunk_hdr_sz) {
        DEBUG("Total chunk size (%" PRIu32 ") smaller than chunk header size",
              chdr.total_sz);
        return SparseFileError::InvalidChunkSize;
    }

    switch (chdr.chunk_type) {
    case CHUNK_TYPE_RAW:
        return process_raw_chunk(chdr, tgt_offset);
    case CHUNK_TYPE_FILL:
        return process_fill_chunk(chdr, tgt_offset);
    case CHUNK_TYPE_DONT_CARE:
        return process_skip_chunk(chdr, tgt_offset);
    case CHUNK_TYPE_CRC32:
        return process_crc32_chunk(chdr, tgt_offset);
    default:
        DEBUG("Unknown chunk type: %u", chdr.chunk_type);
        return SparseFileError::InvalidChunkType;
    }
}

/*!
 * \brief Move to chunk that is responsible for the specified offset
 *
 * \note This function only returns a failure code if a file operation fails. To
 *       check if a matching chunk is found, use `chunk != chunks.end()`.
 *
 * \return Nothing unless an error occurs
 */
oc::result<void> SparseFile::move_to_chunk(uint64_t offset)
{
    // No action needed if the offset is in the current chunk
    if (m_chunk != m_chunks.end()
            && offset >= m_chunk->begin && offset < m_chunk->end) {
        return oc::success();
    }

    // If the offset is in the current range of chunks, then do a binary search
    // to find the right chunk
    if (!m_chunks.empty() && offset < m_chunks.back().end) {
        m_chunk = binary_find(m_chunks.begin(), m_chunks.end(), offset,
                              OffsetComp());
        if (m_chunk != m_chunks.end()) {
            return oc::success();
        }
    }

    // We don't have the chunk, so read until we find it
    m_chunk = m_chunks.end();
    while (m_chunks.size() < m_shdr.total_chunks) {
        size_t chunk_num = m_chunks.size();
        (void) chunk_num;

        DEBUG("Reading next chunk (#%" MB_PRIzu ")", chunk_num);

        // Get starting source and output offsets for chunk
        uint64_t src_begin;
        uint64_t tgt_begin;

        // First chunk starts after the sparse header. Remaining chunks are
        // contiguous.
        if (m_chunks.empty()) {
            src_begin = m_shdr.file_hdr_sz;
            tgt_begin = 0;
        } else {
            src_begin = m_chunks.back().src_end;
            tgt_begin = m_chunks.back().end;
        }

        // Skip to src_begin
        if (src_begin < m_cur_src_offset) {
            DEBUG("Internal error: src_begin (%" PRIu64 ")"
                  " < cur_src_offset (%" PRIu64 ")",
                  src_begin, m_cur_src_offset);
            return SparseFileError::InternalError;
        }

        auto skip_ret = skip_bytes(src_begin - m_cur_src_offset);
        if (!skip_ret) {
            DEBUG("Failed to skip to chunk #%" MB_PRIzu ": %s",
                  chunk_num, skip_ret.error().message().c_str());
            return skip_ret.as_failure();
        }

        ChunkHeader chdr;

        auto read_ret = wread(&chdr, sizeof(chdr));
        if (!read_ret) {
            DEBUG("Failed to read chunk header for chunk %" MB_PRIzu ": %s",
                  chunk_num, read_ret.error().message().c_str());
            return read_ret.as_failure();
        }

        fix_chunk_header_byte_order(chdr);

#if SPARSE_DEBUG
        dump_chunk_header(chdr);
#endif

        // Skip any extra bytes in the chunk header. process_sparse_header()
        // checks the size to make sure that the value won't underflow.
        skip_ret = skip_bytes(m_shdr.chunk_hdr_sz - sizeof(chdr));
        if (!skip_ret) {
            DEBUG("Failed to skip extra bytes in chunk #%" MB_PRIzu
                  "'s header: %s", chunk_num,
                  skip_ret.error().message().c_str());
            return skip_ret.as_failure();
        }

        OUTCOME_TRY(chunk_info, process_chunk(chdr, tgt_begin));

        OPER("Chunk #%" MB_PRIzu " covers source range (%" PRIu64 " - %" PRIu64 ")",
             chunk_num, chunk_info.src_begin, chunk_info.src_end);
        OPER("Chunk #%" MB_PRIzu " covers output range (%" PRIu64 " - %" PRIu64 ")",
             chunk_num, chunk_info.begin, chunk_info.end);

        // Make sure the chunk does not end after the header-specified file size
        if (chunk_info.end > m_file_size) {
            DEBUG("Chunk #%" MB_PRIzu " ends (%" PRIu64 ") after the file size "
                  "specified in the sparse header (%" PRIu64 ")",
                  chunk_num, chunk_info.end, m_file_size);
            return SparseFileError::InvalidChunkBounds;
        }

        m_chunks.push_back(std::move(chunk_info));

        // If we just read the last chunk, make sure it ends at the same
        // position as specified in the sparse header
        if (m_chunks.size() == m_shdr.total_chunks
                && m_chunks.back().end != m_file_size) {
            DEBUG("Last chunk does not end (%" PRIu64 ") at position"
                  " specified by sparse header (%" PRIu64 ")",
                  m_chunks.back().end, m_file_size);
            return SparseFileError::InvalidChunkBounds;
        }

        if (offset >= m_chunks.back().begin && offset < m_chunks.back().end) {
            m_chunk = m_chunks.end() - 1;
            break;
        }

        // Make sure iterator is not invalidated
        m_chunk = m_chunks.end();
    }

    return oc::success();
}

}
