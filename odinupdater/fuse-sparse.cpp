/*
 * Copyright (C) 2016-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#define FUSE_USE_VERSION 26

#include <mutex>
#include <new>
#include <optional>

#include <cerrno>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <fcntl.h>
#include <unistd.h>

#ifdef __clang__
#  pragma GCC diagnostic push
#  if __has_warning("-Wdocumentation")
#    pragma GCC diagnostic ignored "-Wdocumentation"
#  endif
#endif

// fuse
// We have to define _FILE_OFFSET_BITS because <fuse/fuse_common.h> requires it.
// However, since it's no longer a no-op in NDK r15c and newer, we can't define
// it globally. Bionic in API <24 doesn't support _FILE_OFFSET_BITS.
// See: https://github.com/android-ndk/ndk/issues/480
#ifdef __ANDROID__
#  define _FILE_OFFSET_BITS 64
#endif
#include <fuse/fuse.h>
#ifdef __ANDROID__
#  undef _FILE_OFFSET_BITS
#endif

#ifdef __clang__
#  pragma GCC diagnostic pop
#endif

// libmbcommon
#include "mbcommon/file.h"
#include "mbcommon/file/standard.h"

// libmbsparse
#include "mbsparse/sparse.h"

#ifdef __ANDROID__
#define OFF_T loff_t
#else
#define OFF_T off_t
#endif

static char source_fd_path[50];
static uint64_t sparse_size;

struct context
{
    mb::StandardFile source_file;
    mb::sparse::SparseFile sparse_file;
    std::mutex mutex;
};

static std::optional<int> extract_errno(std::error_code ec)
{
    if (ec.category() == std::generic_category()
            || ec.category() == std::system_category()) {
        return ec.value();
    }

    return {};
}

/*!
 * \brief Open callback for fuse
 */
static int fuse_open(const char *path, fuse_file_info *fi)
{
    (void) path;

    if (fi->flags & (O_WRONLY | O_RDWR)) {
        return -EROFS;
    }

    context *ctx = new(std::nothrow) context();
    if (!ctx) {
        return -ENOMEM;
    }

    auto ret = ctx->source_file.open(source_fd_path,
                                     mb::FileOpenMode::ReadOnly);
    if (!ret) {
        fprintf(stderr, "%s: Failed to open file: %s\n",
                source_fd_path, ret.error().message().c_str());
        delete ctx;
        return -extract_errno(ret.error()).value_or(EIO);
    }

    ret = ctx->sparse_file.open(&ctx->source_file);
    if (!ret) {
        fprintf(stderr, "%s: Failed to open sparse file: %s\n",
                source_fd_path, ret.error().message().c_str());
        delete ctx;
        return -extract_errno(ret.error()).value_or(EIO);
    }

    fi->fh = reinterpret_cast<uint64_t>(ctx);

    return 0;
}

/*!
 * \brief Release callback for fuse
 */
static int fuse_release(const char *path, fuse_file_info *fi)
{
    (void) path;

    delete reinterpret_cast<context *>(fi->fh);

    return 0;
}

/*!
 * \brief Read from sparse file
 *
 * \warning Bad stuff will happen if \a ctx->mutex is not locked when this
 *          function is called
 */
static int fuse_read_locked(context *ctx, char *buf, size_t size,
                            OFF_T offset)
{
    // Seek to position
    auto new_offset = ctx->sparse_file.seek(offset, SEEK_SET);
    if (!new_offset) {
        return -extract_errno(new_offset.error()).value_or(EIO);
    }

    auto n = ctx->sparse_file.read(buf, size);
    if (!n) {
        return -extract_errno(n.error()).value_or(EIO);
    }

    return static_cast<int>(n.value());
}

/*!
 * \brief Read callback for fuse
 */
static int fuse_read(const char *path, char *buf, size_t size, OFF_T offset,
                     fuse_file_info *fi)
{
    (void) path;

    context *ctx = reinterpret_cast<context *>(fi->fh);

    std::lock_guard<std::mutex> lock(ctx->mutex);
    return fuse_read_locked(ctx, buf, size, offset);
}

/*!
 * \brief getattr (stat) callback for fuse
 */
