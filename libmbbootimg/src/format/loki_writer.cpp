/*
 * Copyright (C) 2015-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbbootimg/format/loki_writer_p.h"

#include <algorithm>

#include <cerrno>
#include <cinttypes>
#include <cstdio>
#include <cstring>

#include <openssl/sha.h>

#include "mbcommon/endian.h"
#include "mbcommon/file.h"
#include "mbcommon/file_util.h"
#include "mbcommon/string.h"

#include "mbbootimg/entry.h"
#include "mbbootimg/format/align_p.h"
#include "mbbootimg/format/loki_defs.h"
#include "mbbootimg/format/loki_p.h"
#include "mbbootimg/header.h"
#include "mbbootimg/writer.h"
#include "mbbootimg/writer_p.h"

namespace mb
{
namespace bootimg
{
namespace loki
{

constexpr size_t MAX_ABOOT_SIZE = 2 * 1024 * 1024;

int loki_writer_get_header(MbBiWriter *biw, void *userdata, Header &header)
{
    (void) biw;
    (void) userdata;

    header.set_supported_fields(NEW_SUPPORTED_FIELDS);

    return RET_OK;
}

int loki_writer_write_header(MbBiWriter *biw, void *userdata,
                             const Header &header)
{
    LokiWriterCtx *const ctx = static_cast<LokiWriterCtx *>(userdata);
    int ret;

    // Construct header
    memset(&ctx->hdr, 0, sizeof(ctx->hdr));
    memcpy(ctx->hdr.magic, android::BOOT_MAGIC, android::BOOT_MAGIC_SIZE);

    if (auto address = header.kernel_address()) {
        ctx->hdr.kernel_addr = *address;
    }
    if (auto address = header.ramdisk_address()) {
        ctx->hdr.ramdisk_addr = *address;
    }
    if (auto address = header.secondboot_address()) {
        ctx->hdr.second_addr = *address;
    }
    if (auto address = header.kernel_tags_address()) {
        ctx->hdr.tags_addr = *address;
    }
    if (auto page_size = header.page_size()) {
        switch (*page_size) {
        case 2048:
        case 4096:
        case 8192:
        case 16384:
        case 32768:
        case 65536:
        case 131072:
            ctx->hdr.page_size = *page_size;
            break;
        default:
            writer_set_error(biw, ERROR_FILE_FORMAT,
                             "Invalid page size: %" PRIu32, *page_size);
            return RET_FAILED;
        }
    } else {
        writer_set_error(biw, ERROR_FILE_FORMAT,
                         "Page size field is required");
        return RET_FAILED;
    }

    if (auto board_name = header.board_name()) {
        if (board_name->size() >= sizeof(ctx->hdr.name)) {
            writer_set_error(biw, ERROR_FILE_FORMAT,
                             "Board name too long");
            return RET_FAILED;
        }

        strncpy(reinterpret_cast<char *>(ctx->hdr.name), board_name->c_str(),
                sizeof(ctx->hdr.name) - 1);
        ctx->hdr.name[sizeof(ctx->hdr.name) - 1] = '\0';
    }
    if (auto cmdline = header.kernel_cmdline()) {
        if (cmdline->size() >= sizeof(ctx->hdr.cmdline)) {
            writer_set_error(biw, ERROR_FILE_FORMAT,
                             "Kernel cmdline too long");
            return RET_FAILED;
        }

        strncpy(reinterpret_cast<char *>(ctx->hdr.cmdline), cmdline->c_str(),
                sizeof(ctx->hdr.cmdline) - 1);
        ctx->hdr.cmdline[sizeof(ctx->hdr.cmdline) - 1] = '\0';
    }

    // TODO: UNUSED
    // TODO: ID

    // Clear existing entries (none should exist unless this function fails and
    // the user reattempts to call it)
    ctx->seg.entries_clear();

    ret = ctx->seg.entries_add(ENTRY_TYPE_KERNEL,
                               0, false, ctx->hdr.page_size, biw);
    if (ret != RET_OK) return ret;

    ret = ctx->seg.entries_add(ENTRY_TYPE_RAMDISK,
                               0, false, ctx->hdr.page_size, biw);
    if (ret != RET_OK) return ret;

    ret = ctx->seg.entries_add(ENTRY_TYPE_DEVICE_TREE,
                               0, false, ctx->hdr.page_size, biw);
    if (ret != RET_OK) return ret;

    ret = ctx->seg.entries_add(ENTRY_TYPE_ABOOT,
                               0, true, 0, biw);
    if (ret != RET_OK) return ret;

    // Start writing after first page
    if (!biw->file->seek(ctx->hdr.page_size, SEEK_SET, nullptr)) {
        writer_set_error(biw, biw->file->error().value() /* TODO */,
                         "Failed to seek to first page: %s",
                         biw->file->error_string().c_str());
        return biw->file->is_fatal() ? RET_FATAL : RET_FAILED;
    }

    return RET_OK;
}

