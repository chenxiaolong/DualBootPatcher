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

#include "mbcommon/file/standard.h"
#include "mbsparse/sparse.h"

typedef std::unique_ptr<SparseCtx, bool (*)(SparseCtx *)> ScopedSparseCtx;

struct Context
{
    std::string path;
    mb::StandardFile file;
};

bool cbOpen(void *userData)
{
    Context *ctx = static_cast<Context *>(userData);
    if (ctx->file.open(ctx->path, mb::FileOpenMode::READ_ONLY)
            != mb::FileStatus::OK) {
        fprintf(stderr, "%s: Failed to open: %s\n",
                ctx->path.c_str(), ctx->file.error_string().c_str());
        return false;
    }
    return true;
}

bool cbClose(void *userData)
{
    Context *ctx = static_cast<Context *>(userData);
    if (ctx->file.close() != mb::FileStatus::OK) {
        fprintf(stderr, "%s: Failed to close: %s\n",
                ctx->path.c_str(), ctx->file.error_string().c_str());
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
        if (ctx->file.read(buf, size, &partial) != mb::FileStatus::OK) {
            fprintf(stderr, "%s: Failed to read: %s\n",
                    ctx->path.c_str(), ctx->file.error_string().c_str());
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
    if (ctx->file.seek(offset, whence, nullptr) != mb::FileStatus::OK) {
        fprintf(stderr, "%s: Failed to seek: %s\n",
                ctx->path.c_str(), ctx->file.error_string().c_str());
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

    ScopedSparseCtx sparseCtx(sparseCtxNew(), &sparseCtxFree);
    if (!sparseCtx) {
        fprintf(stderr, "Out of memory\n");
        return EXIT_FAILURE;
    }

    if (!sparseOpen(sparseCtx.get(), &cbOpen, &cbClose, &cbRead, &cbSeek,
                    nullptr, &ctx)) {
        return EXIT_FAILURE;
    }

    mb::StandardFile file;

    if (file.open(outputFile, mb::FileOpenMode::WRITE_ONLY)
            != mb::FileStatus::OK) {
        fprintf(stderr, "%s: Failed to open for writing: %s\n",
                outputFile, file.error_string().c_str());
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
            if (file.write(buf, bytesRead, &bytesWritten)
                    != mb::FileStatus::OK) {
                fprintf(stderr, "%s: Failed to write: %s\n",
                        outputFile, file.error_string().c_str());
                return EXIT_FAILURE;
            }
            bytesRead -= bytesWritten;
            ptr += bytesWritten;
        }
    }
    if (!ret) {
        return EXIT_FAILURE;
    }

    return file.close() == mb::FileStatus::OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
