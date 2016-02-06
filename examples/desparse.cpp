/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cstdio>

#include "libmbp/sparse/sparse.h"
#include "libmbpio/file.h"

struct Context
{
    std::string path;
    io::File file;
};

bool cbOpen(void *userData)
{
    Context *ctx = static_cast<Context *>(userData);
    if (!ctx->file.open(ctx->path, io::File::OpenRead)) {
        fprintf(stderr, "%s: Failed to open: %s\n",
                ctx->path.c_str(), ctx->file.errorString().c_str());
        return false;
    }
    return true;
}

bool cbClose(void *userData)
{
    Context *ctx = static_cast<Context *>(userData);
    if (!ctx->file.close()) {
        fprintf(stderr, "%s: Failed to close: %s\n",
                ctx->path.c_str(), ctx->file.errorString().c_str());
        return false;
    }
    return true;
}

bool cbRead(void *buf, uint64_t size, uint64_t *bytesRead, void *userData)
{
    Context *ctx = static_cast<Context *>(userData);
    uint64_t total = 0;
    while (size > 0) {
        uint64_t partial;
        if (!ctx->file.read(buf, size, &partial)) {
            fprintf(stderr, "%s: Failed to read: %s\n",
                    ctx->path.c_str(), ctx->file.errorString().c_str());
            return false;
        }
        size -= partial;
        total += partial;
        buf = (char *) buf + partial;
    }
    *bytesRead = total;
    return true;
}

bool cbSeek(int64_t offset, int whence, void *userData)
{
    Context *ctx = static_cast<Context *>(userData);
    int ioWhence;
    switch (whence) {
    case SEEK_SET:
        ioWhence = io::File::SeekBegin;
        break;
    case SEEK_CUR:
        ioWhence = io::File::SeekCurrent;
        break;
    case SEEK_END:
        ioWhence = io::File::SeekEnd;
        break;
    default:
        return false;
    }
    if (!ctx->file.seek(offset, ioWhence)) {
        fprintf(stderr, "%s: Failed to seek: %s\n",
                ctx->path.c_str(), ctx->file.errorString().c_str());
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

    SparseCtx *sparseCtx = sparseCtxNew();
    if (!sparseCtx) {
        fprintf(stderr, "Out of memory\n");
        return EXIT_FAILURE;
    }

    if (!sparseOpen(sparseCtx, &cbOpen, &cbClose, &cbRead, &cbSeek, nullptr,
                    &ctx)) {
        sparseCtxFree(sparseCtx);
        return EXIT_FAILURE;
    }

    io::File file;
    if (!file.open(outputFile, io::File::OpenWrite)) {
        fprintf(stderr, "%s: Failed to open for writing: %s\n",
                outputFile, file.errorString().c_str());
        sparseCtxFree(sparseCtx);
        return EXIT_FAILURE;
    }

    uint64_t bytesRead;
    char buf[10240];
    bool ret;
    while ((ret = sparseRead(sparseCtx, buf, sizeof(buf), &bytesRead))
            && bytesRead > 0) {
        char *ptr = buf;
        uint64_t bytesWritten;
        while (bytesRead > 0) {
            if (!file.write(ptr, bytesRead, &bytesWritten)) {
                fprintf(stderr, "%s: Failed to write: %s\n",
                        outputFile, file.errorString().c_str());
                sparseCtxFree(sparseCtx);
                return EXIT_FAILURE;
            }
            bytesRead -= bytesWritten;
            ptr += bytesWritten;
        }
    }
    if (!ret) {
        sparseCtxFree(sparseCtx);
        return EXIT_FAILURE;
    }

    sparseCtxFree(sparseCtx);

    return file.close() ? EXIT_SUCCESS : EXIT_FAILURE;
}