int loki_writer_get_entry(MbBiWriter *biw, void *userdata,
                          Entry &entry)
{
    LokiWriterCtx *const ctx = static_cast<LokiWriterCtx *>(userdata);

    return ctx->seg.get_entry(*biw->file, entry, biw);
}

int loki_writer_write_entry(MbBiWriter *biw, void *userdata,
                            const Entry &entry)
{
    LokiWriterCtx *const ctx = static_cast<LokiWriterCtx *>(userdata);

    return ctx->seg.write_entry(*biw->file, entry, biw);
}

int loki_writer_write_data(MbBiWriter *biw, void *userdata,
                           const void *buf, size_t buf_size,
                           size_t &bytes_written)
{
    LokiWriterCtx *const ctx = static_cast<LokiWriterCtx *>(userdata);
    int ret;

    auto const *swentry = ctx->seg.entry();

    if (swentry->type == ENTRY_TYPE_ABOOT) {
        if (buf_size > MAX_ABOOT_SIZE - ctx->aboot.size()) {
            writer_set_error(biw, ERROR_INTERNAL_ERROR,
                             "aboot image too large");
            return RET_FATAL;
        }

        size_t old_aboot_size = ctx->aboot.size();
        ctx->aboot.resize(old_aboot_size + buf_size);

        memcpy(ctx->aboot.data() + old_aboot_size, buf, buf_size);

        bytes_written = buf_size;
    } else {
        ret = ctx->seg.write_data(*biw->file, buf, buf_size, bytes_written,
                                  biw);
        if (ret != RET_OK) {
            return ret;
        }

        // We always include the image in the hash. The size is sometimes
        // included and is handled in loki_writer_finish_entry().
        if (!SHA1_Update(&ctx->sha_ctx, buf, buf_size)) {
            writer_set_error(biw, ERROR_INTERNAL_ERROR,
                             "Failed to update SHA1 hash");
            // This must be fatal as the write already happened and cannot be
            // reattempted
            return RET_FATAL;
        }
    }

    return RET_OK;
}

int loki_writer_finish_entry(MbBiWriter *biw, void *userdata)
{
    LokiWriterCtx *const ctx = static_cast<LokiWriterCtx *>(userdata);
    int ret;

    ret = ctx->seg.finish_entry(*biw->file, biw);
    if (ret != RET_OK) {
        return ret;
    }

    auto const *swentry = ctx->seg.entry();

    // Update SHA1 hash
    uint32_t le32_size = mb_htole32(swentry->size);

    // Include fake 0 size for unsupported secondboot image
    if (swentry->type == ENTRY_TYPE_DEVICE_TREE
            && !SHA1_Update(&ctx->sha_ctx, "\x00\x00\x00\x00", 4)) {
        writer_set_error(biw, ERROR_INTERNAL_ERROR,
                         "Failed to update SHA1 hash");
        return RET_FATAL;
    }

    // Include size for everything except empty DT images
    if (swentry->type != ENTRY_TYPE_ABOOT
            && (swentry->type != ENTRY_TYPE_DEVICE_TREE || swentry->size > 0)
            && !SHA1_Update(&ctx->sha_ctx, &le32_size, sizeof(le32_size))) {
        writer_set_error(biw, ERROR_INTERNAL_ERROR,
                         "Failed to update SHA1 hash");
        return RET_FATAL;
    }

    switch (swentry->type) {
    case ENTRY_TYPE_KERNEL:
        ctx->hdr.kernel_size = swentry->size;
        break;
    case ENTRY_TYPE_RAMDISK:
        ctx->hdr.ramdisk_size = swentry->size;
        break;
    case ENTRY_TYPE_DEVICE_TREE:
        ctx->hdr.dt_size = swentry->size;
        break;
    }

    return RET_OK;
}

