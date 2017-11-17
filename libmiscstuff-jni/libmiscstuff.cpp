/*
 * Copyright (C) 2014-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <cerrno>
#include <cstdarg>
#include <cstdlib>
#include <cstring>

#include <archive.h>
#include <archive_entry.h>

#include <jni.h>

#include "mbcommon/common.h"

#include "mbbootimg/entry.h"
#include "mbbootimg/header.h"
#include "mbbootimg/reader.h"

#include "mblog/android_logger.h"
#include "mblog/logging.h"

#define LOG_TAG "libmiscstuff-jni/libmiscstuff"

#define CLASS_METHOD(method) \
    Java_com_github_chenxiaolong_dualbootpatcher_nativelib_libmiscstuff_LibMiscStuff_ ## method

#define IOException             "java/io/IOException"
#define OutOfMemoryError        "java/lang/OutOfMemoryError"

using namespace mb::bootimg;

typedef std::unique_ptr<archive, decltype(archive_free) *> ScopedArchive;

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

    mb::log::set_logger(std::make_shared<mb::log::AndroidLogger>());
}

struct LaBootImgCtx
{
    Reader reader;
    char buf[10240];
};

static la_ssize_t laBootImgReadCb(archive *a, void *userdata,
                                  const void **buffer)
{
    (void) a;
    LaBootImgCtx *ctx = static_cast<LaBootImgCtx *>(userdata);
    size_t bytesRead;

    if (!ctx->reader.read_data(ctx->buf, sizeof(ctx->buf), bytesRead)) {
        return -1;
    }

    *buffer = ctx->buf;
    return static_cast<la_ssize_t>(bytesRead);
}

JNIEXPORT jstring JNICALL
CLASS_METHOD(getBootImageRomId)(JNIEnv *env, jclass clazz, jstring jfilename)
{
    (void) clazz;

    Reader reader;
    Header header;
    Entry entry;
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

    if (!a) {
        throw_exception(env, IOException, "Failed to allocate archive");
        goto done;
    }

    // Open input boot image
    if (!reader.enable_format_all()) {
        throw_exception(env, IOException,
                        "Failed to enable all boot image formats: %s",
                        reader.error_string().c_str());
        goto done;
    }
    if (!reader.open_filename(filename)) {
        throw_exception(env, IOException,
                        "%s: Failed to open boot image for reading: %s",
                        filename, reader.error_string().c_str());
        goto done;
    }

    // Read header
    if (!reader.read_header(header)) {
        throw_exception(env, IOException,
                        "%s: Failed to read header: %s",
                        filename, reader.error_string().c_str());
        goto done;
    }

    // Go to ramdisk
    if (!reader.go_to_entry(entry, ENTRY_TYPE_RAMDISK)) {
        if (reader.error() == ReaderError::EndOfEntries) {
            throw_exception(env, IOException,
                            "%s: Boot image is missing ramdisk", filename);
        } else {
            throw_exception(env, IOException,
                            "%s: Failed to find ramdisk entry: %s",
                            filename, reader.error_string().c_str());
        }
        goto done;
    }

    // Enable support for common ramdisk formats
    archive_read_support_filter_gzip(a.get());
    archive_read_support_filter_lz4(a.get());
    archive_read_support_filter_lzma(a.get());
    archive_read_support_filter_xz(a.get());
    archive_read_support_format_cpio(a.get());

    // Open ramdisk archive
    ctx.reader = std::move(reader);
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

JNIEXPORT jboolean JNICALL
CLASS_METHOD(bootImagesEqual)(JNIEnv *env, jclass clazz, jstring jfilename1,
                              jstring jfilename2)
{
    (void) clazz;

    Reader reader1;
    Reader reader2;
    Header header1;
    Header header2;
    Entry entry1;
    Entry entry2;
    size_t entries = 0;
    const char *filename1 = nullptr;
    const char *filename2 = nullptr;
    jboolean result = false;

    filename1 = env->GetStringUTFChars(jfilename1, nullptr);
    if (!filename1) {
        goto done;
    }
    filename2 = env->GetStringUTFChars(jfilename2, nullptr);
    if (!filename2) {
        goto done;
    }

    // Set up reader formats
    if (!reader1.enable_format_all()) {
        throw_exception(env, IOException,
                        "Failed to enable all boot image formats: %s",
                        reader1.error_string().c_str());
        goto done;
    }
    if (!reader2.enable_format_all()) {
        throw_exception(env, IOException,
                        "Failed to enable all boot image formats: %s",
                        reader2.error_string().c_str());
        goto done;
    }

    // Open boot images
    if (!reader1.open_filename(filename1)) {
        throw_exception(env, IOException,
                        "%s: Failed to open boot image for reading: %s",
                        filename1, reader1.error_string().c_str());
        goto done;
    }
    if (!reader2.open_filename(filename2)) {
        throw_exception(env, IOException,
                        "%s: Failed to open boot image for reading: %s",
                        filename2, reader2.error_string().c_str());
        goto done;
    }

    // Read headers
    if (!reader1.read_header(header1)) {
        throw_exception(env, IOException,
                        "%s: Failed to read header: %s",
                        filename1, reader1.error_string().c_str());
        goto done;
    }
    if (!reader2.read_header(header2)) {
        throw_exception(env, IOException,
                        "%s: Failed to read header: %s",
                        filename2, reader2.error_string().c_str());
        goto done;
    }

    // Compare headers
    if (header1 != header2) {
        goto done;
    }

    // Count entries in first boot image
    {
        while (true) {
            if (!reader1.read_entry(entry1)) {
                if (reader1.error() == ReaderError::EndOfEntries) {
                    break;
                }
                throw_exception(env, IOException,
                                "%s: Failed to read entry: %s",
                                filename1, reader1.error_string().c_str());
                goto done;
            }
            ++entries;
        }
    }

    // Compare each entry in second image to first
    {
        while (true) {
            if (!reader2.read_entry(entry2)) {
                if (reader2.error() == ReaderError::EndOfEntries) {
                    break;
                }
                throw_exception(env, IOException,
                                "%s: Failed to read entry: %s",
                                filename2, reader2.error_string().c_str());
                goto done;
            }

            if (entries == 0) {
                // Too few entries in second image
                goto done;
            }
            --entries;

            // Find the same entry in first image
            if (!reader1.go_to_entry(entry1, *entry2.type())) {
                if (reader1.error() == ReaderError::EndOfEntries) {
                    // Cannot be equal if entry is missing
                } else {
                    throw_exception(env, IOException,
                                    "%s: Failed to seek to entry: %s", filename1,
                                    reader1.error_string().c_str());
                }
                goto done;
            }

            // Compare data
            char buf1[10240];
            char buf2[10240];
            size_t n1;
            size_t n2;

            while (true) {
                if (!reader1.read_data(buf1, sizeof(buf1), n1)) {
                    throw_exception(env, IOException,
                                    "%s: Failed to read data: %s", filename1,
                                    reader1.error_string().c_str());
                    goto done;
                } else if (n1 == 0) {
                    break;
                }

                if (!reader2.read_data(buf2, n1, n2)) {
                    throw_exception(env, IOException,
                                    "%s: Failed to read data: %s", filename2,
                                    reader2.error_string().c_str());
                    goto done;
                }

                if (n1 != n2 || memcmp(buf1, buf2, n1) != 0) {
                    // Data is not equivalent
                    goto done;
                }
            }
        }
    }

    result = true;

done:
    if (filename1) {
        env->ReleaseStringUTFChars(jfilename1, filename1);
    }
    if (filename2) {
        env->ReleaseStringUTFChars(jfilename2, filename2);
    }

    return result;
}

}