static int fuse_getattr(const char *path, struct stat *stbuf)
{
    (void) path;

    stbuf->st_mode = S_IFREG | 0444;
    stbuf->st_size = static_cast<OFF_T>(sparse_size);

    return 0;
}

/*!
 * \brief Get size of sparse file (needed for fuse_getattr())
 */
static int get_sparse_file_size()
{
    mb::StandardFile source_file;
    mb::sparse::SparseFile sparse_file;

    auto ret = source_file.open(source_fd_path, mb::FileOpenMode::ReadOnly);
    if (!ret) {
        fprintf(stderr, "%s: Failed to open file: %s\n",
                source_fd_path, ret.error().message().c_str());
        return -extract_errno(ret.error()).value_or(EIO);
    }

    ret = sparse_file.open(&source_file);
    if (!ret) {
        fprintf(stderr, "%s: Failed to open sparse file: %s\n",
                source_fd_path, ret.error().message().c_str());
        return -extract_errno(ret.error()).value_or(EIO);
    }

    sparse_size = sparse_file.size();

    return 0;
}

struct arg_ctx
{
    char *source_file = nullptr;
    char *target_file = nullptr;
    bool show_help = false;
};

enum
{
    KEY_HELP
};

static fuse_opt fuse_opts[] =
{
    FUSE_OPT_KEY("-h",     KEY_HELP),
    FUSE_OPT_KEY("--help", KEY_HELP),
    FUSE_OPT_END
};

static void usage(FILE *stream, const char *progname)
{
    fprintf(stream,
            "Usage: %s <sparse file> <target file> [options]\n"
            "\n"
            "general options:\n"
            "    -o opt,[opt...]        comma-separated list of mount options\n"
            "    -h   --help            show this help message\n"
            "\n",
            progname);
}

static int fuse_opt_proc(void *data, const char *arg, int key,
                         fuse_args *outargs)
{
    arg_ctx *ctx = static_cast<arg_ctx *>(data);

    switch (key) {
    case FUSE_OPT_KEY_NONOPT:
        if (!ctx->source_file) {
            ctx->source_file = strdup(arg);
            return 0;
        } else if (!ctx->target_file) {
            ctx->target_file = strdup(arg);
        }
        return 1;

    case KEY_HELP:
        usage(stdout, outargs->argv[0]);
        ctx->show_help = true;
        return fuse_opt_add_arg(outargs, "-ho");

    default:
        return 1;
    }
}

int main(int argc, char *argv[])
{
    fuse_args args = FUSE_ARGS_INIT(argc, argv);
    arg_ctx arg_ctx;

    if (fuse_opt_parse(&args, &arg_ctx, fuse_opts, fuse_opt_proc) == -1) {
        return EXIT_FAILURE;
    }

    int fd = -1;

    if (!arg_ctx.show_help) {
        if (!arg_ctx.source_file) {
            fprintf(stderr, "Missing source file\n");
            return EXIT_FAILURE;
        }
        if (!arg_ctx.target_file) {
            fprintf(stderr, "Missing target file (mount point) parameter\n");
            return EXIT_FAILURE;
        }

        fd = open(arg_ctx.source_file, O_RDONLY | O_CLOEXEC);
        if (fd < 0) {
            fprintf(stderr, "%s: Failed to open: %s\n",
                    arg_ctx.source_file, strerror(errno));
            return EXIT_FAILURE;
        }
        snprintf(source_fd_path, sizeof(source_fd_path),
                 "/proc/self/fd/%d", fd);

        if (get_sparse_file_size() < 0) {
            close(fd);
            return EXIT_FAILURE;
        }
    }

    fuse_operations fuse_oper = {};
    fuse_oper.getattr = fuse_getattr;
    fuse_oper.open    = fuse_open;
    fuse_oper.read    = fuse_read;
    fuse_oper.release = fuse_release;

    int fuse_ret = fuse_main(args.argc, args.argv, &fuse_oper, nullptr);

    if (!arg_ctx.show_help) {
        close(fd);
    }

    fuse_opt_free_args(&args);
    free(arg_ctx.source_file);
    free(arg_ctx.target_file);

    return fuse_ret;
}
