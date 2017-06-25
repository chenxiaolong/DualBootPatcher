/*
 * Copyright (C) 2015-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#ifdef __ANDROID__
// Android does not support C++11 properly...
#define __STDC_LIMIT_MACROS
#endif

#include "mbsparse/sparse.h"

// For std::min()
#include <algorithm>

#include <vector>

#include <cassert>
#include <cinttypes>
#include <cstdint>
#include <cstring>

#include "mbcommon/string.h"
#include "mblog/logging.h"

// Enable debug logging of headers, offsets, etc.?
#define SPARSE_DEBUG 1
// Enable debug logging of operations (warning! very verbose!)
#define SPARSE_DEBUG_OPER 0
// Enable logging of errors
#define SPARSE_ERROR 1

#if SPARSE_DEBUG
#define DEBUG(...) LOGD(__VA_ARGS__)
#else
#define DEBUG(...)
#endif

#if SPARSE_DEBUG_OPER
#define OPER(...) LOGD(__VA_ARGS__)
#else
#define OPER(...)
#endif

#if SPARSE_ERROR
#define ERROR(...) LOGE(__VA_ARGS__)
#else
#define ERROR(...)
#endif

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
    uint64_t srcBegin;
    /*! \brief End of byte range for the entire chunk in the source file */
    uint64_t srcEnd;

    /*! \brief [CHUNK_TYPE_RAW only] Start of raw bytes in input file */
    uint64_t rawBegin;
    /*! \brief [CHUNK_TYPE_RAW only] End of raw bytes in input file */
    uint64_t rawEnd;

    /*! \brief [CHUNK_TYPE_FILL only] Filler value for the chunk */
    uint32_t fillVal;
};

struct SparseCtx
{
    // Callbacks
    SparseOpenCb cbOpen;
    SparseCloseCb cbClose;
    SparseReadCb cbRead;
    SparseSeekCb cbSeek;
    SparseSkipCb cbSkip;
    void *cbUserData;

    void setCallbacks(SparseOpenCb openCb, SparseCloseCb closeCb,
                      SparseReadCb readCb, SparseSeekCb seekCb,
                      SparseSkipCb skipCb, void *userData);
    void clearCallbacks();

    // Callback wrappers to avoid passing ctx->cbUserdata everywhere
    bool open();
    bool close();
    bool read(void *buf, uint64_t size, uint64_t *bytesRead);
    bool seek(int64_t offset, int whence);
    bool skip(uint64_t offset);

    bool skipBytes(uint64_t bytes);

    bool isOpen;

    uint32_t expectedCrc32 = 0;

    uint64_t srcOffset = 0;
    uint64_t outOffset = 0;
    uint64_t fileSize;

    SparseHeader shdr;

    std::vector<ChunkInfo> chunks;
    size_t chunk = 0;
};

void SparseCtx::setCallbacks(SparseOpenCb openCb, SparseCloseCb closeCb,
                             SparseReadCb readCb, SparseSeekCb seekCb,
                             SparseSkipCb skipCb, void *userData)
{
    cbOpen = openCb;
    cbClose = closeCb;
    cbRead = readCb;
    cbSeek = seekCb;
    cbSkip = skipCb;
    cbUserData = userData;
}

void SparseCtx::clearCallbacks()
{
    cbOpen = nullptr;
    cbClose = nullptr;
    cbRead = nullptr;
    cbSeek = nullptr;
    cbSkip = nullptr;
    cbUserData = nullptr;
}

bool SparseCtx::open()
{
    return cbOpen && cbOpen(cbUserData);
}

bool SparseCtx::close()
{
    return cbClose && cbClose(cbUserData);
}

bool SparseCtx::read(void *buf, uint64_t size, uint64_t *bytesRead)
{
    assert(cbRead != nullptr);
    if (cbRead && cbRead(buf, size, bytesRead, cbUserData)) {
        srcOffset += *bytesRead;
        return true;
    }
    return false;
}

bool SparseCtx::seek(int64_t offset, int whence)
{
    // We should never seek the source file from the end
    assert(whence != SEEK_END);
    if (cbSeek && whence != SEEK_END && cbSeek(offset, whence, cbUserData)) {
        if (whence == SEEK_SET) {
            srcOffset = offset;
        } else if (whence == SEEK_CUR) {
            srcOffset += offset;
        }
        return true;
    }
    return false;
}

