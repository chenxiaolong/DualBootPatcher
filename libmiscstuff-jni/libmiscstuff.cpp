/*
 * Copyright (C) 2014-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <memory>

#include <cerrno>
#include <cstdarg>
#include <cstdlib>
#include <cstring>

#include <archive.h>
#include <archive_entry.h>

#include <jni.h>

#include "mbcommon/common.h"

#include "mbbootimg/entry.h"
#include "mbbootimg/reader.h"

#include "mblog/android_logger.h"
#include "mblog/logging.h"


#define CLASS_METHOD(method) \
    Java_com_github_chenxiaolong_dualbootpatcher_nativelib_libmiscstuff_LibMiscStuff_ ## method

#define IOException             "java/io/IOException"
#define OutOfMemoryError        "java/lang/OutOfMemoryError"

typedef std::unique_ptr<archive, decltype(archive_free) *> ScopedArchive;
typedef std::unique_ptr<MbBiReader, decltype(mb_bi_reader_free) *> ScopedReader;

extern "C" {

MB_PRINTF(3, 4)
static bool throw_exception(JNIEnv *env, const char *class_name,
                            const char *fmt, ...)
{
    jclass clazz;
    char *buf;
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = vasprintf(&buf, fmt, ap);
    va_end(ap);

    if (ret < 0) {
        // Can't propagate the error any further
        abort();
    }

    clazz = env->FindClass(class_name);
    if (!clazz) {
        // Java will throw an exception after returning
        return false;
    }

    ret = env->ThrowNew(clazz, buf);
    free(buf);

    return ret == 0;
}

JNIEXPORT void JNICALL
CLASS_METHOD(extractArchive)(JNIEnv *env, jclass clazz, jstring jfilename,
                            jstring jtarget)
{
    (void) clazz;

    bool ret = false;
    const char *filename = nullptr;
    const char *target = nullptr;
    struct archive *in = nullptr;
    struct archive *out = nullptr;
    struct archive_entry *entry;
    int laret;
    char *cwd = nullptr;

    filename = env->GetStringUTFChars(jfilename, nullptr);
    if (!filename) {
        goto done;
    }

    target = env->GetStringUTFChars(jtarget, nullptr);
    if (!target) {
        goto done;
    }

    if (!(in = archive_read_new())) {
        throw_exception(env, OutOfMemoryError, "Out of memory");
        goto done;
    }

    // Add more as needed
    //archive_read_support_format_all(in);
    //archive_read_support_filter_all(in);
    archive_read_support_format_tar(in);
    archive_read_support_format_zip(in);
    archive_read_support_filter_xz(in);

    if (archive_read_open_filename(in, filename, 10240) != ARCHIVE_OK) {
        throw_exception(env, IOException,
                        "%s: Failed to open archive: %s",
                        filename, archive_error_string(in));
        goto done;
    }

    if (!(out = archive_write_disk_new())) {
        throw_exception(env, OutOfMemoryError, "Out of memory");
        goto done;
    }

    archive_write_disk_set_options(out,
                                   ARCHIVE_EXTRACT_ACL |
                                   ARCHIVE_EXTRACT_FFLAGS |
                                   ARCHIVE_EXTRACT_PERM |
                                   ARCHIVE_EXTRACT_SECURE_NODOTDOT |
                                   ARCHIVE_EXTRACT_SECURE_SYMLINKS |
                                   ARCHIVE_EXTRACT_TIME |
                                   ARCHIVE_EXTRACT_UNLINK |
                                   ARCHIVE_EXTRACT_XATTR);

    if (!(cwd = getcwd(nullptr, 0))) {
        throw_exception(env, IOException,
                        "Failed to get cwd: %s", strerror(errno));
        goto done;
    }

    if (chdir(target) < 0) {
        throw_exception(env, IOException,
                        "%s: Failed to change to target directory: %s",
                        target, strerror(errno));
        goto done;
    }

    while ((laret = archive_read_next_header(in, &entry)) == ARCHIVE_OK) {
        if ((laret = archive_write_header(out, entry)) != ARCHIVE_OK) {
            throw_exception(env, IOException,
                            "%s: Failed to write header: %s",
                            target, archive_error_string(out));
            goto done;
        }

        const void *buff;
        size_t size;
        int64_t offset;

        while ((laret = archive_read_data_block(
                in, &buff, &size, &offset)) == ARCHIVE_OK) {
            if (archive_write_data_block(out, buff, size, offset) != ARCHIVE_OK) {
                throw_exception(env, IOException,
                                "%s: Failed to write data: %s",
                                target, archive_error_string(out));
                goto done;
            }
        }

        if (laret != ARCHIVE_EOF) {
            throw_exception(env, IOException,
                            "%s: Data copy ended without reaching EOF: %s",
                            filename, archive_error_string(in));
            goto done;
        }
    }

    if (laret != ARCHIVE_EOF) {
        throw_exception(env, IOException,
                        "%s: Archive extraction ended without reaching EOF: %s",
                        filename, archive_error_string(in));
        goto done;
    }

    ret = true;

done:
    if (cwd) {
        chdir(cwd);
    }

    archive_read_free(in);
    archive_write_free(out);

    if (filename) {
        env->ReleaseStringUTFChars(jfilename, filename);
        env->ReleaseStringUTFChars(jtarget, target);
    }
}

JNIEXPORT void JNICALL
CLASS_METHOD(mblogSetLogcat)(JNIEnv *env, jclass clazz)
{
    (void) env;
    (void) clazz;

    mb::log::set_log_tag("libmbp");
    mb::log::log_set_logger(std::make_shared<mb::log::AndroidLogger>());
}

struct LaBootImgCtx
{
    MbBiReader *bir;
    char buf[10240];
};

la_ssize_t laBootImgReadCb(archive *a, void *userdata, const void **buffer)
{
    (void) a;
    LaBootImgCtx *ctx = static_cast<LaBootImgCtx *>(userdata);
    size_t bytesRead;
    int ret;

    ret = mb_bi_reader_read_data(ctx->bir, ctx->buf, sizeof(ctx->buf),
                                 &bytesRead);
    if (ret == MB_BI_EOF) {
        return 0;
    } else if (ret != MB_BI_OK) {
        return -1;
    }

    *buffer = ctx->buf;
    return static_cast<la_ssize_t>(bytesRead);
}

JNIEXPORT jstring JNICALL
CLASS_METHOD(getBootImageRomId)(JNIEnv *env, jclass clazz, jstring jfilename)
{
    (void) clazz;

    ScopedReader bir(mb_bi_reader_new(), &mb_bi_reader_free);
    MbBiHeader *header;
    MbBiEntry *entry;
    ScopedArchive a(archive_read_new(), &archive_read_free);
    archive_entry *aEntry;
    LaBootImgCtx ctx;
    int ret;
    const char *filename;
    jstring romId = nullptr;

    filename = env->GetStringUTFChars(jfilename, nullptr);
    if (!filename) {
        goto done;
    }

    if (!bir) {
        throw_exception(env, IOException, "Failed to allocate MbBiReader");
        goto done;
    } else if (!a) {
        throw_exception(env, IOException, "Failed to allocate archive");
        goto done;
    }

    // Open input boot image
    ret = mb_bi_reader_enable_format_all(bir.get());
    if (ret != MB_BI_OK) {
        throw_exception(env, IOException,
                        "Failed to enable all boot image formats: %s",
                        mb_bi_reader_error_string(bir.get()));
        goto done;
    }
    ret = mb_bi_reader_open_filename(bir.get(), filename);
    if (ret != MB_BI_OK) {
        throw_exception(env, IOException,
                        "%s: Failed to open boot image for reading: %s",
                        filename, mb_bi_reader_error_string(bir.get()));
        goto done;
    }

    // Read header
    ret = mb_bi_reader_read_header(bir.get(), &header);
    if (ret != MB_BI_OK) {
        throw_exception(env, IOException,
                        "%s: Failed to read header: %s",
                        filename, mb_bi_reader_error_string(bir.get()));
        goto done;
    }

    // Go to ramdisk
    ret = mb_bi_reader_go_to_entry(bir.get(), &entry, MB_BI_ENTRY_RAMDISK);
    if (ret == MB_BI_EOF) {
        throw_exception(env, IOException,
                        "%s: Boot image is missing ramdisk", filename);
        goto done;
    } else if (ret != MB_BI_OK) {
        throw_exception(env, IOException,
                        "%s: Failed to find ramdisk entry: %s",
                        filename, mb_bi_reader_error_string(bir.get()));
        goto done;
    }

    // Enable support for common ramdisk formats
    archive_read_support_filter_gzip(a.get());
    archive_read_support_filter_lzop(a.get());
    archive_read_support_filter_lz4(a.get());
    archive_read_support_filter_lzma(a.get());
    archive_read_support_filter_xz(a.get());
    archive_read_support_format_cpio(a.get());

    // Open ramdisk archive
    ctx.bir = bir.get();
    ret = archive_read_open(a.get(), &ctx, nullptr, &laBootImgReadCb, nullptr);
    if (ret != ARCHIVE_OK) {
        throw_exception(env, IOException,
                        "%s: Failed to open ramdisk: %s",
                        filename, archive_error_string(a.get()));
        goto done;
    }

    while ((ret = archive_read_next_header(a.get(), &aEntry)) == ARCHIVE_OK) {
        const char *path = archive_entry_pathname(aEntry);
        if (!path) {
            throw_exception(env, IOException,
                            "%s: Ramdisk entry has no path", filename);
            goto done;
        }

        if (strcmp(path, "romid") == 0) {
            char buf[32];
            char dummy;
            la_ssize_t n_read;

            n_read = archive_read_data(a.get(), buf, sizeof(buf) - 1);
            if (n_read < 0) {
                throw_exception(env, IOException,
                                "%s: Failed to read ramdisk entry: %s",
                                filename, archive_error_string(a.get()));
                goto done;
            }

            // NULL-terminate
            buf[n_read] = '\0';

            // Ensure that EOF is reached
            n_read = archive_read_data(a.get(), &dummy, 1);
            if (n_read != 0) {
                throw_exception(env, IOException,
                                "%s: /romid in ramdisk is too large",
                                filename);
                goto done;
            }

            romId = env->NewStringUTF(buf);
            goto done;
        }
    }

    if (ret != ARCHIVE_EOF) {
        throw_exception(env, IOException,
                        "%s: Failed to read ramdisk entry header: %s",
                        filename, archive_error_string(a.get()));
        goto done;
    }

done:
    if (filename) {
        env->ReleaseStringUTFChars(jfilename, filename);
    }

    return romId;
}

}