int loki_writer_close(MbBiWriter *biw, void *userdata)
{
    LokiWriterCtx *const ctx = static_cast<LokiWriterCtx *>(userdata);
    int ret;
    size_t n;

    if (ctx->have_file_size) {
        if (!biw->file->seek(ctx->file_size, SEEK_SET, nullptr)) {
            writer_set_error(biw, biw->file->error().value() /* TODO */,
                             "Failed to seek to end of file: %s",
                             biw->file->error_string().c_str());
            return biw->file->is_fatal() ? RET_FATAL : RET_FAILED;
        }
    } else {
        if (!biw->file->seek(0, SEEK_CUR, &ctx->file_size)) {
            writer_set_error(biw, biw->file->error().value() /* TODO */,
                             "Failed to get file offset: %s",
                             biw->file->error_string().c_str());
            return biw->file->is_fatal() ? RET_FATAL : RET_FAILED;
        }

        ctx->have_file_size = true;
    }

    auto const *swentry = ctx->seg.entry();

    // If successful, finish up the boot image
    if (!swentry) {
        // Truncate to set size
        if (!biw->file->truncate(ctx->file_size)) {
            writer_set_error(biw, biw->file->error().value() /* TODO */,
                             "Failed to truncate file: %s",
                             biw->file->error_string().c_str());
            return biw->file->is_fatal() ? RET_FATAL : RET_FAILED;
        }

        // Set ID
        unsigned char digest[SHA_DIGEST_LENGTH];
        if (!SHA1_Final(digest, &ctx->sha_ctx)) {
            writer_set_error(biw, ERROR_INTERNAL_ERROR,
                             "Failed to update SHA1 hash");
            return RET_FATAL;
        }
        memcpy(ctx->hdr.id, digest, SHA_DIGEST_LENGTH);

        // Convert fields back to little-endian
        android::AndroidHeader hdr = ctx->hdr;
        android_fix_header_byte_order(hdr);

        // Seek back to beginning to write header
        if (!biw->file->seek(0, SEEK_SET, nullptr)) {
            writer_set_error(biw, biw->file->error().value() /* TODO */,
                             "Failed to seek to beginning: %s",
                             biw->file->error_string().c_str());
            return biw->file->is_fatal() ? RET_FATAL : RET_FAILED;
        }

        // Write header
        if (!file_write_fully(*biw->file, &hdr, sizeof(hdr), n)
                || n != sizeof(hdr)) {
            writer_set_error(biw, biw->file->error().value() /* TODO */,
                             "Failed to write header: %s",
                             biw->file->error_string().c_str());
            return biw->file->is_fatal() ? RET_FATAL : RET_FAILED;
        }

        // Patch with Loki
        ret = _loki_patch_file(biw, *biw->file, ctx->aboot.data(),
                               ctx->aboot.size());
        if (ret != RET_OK) {
            return ret;
        }
    }

    return RET_OK;
}

int loki_writer_free(MbBiWriter *bir, void *userdata)
{
    (void) bir;
    delete static_cast<LokiWriterCtx *>(userdata);
    return RET_OK;
}

}

/*!
 * \brief Set Loki boot image output format
 *
 * \param biw MbBiWriter
 *
 * \return
 *   * #RET_OK if the format is successfully enabled
 *   * #RET_WARN if the format is already enabled
 *   * \<= #RET_FAILED if an error occurs
 */
int writer_set_format_loki(MbBiWriter *biw)
{
    using namespace loki;

    LokiWriterCtx *const ctx = new LokiWriterCtx();

    if (!SHA1_Init(&ctx->sha_ctx)) {
        writer_set_error(biw, ERROR_INTERNAL_ERROR,
                         "Failed to initialize SHA_CTX");
        delete ctx;
        return false;
    }

    return _writer_register_format(biw,
                                   ctx,
                                   FORMAT_LOKI,
                                   FORMAT_NAME_LOKI,
                                   nullptr,
                                   &loki_writer_get_header,
                                   &loki_writer_write_header,
                                   &loki_writer_get_entry,
                                   &loki_writer_write_entry,
                                   &loki_writer_write_data,
                                   &loki_writer_finish_entry,
                                   &loki_writer_close,
                                   &loki_writer_free);
}

}
}