bool SparseCtx::skip(uint64_t offset)
{
    return cbSkip && cbSkip(offset, cbUserData);
}

/*!
 * \brief Skip a certain amount of bytes
 *
 * If a seek callback is available, use it to seek the specified amount of
 * bytes. Otherwise, if a skip callback is available, use it to skip the
 * specified amount of bytes. If neither callback is available, read the
 * specified amount of bytes and discard the data.
 *
 * \param bytes Number of bytes to skip
 * \return Whether the specified amount of bytes were successfully skipped
 */
bool SparseCtx::skipBytes(uint64_t bytes)
{
    if (cbSeek) {
        // If we ran random-read, then simply seek
        return seek(bytes, SEEK_CUR);
    } else if (cbSkip) {
        // Otherwise, if there is a skip function, then use that
        return skip(bytes);
    } else {
        // If neither are available, read the specified number of bytes
        char dummy[10240];
        while (bytes > 0) {
            uint64_t toRead = std::min<uint64_t>(sizeof(dummy), bytes);
            uint64_t bytesRead;
            if (!read(dummy, toRead, &bytesRead) || bytesRead == 0) {
                DEBUG("Read failed or reached EOF when skipping bytes");
                return false;
            }
            bytes -= bytesRead;
        }
        return true;
    }
}

#if SPARSE_DEBUG
static void dumpSparseHeader(SparseHeader *header)
{
    DEBUG("Sparse header:");
    DEBUG("- magic:          0x%08" PRIx32, header->magic);
    DEBUG("- major_version:  0x%04" PRIx16, header->major_version);
    DEBUG("- minor_version:  0x%04" PRIx16, header->minor_version);
    DEBUG("- file_hdr_sz:    %" PRIu16 " (bytes)", header->file_hdr_sz);
    DEBUG("- chunk_hdr_sz:   %" PRIu16 " (bytes)", header->chunk_hdr_sz);
    DEBUG("- blk_sz:         %" PRIu32 " (bytes)", header->blk_sz);
    DEBUG("- total_blks:     %" PRIu32, header->total_blks);
    DEBUG("- total_chunks:   %" PRIu32, header->total_chunks);
    DEBUG("- image_checksum: 0x%08" PRIu32, header->image_checksum);
}

