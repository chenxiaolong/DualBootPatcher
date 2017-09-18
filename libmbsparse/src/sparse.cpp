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

#include <vector>

#include <cassert>
#include <cinttypes>
#include <cstdint>
#include <cstring>

#include "mbcommon/algorithm.h"
#include "mbcommon/endian.h"
#include "mbcommon/file_util.h"
#include "mbcommon/string.h"

#include "mbsparse/sparse_p.h"

// Enable debug logging of headers, offsets, etc.?
#define SPARSE_DEBUG 0
// Enable debug logging of operations (warning! very verbose!)
#define SPARSE_DEBUG_OPER 0

#if SPARSE_DEBUG || SPARSE_DEBUG_OPER
#  include "mblog/logging.h"
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

namespace mb
{
namespace sparse
{

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

SparseFilePrivate::SparseFilePrivate(SparseFile *sf)
    : _pub_ptr(sf)
{
    clear();
}

void SparseFilePrivate::clear()
{
    file = nullptr;
    expected_crc32 = 0;
    cur_src_offset = 0;
    cur_tgt_offset = 0;
    file_size = 0;
    chunks.clear();
    chunk = chunks.end();
}

bool SparseFilePrivate::wread(void *buf, size_t size)
{
    MB_PUBLIC(SparseFile);
    size_t bytes_read;

    if (!file_read_fully(*file, buf, size, bytes_read)) {
        pub->set_error(file->error(), "Failed to read file: %s",
                       file->error_string().c_str());
        return false;
    }

    if (bytes_read != size) {
        pub->set_error(file->error(), "Requested %" MB_PRIzu
                       " bytes, but only read %" MB_PRIzu " bytes",
                       size, bytes_read);
        pub->set_fatal(true);
        return false;
    }

    cur_src_offset += bytes_read;
    return true;
}

bool SparseFilePrivate::wseek(int64_t offset)
{
    MB_PUBLIC(SparseFile);

    if (!file->seek(offset, SEEK_CUR, nullptr)) {
        pub->set_error(file->error(), "Failed to seek file: %s",
                       file->error_string().c_str());
        return false;
    }

    cur_src_offset = static_cast<uint64_t>(
            static_cast<int64_t>(cur_src_offset) + offset);
    return true;
}

/*!
 * \brief Skip a certain amount of bytes
 *
 * \param bytes Number of bytes to skip
 *
 * \return Whether the bytes are successfully skipped
 */
bool SparseFilePrivate::skip_bytes(uint64_t bytes)
{
    MB_PUBLIC(SparseFile);

    if (bytes == 0) {
        return true;
    }

    switch (seekability) {
    case Seekability::CAN_SEEK:
    case Seekability::CAN_SKIP:
        return wseek(static_cast<int64_t>(bytes));
    case Seekability::CAN_READ:
        uint64_t discarded;
        if (!file_read_discard(*file, bytes, discarded)) {
            pub->set_error(file->error(), "Failed to read and discard data: %s",
                           file->error_string().c_str());
            return false;
        } else if (discarded != bytes) {
            pub->set_error(file->error(), "Reached EOF when skipping bytes: %s",
                           file->error_string().c_str());
            pub->set_fatal(true);
            return false;
        }
        return true;
    }

    // Unreached
    pub->set_fatal(true);
    return false;
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
 * \return Whether the sparse header is successfully read
 */
bool SparseFilePrivate::process_sparse_header(const void *preread_data,
                                              size_t preread_size)
{
    MB_PUBLIC(SparseFile);

    assert(preread_size < sizeof(shdr));

    // Read header
    memcpy(&shdr, preread_data, preread_size);
    if (!wread(reinterpret_cast<char *>(&shdr) + preread_size,
               sizeof(shdr) - preread_size)) {
        return false;
    }

    fix_sparse_header_byte_order(shdr);

#if SPARSE_DEBUG
    dump_sparse_header(shdr);
#endif

    // Check magic bytes
    if (shdr.magic != SPARSE_HEADER_MAGIC) {
        pub->set_error(make_error_code(FileError::BadFileFormat),
                       "Expected magic to be %08x, but got %08x",
                       SPARSE_HEADER_MAGIC, shdr.magic);
        return false;
    }

    // Check major version
    if (shdr.major_version != SPARSE_HEADER_MAJOR_VER) {
        pub->set_error(make_error_code(FileError::BadFileFormat),
                       "Expected major version to be %u, but got %u",
                       SPARSE_HEADER_MAJOR_VER, shdr.major_version);
        return false;
    }

    // Check sparse header size field
    if (shdr.file_hdr_sz < sizeof(SparseHeader)) {
        pub->set_error(make_error_code(FileError::BadFileFormat),
                       "Expected sparse header size to be at least %" MB_PRIzu
                       ", but have %" PRIu32, sizeof(shdr), shdr.file_hdr_sz);
        return false;
    }

    // Check chunk header size field
    if (shdr.chunk_hdr_sz < sizeof(ChunkHeader)) {
        pub->set_error(make_error_code(FileError::BadFileFormat),
                       "Expected chunk header size to be at least %" MB_PRIzu
                       ", but have %" PRIu32, sizeof(SparseHeader),
                       shdr.chunk_hdr_sz);
        return false;
    }

    // Skip any extra bytes in the file header
    if (!skip_bytes(shdr.file_hdr_sz - sizeof(SparseHeader))) {
        return false;
    }

    file_size = static_cast<uint64_t>(shdr.total_blks) * shdr.blk_sz;

    return true;
}

/*!
 * \brief Read and verify raw chunk header
 *
 * \param[in] chdr Chunk header
 * \param[in] tgt_offset Offset of the output file
 * \param[out] chunk_out ChunkInfo to store chunk info
 *
 * \return Whether the chunk header is valid
 */
bool SparseFilePrivate::process_raw_chunk(const ChunkHeader &chdr,
                                          uint64_t tgt_offset,
                                          ChunkInfo &chunk_out)
{
    MB_PUBLIC(SparseFile);

    uint32_t data_size = chdr.total_sz - shdr.chunk_hdr_sz;
    uint64_t chunk_size = static_cast<uint64_t>(chdr.chunk_sz) * shdr.blk_sz;

    if (data_size != chunk_size) {
        pub->set_error(make_error_code(FileError::BadFileFormat),
                       "Number of data blocks (%" PRIu32 ") does not match"
                       " number of expected blocks (%" PRIu32 ")",
                       data_size / shdr.blk_sz, chdr.chunk_sz);
        return false;
    }

    chunk_out.type = chdr.chunk_type;
    chunk_out.begin = tgt_offset;
    chunk_out.end = tgt_offset + data_size;
    chunk_out.src_begin = cur_src_offset - shdr.chunk_hdr_sz;
    chunk_out.src_end = cur_src_offset + data_size;
    chunk_out.raw_begin = cur_src_offset;
    chunk_out.raw_end = cur_src_offset + data_size;

    return true;
}

/*!
 * \brief Read and verify fill chunk header
 *
 * This function will check the following properties:
 *   * Chunk data size is 4 bytes (`sizeof(uint32_t)`)
 *
 * \param[in] chdr Chunk header
 * \param[in] tgt_offset Offset of the output file
 * \param[out] chunk_out ChunkInfo to store chunk info
 *
 * \return Whether the chunk header is valid
 */
bool SparseFilePrivate::process_fill_chunk(const ChunkHeader &chdr,
                                           uint64_t tgt_offset,
                                           ChunkInfo &chunk_out)
{
    MB_PUBLIC(SparseFile);

    uint32_t data_size = chdr.total_sz - shdr.chunk_hdr_sz;
    uint64_t chunk_size = static_cast<uint64_t>(chdr.chunk_sz) * shdr.blk_sz;
    uint32_t fill_val;

    if (data_size != sizeof(fill_val)) {
        pub->set_error(make_error_code(FileError::BadFileFormat),
                       "Data size (%" PRIu32 ") does not match size of 32-bit "
                       "integer", data_size);
        return false;
    }

    uint64_t src_begin = cur_src_offset - shdr.chunk_hdr_sz;

    if (!wread(&fill_val, sizeof(fill_val))) {
        return false;
    }

    uint64_t src_end = cur_src_offset;

    chunk_out.type = chdr.chunk_type;
    chunk_out.begin = tgt_offset;
    chunk_out.end = tgt_offset + chunk_size;
    chunk_out.src_begin = src_begin;
    chunk_out.src_end = src_end;
    chunk_out.fill_val = fill_val;

    return true;
}

/*!
 * \brief Read and verify skip chunk header
 *
 * This function will check the following properties:
 *   * Chunk data size is 0 bytes
 *
 * \param[in] chdr Chunk header
 * \param[in] tgt_offset Offset of the output file
 * \param[out] chunk_out ChunkInfo to store chunk info
 *
 * \return Whether the chunk header is valid
 */
bool SparseFilePrivate::process_skip_chunk(const ChunkHeader &chdr,
                                           uint64_t tgt_offset,
                                           ChunkInfo &chunk_out)
{
    MB_PUBLIC(SparseFile);

    uint32_t data_size = chdr.total_sz - shdr.chunk_hdr_sz;
    uint64_t chunk_size = static_cast<uint64_t>(chdr.chunk_sz) * shdr.blk_sz;

    if (data_size != 0) {
        pub->set_error(make_error_code(FileError::BadFileFormat),
                       "Data size (%" PRIu32 ") is not 0", data_size);
        return false;
    }

    chunk_out.type = chdr.chunk_type;
    chunk_out.begin = tgt_offset;
    chunk_out.end = tgt_offset + chunk_size;
    chunk_out.src_begin = cur_src_offset - shdr.chunk_hdr_sz;
    chunk_out.src_end = cur_src_offset + data_size;

    return true;
}

/*!
 * \brief Read and verify CRC32 chunk header
 *
 * This function will check the following properties:
 *   * Chunk data size is 4 bytes (`sizeof(uint32_t)`)
 *
 * \param[in] chdr Chunk header
 * \param[in] tgt_offset Offset of the output file
 * \param[out] chunk_out ChunkInfo to store chunk info
 *
 * \return Whether the chunk header is valid
 */
bool SparseFilePrivate::process_crc32_chunk(const ChunkHeader &chdr,
                                            uint64_t tgt_offset,
                                            ChunkInfo &chunk_out)
{
    MB_PUBLIC(SparseFile);
    (void) tgt_offset;

    uint32_t data_size = chdr.total_sz - shdr.chunk_hdr_sz;
    uint64_t chunk_size = static_cast<uint64_t>(chdr.chunk_sz) * shdr.blk_sz;
    uint32_t crc32;

    if (chunk_size != 0) {
        pub->set_error(make_error_code(FileError::BadFileFormat),
                       "Chunk data size (%" PRIu64 ") is not 0", chunk_size);
        return false;
    }

    if (data_size != sizeof(crc32)) {
        pub->set_error(make_error_code(FileError::BadFileFormat),
                       "Data size (%" PRIu32 ") does not match size of 32-bit"
                       " integer", data_size);
        return false;
    }

    if (!wread(&crc32, sizeof(crc32))) {
        return false;
    }

    expected_crc32 = mb_le32toh(crc32);

    chunk_out.type = chdr.chunk_type;
    chunk_out.begin = tgt_offset;
    chunk_out.end = tgt_offset;
    chunk_out.src_begin = tgt_offset - shdr.chunk_hdr_sz;
    chunk_out.src_end = tgt_offset + data_size;

    return true;
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
 * \param[in] chdr Chunk header
 * \param[in] tgt_offset Offset of the output file
 * \param[out] chunk_out ChunkInfo to store chunk info
 *
 * \return Whether the chunk header is valid
 */
bool SparseFilePrivate::process_chunk(const ChunkHeader &chdr,
                                      uint64_t tgt_offset,
                                      ChunkInfo &chunk_out)
{
    MB_PUBLIC(SparseFile);

    if (chdr.total_sz < shdr.chunk_hdr_sz) {
        pub->set_error(make_error_code(FileError::BadFileFormat),
                       "Total chunk size (%" PRIu32 ") smaller than chunk "
                       "header size", chdr.total_sz);
        return false;
    }

    switch (chdr.chunk_type) {
    case CHUNK_TYPE_RAW:
        return process_raw_chunk(chdr, tgt_offset, chunk_out);
    case CHUNK_TYPE_FILL:
        return process_fill_chunk(chdr, tgt_offset, chunk_out);
    case CHUNK_TYPE_DONT_CARE:
        return process_skip_chunk(chdr, tgt_offset, chunk_out);
    case CHUNK_TYPE_CRC32:
        return process_crc32_chunk(chdr, tgt_offset, chunk_out);
    default:
        pub->set_error(make_error_code(FileError::BadFileFormat),
                       "Unknown chunk type: %u", chdr.chunk_type);
        return false;
    }
}

/*!
 * \brief Move to chunk that is responsible for the specified offset
 *
 * \note This function only returns a failure code if a file operation fails. To
 *       check if a matching chunk is found, use `chunk != chunks.end()`.
 *
 * \return True unless an error occurs
 */
bool SparseFilePrivate::move_to_chunk(uint64_t offset)
{
    MB_PUBLIC(SparseFile);

    // No action needed if the offset is in the current chunk
    if (chunk != chunks.end()
            && offset >= chunk->begin && offset < chunk->end) {
        return true;
    }

    // If the offset is in the current range of chunks, then do a binary search
    // to find the right chunk
    if (!chunks.empty() && offset < chunks.back().end) {
        chunk = binary_find(chunks.begin(), chunks.end(), offset, OffsetComp());
        if (chunk != chunks.end()) {
            return true;
        }
    }

    // We don't have the chunk, so read until we find it
    chunk = chunks.end();
    while (chunks.size() < shdr.total_chunks) {
        size_t chunk_num = chunks.size();

        DEBUG("Reading next chunk (#%" MB_PRIzu ")", chunk_num);

        // Get starting source and output offsets for chunk
        uint64_t src_begin;
        uint64_t tgt_begin;

        // First chunk starts after the sparse header. Remaining chunks are
        // contiguous.
        if (chunks.empty()) {
            src_begin = shdr.file_hdr_sz;
            tgt_begin = 0;
        } else {
            src_begin = chunks.back().src_end;
            tgt_begin = chunks.back().end;
        }

        // Skip to src_begin
        if (src_begin < cur_src_offset) {
            pub->set_error(make_error_code(FileError::BadFileFormat),
                           "Internal error: src_begin (%" PRIu64 ")"
                           " < cur_src_offset (%" PRIu64 ")",
                           src_begin, cur_src_offset);
            pub->set_fatal(true);
            return false;
        }

        if (!skip_bytes(src_begin - cur_src_offset)) {
            pub->set_error(pub->error(),
                           "Failed to skip to chunk #%" MB_PRIzu ": %s",
                           chunk_num, pub->error_string().c_str());
            pub->set_fatal(true);
            return false;
        }

        ChunkHeader chdr;

        if (!wread(&chdr, sizeof(chdr))) {
            pub->set_error(pub->error(),
                           "Failed to read chunk header for chunk %" MB_PRIzu
                           ": %s", chunk_num, pub->error_string().c_str());
            pub->set_fatal(true);
            return false;
        }

        fix_chunk_header_byte_order(chdr);

#if SPARSE_DEBUG
        dump_chunk_header(chdr);
#endif

        // Skip any extra bytes in the chunk header. process_sparse_header()
        // checks the size to make sure that the value won't underflow.
        if (!skip_bytes(shdr.chunk_hdr_sz - sizeof(chdr))) {
            pub->set_error(pub->error(),
                           "Failed to skip extra bytes in chunk #%" MB_PRIzu
                           "'s header: %s", chunk_num,
                           pub->error_string().c_str());
            pub->set_fatal(true);
            return false;
        }

        ChunkInfo chunk_info{};

        if (!process_chunk(chdr, tgt_begin, chunk_info)) {
            pub->set_fatal(true);
            return false;
        }

        OPER("Chunk #%" MB_PRIzu " covers source range (%" PRIu64 " - %" PRIu64 ")",
             chunk_num, chunk_info.src_begin, chunk_info.src_end);
        OPER("Chunk #%" MB_PRIzu " covers output range (%" PRIu64 " - %" PRIu64 ")",
             chunk_num, chunk_info.begin, chunk_info.end);

        // Make sure the chunk does not end after the header-specified file size
        if (chunk_info.end > file_size) {
            pub->set_error(make_error_code(FileError::BadFileFormat),
                           "Chunk #%" MB_PRIzu " ends (%" PRIu64 ") after the "
                           "file size specified in the sparse header (%"
                           PRIu64 ")", chunk_num, chunk_info.end, file_size);
            pub->set_fatal(true);
            return false;
        }

        chunks.push_back(std::move(chunk_info));

        // If we just read the last chunk, make sure it ends at the same
        // position as specified in the sparse header
        if (chunks.size() == shdr.total_chunks
                && chunks.back().end != file_size) {
            pub->set_error(make_error_code(FileError::BadFileFormat),
                           "Last chunk does not end (%" PRIu64 ") at position"
                           " specified by sparse header (%" PRIu64 ")",
                           chunks.back().end, file_size);
            pub->set_fatal(true);
            return false;
        }

        if (offset >= chunks.back().begin && offset < chunks.back().end) {
            chunk = chunks.end() - 1;
            break;
        }

        // Make sure iterator is not invalidated
        chunk = chunks.end();
    }

    return true;
}

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
    : SparseFile(new SparseFilePrivate(this))
{
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
    : SparseFile(new SparseFilePrivate(this), file)
{
}

/*! \cond INTERNAL */
SparseFile::SparseFile(SparseFilePrivate *priv)
    : _priv_ptr(priv)
{
}

SparseFile::SparseFile(SparseFilePrivate *priv, File *file)
    : _priv_ptr(priv)
{
    open(file);
}
/*! \endcond */

SparseFile::~SparseFile()
{
    close();
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
 * \return Whether the file is successfully opened
 */
bool SparseFile::open(File *file)
{
    MB_PRIVATE(SparseFile);
    if (priv) {
        priv->file = file;
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
    MB_PRIVATE(SparseFile);
    return priv->file_size;
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
 * file.seek(0, SEEK_CUR, nullptr) == true;
 * // Forward skipping unsupported if file.error() == FileError::UnsupportedSeek
 * \endcode
 *
 * The following test will be run (following the forward skipping test) to
 * determine if the file supports random seeking.
 *
 * \code{.cpp}
 * file.read(buf, 1, &n) == true;
 * file.seek(-1, SEEK_CUR, nullptr) == true;
 * // Random seeking unsupported if file.error() == FileError::UnsupportedSeek
 * \endcode
 *
 * \note This function will fail if the file handle is not open.
 *
 * \pre The caller should position the file at the beginning of the sparse file
 *      data. SparseFile will use that as the base offset and only perform
 *      relative seeks. This allows for opening sparse files where the data
 *      isn't at the beginning of the file.
 *
 * \return Whether the sparse file is successfully opened
 */
bool SparseFile::on_open()
{
    MB_PRIVATE(SparseFile);

    if (!priv->file->is_open()) {
        set_error(make_error_code(FileError::InvalidState),
                  "Underlying file is not open");
        return false;
    }

    unsigned char first_byte;
    size_t n;

    priv->seekability = Seekability::CAN_READ;

    if (priv->file->seek(0, SEEK_CUR, nullptr)) {
        DEBUG("File supports forward skipping");
        priv->seekability = Seekability::CAN_SKIP;
    } else if (priv->file->error() != FileErrorC::Unsupported) {
        set_error(priv->file->error(), "%s",
                  priv->file->error_string().c_str());
        return false;
    }

    if (!priv->file->read(&first_byte, 1, n)) {
        set_error(priv->file->error(), "%s",
                  priv->file->error_string().c_str());
        return false;
    } else if (n != 1) {
        set_error(make_error_code(FileError::BadFileFormat),
                  "Failed to read first byte of file");
        return false;
    }
    priv->cur_src_offset += n;

    if (priv->file->seek(-static_cast<int64_t>(n), SEEK_CUR, nullptr)) {
        DEBUG("File supports random seeking");
        priv->seekability = Seekability::CAN_SEEK;
        priv->cur_src_offset -= n;
        n = 0;
    } else if (priv->file->error() != FileErrorC::Unsupported) {
        set_error(priv->file->error(), "%s",
                  priv->file->error_string().c_str());
        return false;
    }

    if (!priv->process_sparse_header(&first_byte, n)) {
        return false;
    }

    return true;
}

/*!
 * \brief Close opened sparse file
 *
 * \note If the sparse file is open, then no matter what value is returned, the
 *       sparse file will be closed. The return value is the return value of the
 *       close callback function (if one was provided).
 *
 * \return
 *   * True if no error was encountered when closing the file.
 *   * False if an error occurs while closing the file
 */
bool SparseFile::on_close()
{
    MB_PRIVATE(SparseFile);

    // Reset to allow opening another file
    priv->clear();

    return true;
}

/*!
 * \brief Read sparse file
 *
 * This function will read the specified amount of bytes from the source file.
 * If this function returns true and \p bytes_read is less than \p size, then
 * the end of the sparse file (EOF) has been reached. If any error occurs, the
 * function will return false and any further attempts to read or seek the
 * sparse file results in undefined behavior as the internal state will be
 * invalid.
 *
 * Some examples of failures include:
 * * Source reaching EOF before the end of the sparse file is reached
 * * Ran out of sparse chunks despite the main sparse header promising more
 *   chunks
 * * Chunk header is invalid
 *
 * \param buf Buffer to read data into
 * \param size Number of bytes to read
 * \param bytes_read Number of bytes that were read
 *
 * \return Whether the specified number of bytes were successfully read
 */
bool SparseFile::on_read(void *buf, size_t size, size_t &bytes_read)
{
    MB_PRIVATE(SparseFile);

    OPER("read(buf, %" MB_PRIzu ", *bytesRead)", size);

    uint64_t total_read = 0;

    while (size > 0) {
        if (!priv->move_to_chunk(priv->cur_tgt_offset)) {
            return false;
        } else if (priv->chunk == priv->chunks.end()) {
            OPER("Reached EOF");
            break;
        }

        assert(priv->cur_tgt_offset >= priv->chunk->begin
                && priv->cur_tgt_offset < priv->chunk->end);

        // Read until the end of the current chunk
        uint64_t n_read = 0;
        uint64_t to_read = std::min<uint64_t>(
                size, priv->chunk->end - priv->cur_tgt_offset);

        OPER("Reading %" PRIu64 " bytes from chunk %" MB_PRIzu,
             to_read, priv->chunk - priv->chunks.begin());

        switch (priv->chunk->type) {
        case CHUNK_TYPE_RAW: {
            // Figure out how much to seek in the input data
            uint64_t diff = priv->cur_tgt_offset - priv->chunk->begin;
            OPER("Raw data is %" PRIu64 " bytes into the raw chunk", diff);

            uint64_t raw_src_offset = priv->chunk->raw_begin + diff;
            if (raw_src_offset != priv->cur_src_offset) {
                assert(priv->seekability == Seekability::CAN_SEEK);

                int64_t seek_offset;
                if (raw_src_offset < priv->cur_src_offset) {
                    seek_offset = -static_cast<int64_t>(
                            priv->cur_src_offset - raw_src_offset);
                } else {
                    seek_offset = static_cast<int64_t>(
                            raw_src_offset - priv->cur_src_offset);
                }

                if (!priv->wseek(seek_offset)) {
                    return false;
                }
            }

            if (!priv->wread(buf, static_cast<size_t>(to_read))) {
                return false;
            }

            n_read = to_read;
            break;
        }
        case CHUNK_TYPE_FILL: {
            static_assert(sizeof(priv->chunk->fill_val) == sizeof(uint32_t),
                          "Mismatched fill_val size");
            auto shift = (priv->cur_tgt_offset - priv->chunk->begin)
                    % sizeof(uint32_t);
            uint32_t fill_val = mb_htole32(priv->chunk->fill_val);
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
            memset(buf, 0, static_cast<size_t>(to_read));
            n_read = to_read;
            break;
        default:
            assert(false);
        }

        OPER("Read %" PRIu64 " bytes", n_read);
        total_read += n_read;
        priv->cur_tgt_offset += n_read;
        size -= static_cast<size_t>(n_read);
        buf = reinterpret_cast<unsigned char *>(buf) + n_read;
    }

    bytes_read = static_cast<size_t>(total_read);
    return true;
}

/*!
 * \brief Seek sparse file
 *
 * \p whence takes the same \a SEEK_SET, \a SEEK_CUR, and \a SEEK_END values as
 * \a lseek() in `\<stdio.h\>`.
 *
 * \note Seeking will only work if the underlying file handle supports seeking.
 *
 * \param[in] offset Offset to seek
 * \param[in] whence \a SEEK_SET, \a SEEK_CUR, or \a SEEK_END
 * \param[out] new_offset_out Pointer to store new offset of sparse file
 *
 * \return Whether the seeking was successful
 */
bool SparseFile::on_seek(int64_t offset, int whence, uint64_t &new_offset_out)
{
    MB_PRIVATE(SparseFile);

    OPER("seek(%" PRId64 ", %d)", offset, whence);

    if (priv->seekability != Seekability::CAN_SEEK) {
        set_error(make_error_code(FileError::UnsupportedSeek),
                  "Underlying file does not support seeking");
        return false;
    }

    uint64_t new_offset;
    switch (whence) {
    case SEEK_SET:
        if (offset < 0) {
            set_error(make_error_code(FileError::ArgumentOutOfRange),
                      "Cannot seek to negative offset");
            return false;
        }
        new_offset = static_cast<uint64_t>(offset);
        break;
    case SEEK_CUR:
        if ((offset < 0 && static_cast<uint64_t>(-offset) > priv->cur_tgt_offset)
                || (offset > 0 && priv->cur_tgt_offset
                        >= UINT64_MAX - static_cast<uint64_t>(offset))) {
            set_error(make_error_code(FileError::IntegerOverflow),
                      "Offset overflows uint64_t");
            return false;
        }
        new_offset = priv->cur_tgt_offset + static_cast<uint64_t>(offset);
        break;
    case SEEK_END:
        if ((offset < 0 && static_cast<uint64_t>(-offset) > priv->file_size)
                || (offset > 0 && priv->file_size
                        >= UINT64_MAX - static_cast<uint64_t>(offset))) {
            set_error(make_error_code(FileError::IntegerOverflow),
                      "Offset overflows uint64_t");
            return false;
        }
        new_offset = priv->file_size + static_cast<uint64_t>(offset);
        break;
    default:
        set_error(make_error_code(FileError::InvalidWhence),
                  "Invalid seek whence: %d", whence);
        return false;
    }

    if (!priv->move_to_chunk(new_offset)) {
        return false;
    }

    // May move past EOF, which is okay (mimics lseek behavior), but read()
    priv->cur_tgt_offset = new_offset;

    new_offset_out = new_offset;

    return true;
}

}
}
