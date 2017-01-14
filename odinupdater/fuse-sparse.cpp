#define FUSE_USE_VERSION 26

#include <new>

#include <cerrno>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>

// fuse
#include <fuse/fuse.h>

// libmbcommon
#include "mbcommon/file.h"
#include "mbcommon/file/filename.h"

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
    SparseCtx *sctx;
    MbFile *file;
    pthread_mutex_t mutex;
};

/*!
 * \brief Open callback for sparseOpen()
 */
static bool cb_open(void *userData)
{
    context *ctx = static_cast<context *>(userData);
    if (mb_file_open_filename(ctx->file, source_fd_path, MB_FILE_OPEN_READ_ONLY)
            != MB_FILE_OK) {
        fprintf(stderr, "%s: Failed to open: %s\n",
                source_fd_path, mb_file_error_string(ctx->file));
        return false;
    }
    return true;
}

/*!
 * \brief Close callback for sparseOpen()
 */
static bool cb_close(void *userData)
{
    context *ctx = static_cast<context *>(userData);
    if (mb_file_close(ctx->file) != MB_FILE_OK) {
        fprintf(stderr, "%s: Failed to close: %s\n",
                source_fd_path, mb_file_error_string(ctx->file));
        return false;
    }
    return true;
}

/*!
 * \brief Read callback for sparseOpen()
 */
static bool cb_read(void *buf, uint64_t size, uint64_t *bytesRead,
                    void *userData)
{
    context *ctx = static_cast<context *>(userData);
    size_t total = 0;
    while (size > 0) {
        size_t partial;
        if (mb_file_read(ctx->file, buf, size, &partial) != MB_FILE_OK) {
            fprintf(stderr, "%s: Failed to read: %s\n",
                    source_fd_path, mb_file_error_string(ctx->file));
            return false;
        }
        size -= partial;
        total += partial;
        buf = static_cast<char *>(buf) + partial;
    }
    *bytesRead = total;
    return true;
}

/*!
 * \brief Seek callback for sparseOpen()
 */
static bool cb_seek(int64_t offset, int whence, void *userData)
{
    context *ctx = static_cast<context *>(userData);
    if (mb_file_seek(ctx->file, offset, whence, nullptr) != MB_FILE_OK) {
        fprintf(stderr, "%s: Failed to seek: %s\n",
                source_fd_path, mb_file_error_string(ctx->file));
        return false;
    }
    return true;
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

    ctx->sctx = sparseCtxNew();
    if (!ctx->sctx) {
        delete ctx;
        return -ENOMEM;
    }

    ctx->file = mb_file_new();
    if (!ctx->file) {
        sparseCtxFree(ctx->sctx);
        delete ctx;
        return -ENOMEM;
    }

    if (!sparseOpen(ctx->sctx, &cb_open, &cb_close, &cb_read, &cb_seek, nullptr,
                    ctx)) {
        sparseCtxFree(ctx->sctx);
        mb_file_free(ctx->file);
        delete ctx;
        return -EIO;
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

    context *ctx = reinterpret_cast<context *>(fi->fh);
    sparseCtxFree(ctx->sctx);
    mb_file_free(ctx->file);
    delete ctx;

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
    if (!sparseSeek(ctx->sctx, offset, SEEK_SET)) {
        return -1;
    }

    uint64_t bytes_read;
    if (!sparseRead(ctx->sctx, buf, size, &bytes_read)) {
        return -1;
    }

    return bytes_read;
}

/*!
 * \brief Read callback for fuse
 */
static int fuse_read(const char *path, char *buf, size_t size, OFF_T offset,
                     fuse_file_info *fi)
{
    (void) path;

    context *ctx = reinterpret_cast<context *>(fi->fh);

    pthread_mutex_lock(&ctx->mutex);
    int ret = fuse_read_locked(ctx, buf, size, offset);
    pthread_mutex_unlock(&ctx->mutex);

    return ret;
}

/*!
 * \brief getattr (stat) callback for fuse
 */
static int fuse_getattr(const char *path, struct stat *stbuf)
{
    (void) path;

    stbuf->st_mode = S_IFREG | 0444;
    stbuf->st_size = sparse_size;

    return 0;
}

/*!
 * \brief Get size of sparse file (needed for fuse_getattr())
 */
static int get_sparse_file_size()
{
    context *ctx = new(std::nothrow) context();
    if (!ctx) {
        return -ENOMEM;
    }

    ctx->sctx = sparseCtxNew();
    if (!ctx->sctx) {
        delete ctx;
        return -ENOMEM;
    }

    ctx->file = mb_file_new();
    if (!ctx->file) {
        sparseCtxFree(ctx->sctx);
        delete ctx;
        return -ENOMEM;
    }

    if (!sparseOpen(ctx->sctx, &cb_open, &cb_close, &cb_read, &cb_seek, nullptr,
                    ctx)) {
        sparseCtxFree(ctx->sctx);
        mb_file_free(ctx->file);
        delete ctx;
        return -EIO;
    }

    if (!sparseSize(ctx->sctx, &sparse_size)) {
        sparseCtxFree(ctx->sctx);
        mb_file_free(ctx->file);
        delete ctx;
        return -EIO;
    }

    sparseCtxFree(ctx->sctx);
    mb_file_free(ctx->file);
    delete ctx;
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

        fd = open(arg_ctx.source_file, O_RDONLY);
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

    fuse_operations fuse_oper;
    memset(&fuse_oper, 0, sizeof(fuse_oper));
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