static const char * chunkTypeToString(uint16_t chunkType)
{
    switch (chunkType) {
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

static void dumpChunkHeader(ChunkHeader *header)
{
    DEBUG("Chunk header:");
    DEBUG("- chunk_type: 0x%04" PRIx16 " (%s)", header->chunk_type,
          chunkTypeToString(header->chunk_type));
    DEBUG("- reserved1:  0x%04" PRIx16, header->reserved1);
    DEBUG("- chunk_sz:   %" PRIu32 " (blocks)", header->chunk_sz);
    DEBUG("- total_sz:   %" PRIu32 " (bytes)", header->total_sz);
}
#endif

/*!
 * \brief Read the specified number of bytes completely (no partial reads)
 *
 * \param ctx Sparse context
 * \param buf Output buffer
 * \param size Bytes to read
 * \return Whether the specified amount of bytes (no more or less) were
 *         successfully read
 */
static bool readFully(SparseCtx *ctx, void *buf, std::size_t size)
{
    uint64_t bytesRead;
    if (!ctx->read(buf, size, &bytesRead)) {
        ERROR("Sparse read callback returned failure");
        return false;
    }
    if (bytesRead != size) {
        ERROR("Requested %" PRIu64 " bytes, but only read %" PRIu64 " bytes",
              (uint64_t) size, bytesRead);
        return false;
    }
    return true;
}

/*!
 * \brief Read and verify raw chunk header
 *
 * Thie function will check the following properties:
 * - (No additional checks)
 *
 * \param ctx Sparse context
 * \param chunkHeader Chunk header for the current chunk
 * \param outOffset Current offset of the output file
 * \return Whether the chunk header is valid
 */
static bool processRawChunk(SparseCtx *ctx, ChunkHeader *chunkHeader,
                            uint64_t outOffset)
{
    uint32_t dataSize = chunkHeader->total_sz - ctx->shdr.chunk_hdr_sz;
    uint64_t chunkSize = (uint64_t) chunkHeader->chunk_sz * ctx->shdr.blk_sz;

    if (dataSize != chunkSize) {
        ERROR("Number of data blocks (%" PRIu32 ") does not match"
              " number of expected blocks (%" PRIu32 ")",
              dataSize / ctx->shdr.blk_sz, chunkHeader->chunk_sz);
        return false;
    }

    ctx->chunks.emplace_back();
    ChunkInfo &chunk = ctx->chunks.back();
    chunk.type = chunkHeader->chunk_type;
    chunk.begin = outOffset;
    chunk.end = outOffset + dataSize;
    chunk.srcBegin = ctx->srcOffset - ctx->shdr.chunk_hdr_sz;
    chunk.srcEnd = ctx->srcOffset + dataSize;
    chunk.rawBegin = ctx->srcOffset;
    chunk.rawEnd = chunk.rawBegin + dataSize;

    return true;
}

/*!
 * \brief Read and verify fill chunk header
 *
 * Thie function will check the following properties:
 * - Chunk data size is 4 bytes (sizeof(uint32_t))
 *
 * \param ctx Sparse context
 * \param chunkHeader Chunk header for the current chunk
 * \param outOffset Current offset of the output file
 * \return Whether the chunk header is valid
 */
static bool processFillChunk(SparseCtx *ctx, ChunkHeader *chunkHeader,
                             uint64_t outOffset)
{
    uint32_t dataSize = chunkHeader->total_sz - ctx->shdr.chunk_hdr_sz;
    uint64_t chunkSize = (uint64_t) chunkHeader->chunk_sz * ctx->shdr.blk_sz;
    uint32_t fillVal;

    if (dataSize != sizeof(fillVal)) {
        ERROR("Data size (%" PRIu32 ") does not match size of 32-bit integer",
              dataSize);
        return false;
    }

    uint64_t srcBegin = ctx->srcOffset - ctx->shdr.chunk_hdr_sz;

    if (!readFully(ctx, &fillVal, sizeof(fillVal))) {
        return false;
    }

    uint64_t srcEnd = ctx->srcOffset;

    ctx->chunks.emplace_back();
    ChunkInfo &chunk = ctx->chunks.back();
    chunk.type = chunkHeader->chunk_type;
    chunk.begin = outOffset;
    chunk.end = outOffset + chunkSize;
    chunk.srcBegin = srcBegin;
    chunk.srcEnd = srcEnd;
    chunk.fillVal = fillVal;

    return true;
}

/*!
 * \brief Read and verify skip chunk header
 *
 * Thie function will check the following properties:
 * - Chunk data size is 0 bytes
 *
 * \param ctx Sparse context
 * \param chunkHeader Chunk header for the current chunk
 * \param outOffset Current offset of the output file
 * \return Whether the chunk header is valid
 */
static bool processSkipChunk(SparseCtx *ctx, ChunkHeader *chunkHeader,
                             uint64_t outOffset)
{
    uint32_t dataSize = chunkHeader->total_sz - ctx->shdr.chunk_hdr_sz;
    uint64_t chunkSize = (uint64_t) chunkHeader->chunk_sz * ctx->shdr.blk_sz;

    if (dataSize != 0) {
        ERROR("Data size (%" PRIu32 ") is not 0", dataSize);
        return false;
    }

    ctx->chunks.emplace_back();
    ChunkInfo &chunk = ctx->chunks.back();
    chunk.type = chunkHeader->chunk_type;
    chunk.begin = outOffset;
    chunk.end = outOffset + chunkSize;
    chunk.srcBegin = ctx->srcOffset - ctx->shdr.chunk_hdr_sz;
    chunk.srcEnd = ctx->srcOffset + dataSize;

    return true;
}

/*!
 * \brief Read and verify CRC32 chunk header
 *
 * Thie function will check the following properties:
 * - Chunk data size is 4 bytes (sizeof(uint32_t))
 *
 * \param ctx Sparse context
 * \param chunkHeader Chunk header for the current chunk
 * \param outOffset Current offset of the output file
 * \return Whether the chunk header is valid
 */
static bool processCrc32Chunk(SparseCtx *ctx, ChunkHeader *chunkHeader,
                              uint64_t outOffset)
{
    (void) outOffset;

    uint32_t dataSize = chunkHeader->total_sz - ctx->shdr.chunk_hdr_sz;
    uint64_t chunkSize = (uint64_t) chunkHeader->chunk_sz * ctx->shdr.blk_sz;
    uint32_t expectedCrc32;

    if (chunkSize != 0) {
        ERROR("Chunk data size (%" PRIu64 ") is not 0", chunkSize);
        return false;
    }

    if (dataSize != sizeof(expectedCrc32)) {
        ERROR("Data size (%" PRIu32 ") does not match size of 32-bit integer",
              dataSize);
        return false;
    }

    if (!readFully(ctx, &expectedCrc32, sizeof(expectedCrc32))) {
        return false;
    }

    ctx->expectedCrc32 = expectedCrc32;

    ctx->chunks.emplace_back();
    ChunkInfo &chunk = ctx->chunks.back();
    chunk.type = chunkHeader->chunk_type;
    chunk.begin = outOffset;
    chunk.end = outOffset;
    chunk.srcBegin = ctx->srcOffset - ctx->shdr.chunk_hdr_sz;
    chunk.srcEnd = ctx->srcOffset + dataSize;

    return true;
}

/*!
 * \brief Verify chunk header and add to chunk list
 *
 * Thie function will check the following properties:
 * - Number of chunk data blocks matches expected number of blocks
 *
 * \pre The file position should be at the byte immediately after the chunk
 *      header
 * \pre The value pointed by \a srcOffset should be set to the file position of
 *      the source file
 * \pre \a outOffset should be set to the beginning of the range in the output
 *      file that this chunk represents
 * \post If the chunk is a raw chunk, the source file position will be advanced
 *       to the first byte of the raw data. Otherwise, the source file position
 *       will advanced to the byte after the entire chunk.
 * \post The value pointed by \a srcOffset will be set to the new source file
 *       position
 *
 * \param ctx Sparse context
 * \param chunkHeader Chunk header for the current chunk
 * \param outOffset Current offset of the output file
 * \return Whether the chunk header is valid
 */
static bool processChunk(SparseCtx *ctx, ChunkHeader *chunkHeader,
                         uint64_t outOffset)
{
    bool ret;

    switch (chunkHeader->chunk_type) {
    case CHUNK_TYPE_RAW:
        ret = processRawChunk(ctx, chunkHeader, outOffset);
        break;
    case CHUNK_TYPE_FILL:
        ret = processFillChunk(ctx, chunkHeader, outOffset);
        break;
    case CHUNK_TYPE_DONT_CARE:
        ret = processSkipChunk(ctx, chunkHeader, outOffset);
        break;
    case CHUNK_TYPE_CRC32:
        ret = processCrc32Chunk(ctx, chunkHeader, outOffset);
        break;
    default:
        ERROR("Unknown chunk type: %u", chunkHeader->chunk_type);
        ret = false;
        break;
    }

    return ret;
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
 * \note The structure is read directly into memory. The host CPU must be
 *       little-endian and support having the structure layout in memory without
 *       padding.
 *
 * \note This does not process any of the chunk headers. The chunk headers are
 *       processed on-the-fly when reading the archive.
 *
 * \pre The file position should be at the beginning of the file (or at the
 *      beginning of the sparse header
 * \post The file position will be advanced to the byte after the sparse header.
 *       If the sparse header size, as specified by the \a file_hdr_sz field, is
 *       larger than the expected size, the extra bytes will be skipped as well.
 *
 * \param ctx Sparse context
 * \return Whether the sparse header is valid
 */
bool processSparseHeader(SparseCtx *ctx)
{
    // Read header
    if (!readFully(ctx, &ctx->shdr, sizeof(SparseHeader))) {
        return false;
    }

#if SPARSE_DEBUG
    dumpSparseHeader(&ctx->shdr);
#endif

    // Check magic bytes
    if (ctx->shdr.magic != SPARSE_HEADER_MAGIC) {
        ERROR("Expected magic to be %08x, but got %08x",
              SPARSE_HEADER_MAGIC, ctx->shdr.magic);
        return false;
    }

    // Check major version
    if (ctx->shdr.major_version != SPARSE_HEADER_MAJOR_VER) {
        ERROR("Expected major version to be %u, but got %u",
              SPARSE_HEADER_MAJOR_VER, ctx->shdr.major_version);
        return false;
    }

    // Check file header size field
    if (ctx->shdr.file_hdr_sz < sizeof(SparseHeader)) {
        ERROR("Expected file header size to be at least %" MB_PRIzu
              ", but have %" PRIu32,
              sizeof(ctx->shdr), ctx->shdr.file_hdr_sz);
        return false;
    }

    // Check chunk header size field
    if (ctx->shdr.chunk_hdr_sz < sizeof(ChunkHeader)) {
        ERROR("Expected chunk header size to be at least %" MB_PRIzu
              ", but have %" PRIu32,
              sizeof(SparseHeader), ctx->shdr.chunk_hdr_sz);
        return false;
    }

    // Skip any extra bytes in the file header
    std::size_t diff = ctx->shdr.file_hdr_sz - sizeof(SparseHeader);
    if (!ctx->skipBytes(diff)) {
        return false;
    }

    ctx->fileSize = (uint64_t) ctx->shdr.total_blks * ctx->shdr.blk_sz;

    return true;
}

/*!
 * \brief Find and move to chunk that is responsible for the specified offset
 *
 * \warning Always check if the offset exceeds the range of all chunks (EOF) by
 *          testing: "ctx->chunk == ctx->shdr.total_chunks"
 *
 * \return True unless an error occurs
 */
bool tryMoveToChunkForOffset(SparseCtx *ctx, uint64_t offset)
{
    // If were at EOF, move back one so we can search again
    if (ctx->shdr.total_chunks != 0 && ctx->chunk == ctx->shdr.total_chunks) {
        --ctx->chunk;
    }

    // Allow backwards seeking. If we read some chunks before, then search
    // backwards if offset is less than the beginning of the chunk.
    if (ctx->chunk < ctx->chunks.size()) {
        while (offset < ctx->chunks[ctx->chunk].begin) {
            if (ctx->chunk == 0) {
                break;
            }
            --ctx->chunk;
        }
    }

    for (; ctx->chunk < ctx->shdr.total_chunks; ++ctx->chunk) {
        // If we don't have the chunk yet, then read it
        if (ctx->chunk >= ctx->chunks.size()) {
            DEBUG("Reading next chunk (#%" MB_PRIzu ")", ctx->chunk);

            // Get starting offset for chunk in source file and starting
            // offset for data in the output file
            uint64_t srcBegin = ctx->shdr.file_hdr_sz;
            uint64_t outBegin = 0;
            if (ctx->chunk > 0) {
                srcBegin = ctx->chunks[ctx->chunk - 1].srcEnd;
                outBegin = ctx->chunks[ctx->chunk - 1].end;
            }

            // Skip to srcBegin
            if (srcBegin < ctx->srcOffset) {
                ERROR("- Internal error: srcBegin (%" PRIu64 ")"
                      " < srcOffset (%" PRIu64 ")", srcBegin, ctx->srcOffset);
                return false;
            }

            uint64_t diff = srcBegin - ctx->srcOffset;
            if (diff > 0 && !ctx->skipBytes(diff)) {
                ERROR("- Failed to skip to chunk #%" MB_PRIzu, ctx->chunk);
                return false;
            }

            ChunkHeader chunkHeader;

            if (!readFully(ctx, &chunkHeader, sizeof(ChunkHeader))) {
                ERROR("- Failed to read chunk header for chunk %" MB_PRIzu,
                      ctx->chunk);
                return false;
            }

#if SPARSE_DEBUG
            dumpChunkHeader(&chunkHeader);
#endif

            // Skip any extra bytes in the chunk header. processSparseHeader()
            // checks the size to make sure that the value won't underflow
            diff = ctx->shdr.chunk_hdr_sz - sizeof(ChunkHeader);
            if (!ctx->skipBytes(diff)) {
                ERROR("- Failed to skip extra bytes in chunk #%" MB_PRIzu "'s header",
                      ctx->chunk);
                return false;
            }

            if (!processChunk(ctx, &chunkHeader, outBegin)) {
                return false;
            }

            OPER("- Chunk #%" MB_PRIzu " covers source range (%" PRIu64 " - %" PRIu64 ")",
                 ctx->chunk, ctx->chunks[ctx->chunk].srcBegin,
                 ctx->chunks[ctx->chunk].srcEnd);
            OPER("- Chunk #%" MB_PRIzu " covers output range (%" PRIu64 " - %" PRIu64 ")",
                 ctx->chunk, ctx->chunks[ctx->chunk].begin,
                 ctx->chunks[ctx->chunk].end);

            // Make sure the chunk does not end after the header-specified file
            // size
            if (ctx->chunks[ctx->chunk].end > ctx->fileSize) {
                ERROR("Chunk #%" MB_PRIzu " ends (%" PRIu64 ") after the file size "
                      "specified in the sparse header (%" PRIu64 ")",
                      ctx->chunk, ctx->chunks[ctx->chunk].end, ctx->fileSize);
                return false;
            }

            // If we just read the last chunk, make sure it ends at the same
            // position as specified in the sparse header
            if (ctx->chunk == ctx->shdr.total_chunks - 1
                    && ctx->chunks[ctx->chunk].end != ctx->fileSize) {
                ERROR("Last chunk does not end (%" PRIu64 ")"
                      " at position specified by sparse header (%" PRIu64 ")",
                      ctx->chunks[ctx->chunk].end, ctx->fileSize);
                return false;
            }
        }

        if (offset >= ctx->chunks[ctx->chunk].begin
                && offset < ctx->chunks[ctx->chunk].end) {
            // Found matching chunk. Stop looking
            break;
        }
    }

    return true;
}

extern "C" {

SparseCtx * sparseCtxNew()
{
    SparseCtx *ctx = new(std::nothrow) SparseCtx();
    if (!ctx) {
        return nullptr;
    }
    ctx->isOpen = false;
    return ctx;
}

bool sparseCtxFree(SparseCtx *ctx)
{
    bool ret = true;
    if (ctx->isOpen) {
        ret = ctx->close();
    }
    delete ctx;
    return ret;
}

/*!
 * \brief Open sparse file for reading
 *
 * The source, which may not necessarily be a file, is read by calling functions
 * provided by the caller.
 *
 * All callbacks besides \a readCb are optional. The read callback should always
 * read the specified number of bytes before EOF is reached. The library will
 * treat a short read count as EOF.
 *
 * If a seek callback is provided, then the library will allow random reads to
 * the sparse file. Otherwise, the sparse file can only be read sequentially and
 * \a sparseRead() would not work. A skip callback can also be provided for
 * skipping bytes in a backwards-unseekable source. If both a seek callback and
 * a skip callback are provided, the seek callback will always be used. If
 * neither are provided, then the read callback will be repeatedly called and
 * the results discarded.
 *
 * The open and close callbacks are provided for convenience only. If an error
 * occurs between the time the open callback is called and this function
 * returns, then the close callback will be called. Thus, the underlying source
 * will always be "open" if this function returns true and "closed" if this
 * function returns false.
 *
 * \note If a seek callback is provided, the library to seek to the beginning of
 *       the source. Otherwise, it's up to the caller to ensure that the
 *       position of the source is at the beginning of the sparse file.
 *
 * \note After the sparse file is closed, another sparse file can be opened
 *       using the same \a ctx object.
 *
 * \param ctx Sparse context
 * \param openCb Open callback
 * \param closeCb Close callback
 * \param readCb Read callback
 * \param seekCb Seek callback
 * \param skipCb Skip callback
 * \param userData Caller-supplied pointer to pass to callback functions
 * \return Whether the sparse file is opened and the sparse header is determined
 *         to be valid
 */
bool sparseOpen(SparseCtx *ctx, SparseOpenCb openCb, SparseCloseCb closeCb,
                SparseReadCb readCb, SparseSeekCb seekCb, SparseSkipCb skipCb,
                void *userData)
{
    if (ctx->isOpen) {
        return false;
    }

    ctx->setCallbacks(openCb, closeCb, readCb, seekCb, skipCb, userData);

    if (ctx->cbOpen && !ctx->open()) {
        ctx->clearCallbacks();
        return false;
    }

    // Goto beginning of file if we can. Otherwise, just assume the file is at
    // the beginning
    if (ctx->cbSeek && !ctx->seek(0, SEEK_SET)) {
        return false;
    }

    // Process main sparse header
    if (!processSparseHeader(ctx)) {
        ctx->close();
        ctx->clearCallbacks();
        return false;
    }

    // The chunk headers are processed on demand

    ctx->isOpen = true;

    return true;
}

/*!
 * \brief Close opened sparse file
 *
 * \note If the sparse file is open, then no matter what value is returned, the
 *       sparse file will be closed. The return value is the return value of the
 *       close callback function (if one was provided).
 *
 * \return Whether the sparse file was closed
 */
bool sparseClose(SparseCtx *ctx)
{
    if (!ctx->isOpen) {
        return false;
    }

    ctx->isOpen = false;
    ctx->srcOffset = 0;
    ctx->outOffset = 0;
    ctx->expectedCrc32 = 0;
    ctx->chunks.clear();
    ctx->chunk = 0;

    bool ret = true;
    if (ctx->cbClose) {
        ret = ctx->close();
    }

    ctx->clearCallbacks();
    return ret;
}

/*!
 * \brief Read sparse file
 *
 * This function will read the specified amount of bytes from the source file.
 * If this function returns true and \a bytesRead is less than \a size, then
 * the end of the sparse file (EOF) has been reached. If any error occurs, the
 * function will return false and any further attempts to read or seek the
 * sparse file results in undefined behavior as the internal state will be
 * invalid.
 *
 * Some examples of failures include:
 * - Source reaching EOF before the end of the sparse file is reached
 * - Ran out of sparse chunks despite the main sparse header promising more
 *   chunks
 * - Chunk header is invalid
 *
 * \param ctx Sparse context
 * \param buf Buffer to read data into
 * \param size Number of bytes to read
 * \param bytesRead Number of bytes that were read
 * \return Whether the specified number of bytes were successfully read
 */
bool sparseRead(SparseCtx *ctx, void *buf, uint64_t size, uint64_t *bytesRead)
{
    if (!ctx->isOpen) {
        return false;
    }

    OPER("read(buf, %" PRId64 ", *bytesRead)", size);

    uint64_t totalRead = 0;

    while (size > 0) {
        // If no chunks have been read yet or the current offset exceeds the
        // range of the current chunk, then look for the next chunk.

        if (ctx->chunks.empty()
                || ctx->chunk == ctx->shdr.total_chunks
                || ctx->outOffset >= ctx->chunks[ctx->chunk].end) {
            if (!tryMoveToChunkForOffset(ctx, ctx->outOffset)) {
                return false;
            }

            if (ctx->chunk == ctx->shdr.total_chunks) {
                OPER("- Found EOF");
                break;
            }
        }

        assert(ctx->outOffset >= ctx->chunks[ctx->chunk].begin
                && ctx->outOffset < ctx->chunks[ctx->chunk].end);

        // Read until the end of the current chunk
        uint64_t nRead = 0;
        uint64_t toRead = std::min(size,
                ctx->chunks[ctx->chunk].end - ctx->outOffset);

        OPER("- Reading %" PRIu64 " bytes from chunk %" MB_PRIzu,
             toRead, ctx->chunk);

        switch (ctx->chunks[ctx->chunk].type) {
        case CHUNK_TYPE_RAW: {
            // Figure out how much to seek in the input data
            uint64_t diff = ctx->outOffset - ctx->chunks[ctx->chunk].begin;
            OPER("- Raw data is %" PRIu64 " bytes into the raw chunk", diff);
            if (ctx->cbSeek && !ctx->seek(
                    ctx->chunks[ctx->chunk].rawBegin + diff, SEEK_SET)) {
                return false;
            }
            if (!ctx->read(buf, toRead, &nRead)) {
                return false;
            }
            if (nRead == 0) {
                // EOF
                goto done;
            }
            break;
        }
        case CHUNK_TYPE_FILL: {
            assert(sizeof(ctx->chunks[ctx->chunk].fillVal) == sizeof(uint32_t));
            auto shift = (ctx->outOffset - ctx->chunks[ctx->chunk].begin)
                    % sizeof(uint32_t);
            uint32_t fillVal = ctx->chunks[ctx->chunk].fillVal;
            uint32_t shifted = 0;
            for (size_t i = 0; i < sizeof(uint32_t); ++i) {
                ((char *) &shifted)[i] =
                        ((char *) &fillVal)[(i + shift) % sizeof(uint32_t)];
                //uint32_t amount = 8 * ((i + shift) % sizeof(uint32_t));
                //shifted |= (fillVal & (0xff << amount)) >> amount << i * 8;
            }
            char *tempBuf = (char *) buf;
            while (toRead > 0) {
                size_t toWrite = std::min<size_t>(sizeof(shifted), toRead);
                memcpy(tempBuf, &shifted, toWrite);
                nRead += toWrite;
                toRead -= toWrite;
                tempBuf += toWrite;
            }
            break;
        }
        case CHUNK_TYPE_DONT_CARE:
            memset(buf, 0, toRead);
            nRead = toRead;
            break;
        case CHUNK_TYPE_CRC32:
            assert(false);
        default:
            return false;
        }

        OPER("- Read %" PRIu64 " bytes", nRead);
        totalRead += nRead;
        ctx->outOffset += nRead;
        size -= nRead;
        buf = (char *) buf + nRead;
    }

done:
    *bytesRead = totalRead;
    return true;
}

/*!
 * \brief Seek sparse file
 *
 * \a whence takes the same \a SEEK_SET, \a SEEK_CUR, and \a SEEK_END values as
 * \a lseek() in <stdio.h>.
 *
 * \note If a seek callback was not provided, then this function will always
 *       return false;
 *
 * \param ctx Sparse context
 * \param offset Offset to seek
 * \param whence \a SEEK_SET, \a SEEK_CUR, or \a SEEK_END
 * \return Whether the seeking was successful
 */
bool sparseSeek(SparseCtx *ctx, int64_t offset, int whence)
{
    if (!ctx->isOpen) {
        return false;
    }

    OPER("seek(%" PRId64 ", %d)", offset, whence);

    if (!ctx->cbSeek) {
        OPER("- Cannot seek because no seek callback is registered");
        return false;
    }

    uint64_t newOffset;
    switch (whence) {
    case SEEK_SET:
        if (offset < 0) {
            OPER("- Tried to seek to negative offset");
            return false;
        }
        newOffset = offset;
        break;
    case SEEK_CUR:
        if ((offset < 0 && (uint64_t) -offset > ctx->outOffset)
                || (offset > 0 && ctx->outOffset >= UINT64_MAX - offset)) {
            OPER("- Offset overflows uint64_t");
            return false;
        }
        newOffset = ctx->outOffset + offset;
        break;
    case SEEK_END:
        if ((offset < 0 && (uint64_t) -offset > ctx->fileSize)
                || (offset > 0 && ctx->fileSize >= UINT64_MAX - offset)) {
            OPER("- Offset overflows uint64_t");
            return false;
        }
        newOffset = ctx->fileSize + offset;
        break;
    default:
        OPER("- Invalid seek whence: %d", whence);
        return false;
    }

    if (!tryMoveToChunkForOffset(ctx, newOffset)) {
        return false;
    }

    // May move past EOF, which is okay (mimics lseek behavior), but
    // sparseRead() will know to read nothing in that case
    ctx->outOffset = newOffset;
    return true;
}

/*!
 * \brief Get file pointer position in sparse file
 *
 * \param ctx Sparse context
 * \param offset Output pointer for current position in sparse file
 * \return True, unless the file is not open
 */
bool sparseTell(SparseCtx *ctx, uint64_t *offset)
{
    if (!ctx->isOpen) {
        return false;
    }

    *offset = ctx->outOffset;
    return true;
}

/*!
 * \brief Get the expected size of the sparse file
 *
 * \param ctx Sparse context
 * \param size Output pointer for the expected file size
 * \return True, unless the file is not open
 */
bool sparseSize(SparseCtx *ctx, uint64_t *size)
{
    if (!ctx->isOpen) {
        return false;
    }

    *size = ctx->fileSize;
    return true;
}

}
