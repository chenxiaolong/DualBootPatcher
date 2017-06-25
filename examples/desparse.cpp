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

#include <memory>
#include <string>

#include <cstdlib>
#include <cstdio>

#include "mbcommon/file/filename.h"
#include "mbsparse/sparse.h"

typedef std::unique_ptr<MbFile, int (*)(MbFile *)> ScopedMbFile;
typedef std::unique_ptr<SparseCtx, bool (*)(SparseCtx *)> ScopedSparseCtx;

struct Context
{
    std::string path;
    ScopedMbFile file{nullptr, &mb_file_free};
};

bool cbOpen(void *userData)
{
    Context *ctx = static_cast<Context *>(userData);
    if (mb_file_open_filename(ctx->file.get(), ctx->path.c_str(),
            MB_FILE_OPEN_READ_ONLY) != MB_FILE_OK) {
        fprintf(stderr, "%s: Failed to open: %s\n",
                ctx->path.c_str(), mb_file_error_string(ctx->file.get()));
        return false;
    }
    return true;
}

bool cbClose(void *userData)
{
    Context *ctx = static_cast<Context *>(userData);
    if (mb_file_close(ctx->file.get()) != MB_FILE_OK) {
        fprintf(stderr, "%s: Failed to close: %s\n",
                ctx->path.c_str(), mb_file_error_string(ctx->file.get()));
        return false;
    }
    return true;
}

bool cbRead(void *buf, uint64_t size, uint64_t *bytesRead, void *userData)
{
    Context *ctx = static_cast<Context *>(userData);
    size_t total = 0;
    while (size > 0) {
        size_t partial;
        if (mb_file_read(ctx->file.get(), buf, size, &partial) != MB_FILE_OK) {
            fprintf(stderr, "%s: Failed to read: %s\n",
                    ctx->path.c_str(), mb_file_error_string(ctx->file.get()));
            return false;
        }
        size -= partial;
        total += partial;
        buf = static_cast<char *>(buf) + partial;
    }
    *bytesRead = total;
    return true;
}

bool cbSeek(int64_t offset, int whence, void *userData)
{
    Context *ctx = static_cast<Context *>(userData);
    if (mb_file_seek(ctx->file.get(), offset, whence, nullptr) != MB_FILE_OK) {
        fprintf(stderr, "%s: Failed to seek: %s\n",
                ctx->path.c_str(), mb_file_error_string(ctx->file.get()));
        return false;
    }
    return true;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        std::fprintf(stderr, "Usage: %s <input file> <output file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *inputFile = argv[1];
    const char *outputFile = argv[2];

    Context ctx;
    ctx.path = inputFile;
    ctx.file.reset(mb_file_new());

    if (!ctx.file) {
        fprintf(stderr, "Out of memory\n");
        return EXIT_FAILURE;
    }

    ScopedSparseCtx sparseCtx(sparseCtxNew(), &sparseCtxFree);
    if (!sparseCtx) {
        fprintf(stderr, "Out of memory\n");
        return EXIT_FAILURE;
    }

    if (!sparseOpen(sparseCtx.get(), &cbOpen, &cbClose, &cbRead, &cbSeek,
                    nullptr, &ctx)) {
        return EXIT_FAILURE;
    }

    ScopedMbFile file(mb_file_new(), &mb_file_free);
    if (!ctx.file) {
        fprintf(stderr, "Out of memory\n");
        return EXIT_FAILURE;
    }

    if (mb_file_open_filename(file.get(), outputFile, MB_FILE_OPEN_WRITE_ONLY)
            != MB_FILE_OK) {
        fprintf(stderr, "%s: Failed to open for writing: %s\n",
                outputFile, mb_file_error_string(file.get()));
        return EXIT_FAILURE;
    }

    uint64_t bytesRead;
    char buf[10240];
    bool ret;
    while ((ret = sparseRead(sparseCtx.get(), buf, sizeof(buf), &bytesRead))
            && bytesRead > 0) {
        char *ptr = buf;
        size_t bytesWritten;
        while (bytesRead > 0) {
            if (mb_file_write(file.get(), buf, bytesRead, &bytesWritten)
                    != MB_FILE_OK) {
                fprintf(stderr, "%s: Failed to write: %s\n",
                        outputFile, mb_file_error_string(file.get()));
                return EXIT_FAILURE;
            }
            bytesRead -= bytesWritten;
            ptr += bytesWritten;
        }
    }
    if (!ret) {
        return EXIT_FAILURE;
    }

    return mb_file_close(file.get()) == MB_FILE_OK
            ? EXIT_SUCCESS : EXIT_FAILURE;
}
