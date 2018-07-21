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
#include "mbcommon/finally.h"

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

using namespace mb;
using namespace mb::bootimg;

typedef std::unique_ptr<archive, decltype(archive_free) *> ScopedArchive;

extern "C" {

MB_PRINTF(3, 4)
static bool throw_exception(JNIEnv *env, const char *class_name,
                            const char *fmt, ...)
{
    char *buf;
    va_list ap;

    va_start(ap, fmt);
    int ret = vasprintf(&buf, fmt, ap);
    va_end(ap);

    if (ret < 0) {
        // Can't propagate the error any further
        abort();
    }

    auto free_buf = finally([&] {
        free(buf);
    });

    jclass clazz = env->FindClass(class_name);
    if (!clazz) {
        // Java will throw an exception after returning
        return false;
    }

    return env->ThrowNew(clazz, buf) == 0;
}

JNIEXPORT void JNICALL
CLASS_METHOD(extractArchive)(JNIEnv *env, jclass clazz, jstring jfilename,
                            jstring jtarget)
{
    (void) clazz;

    const char *filename = env->GetStringUTFChars(jfilename, nullptr);
    if (!filename) {
        return;
    }

    auto free_filename = finally([&] {
        if (filename) {
            env->ReleaseStringUTFChars(jfilename, filename);
        }
    });

    const char *target = env->GetStringUTFChars(jtarget, nullptr);
    if (!target) {
        return;
    }

    auto free_target = finally([&] {
        if (target) {
            env->ReleaseStringUTFChars(jtarget, target);
        }
    });

    ScopedArchive in(archive_read_new(), &archive_read_free);
    if (!in) {
        throw_exception(env, OutOfMemoryError, "Failed to allocate archive");
        return;
    }

    // Add more as needed
    //archive_read_support_format_all(in.get());
    //archive_read_support_filter_all(in.get());
    archive_read_support_format_tar(in.get());
    archive_read_support_format_zip(in.get());
    archive_read_support_filter_xz(in.get());

    if (archive_read_open_filename(in.get(), filename, 10240) != ARCHIVE_OK) {
        throw_exception(env, IOException,
                        "%s: Failed to open archive: %s",
                        filename, archive_error_string(in.get()));
        return;
    }

    ScopedArchive out(archive_write_disk_new(), &archive_write_free);
    if (!out) {
        throw_exception(env, OutOfMemoryError, "Failed to allocate archive");
        return;
    }

    archive_write_disk_set_options(out.get(),
                                   ARCHIVE_EXTRACT_ACL |
                                   ARCHIVE_EXTRACT_FFLAGS |
                                   ARCHIVE_EXTRACT_PERM |
                                   ARCHIVE_EXTRACT_SECURE_NODOTDOT |
                                   ARCHIVE_EXTRACT_SECURE_SYMLINKS |
                                   ARCHIVE_EXTRACT_TIME |
                                   ARCHIVE_EXTRACT_UNLINK |
                                   ARCHIVE_EXTRACT_XATTR);

    const char *cwd = getcwd(nullptr, 0);
    if (!cwd) {
        throw_exception(env, IOException,
                        "Failed to get cwd: %s", strerror(errno));
        return;
    }

    if (chdir(target) < 0) {
        throw_exception(env, IOException,
                        "%s: Failed to change to target directory: %s",
                        target, strerror(errno));
        return;
    }

    auto restore_cwd = finally([&] {
        chdir(cwd);
    });

    archive_entry *entry;
    int laret;

    while ((laret = archive_read_next_header(in.get(), &entry)) == ARCHIVE_OK) {
        if ((laret = archive_write_header(out.get(), entry)) != ARCHIVE_OK) {
            throw_exception(env, IOException,
                            "%s: Failed to write header: %s",
                            target, archive_error_string(out.get()));
            return;
        }

        const void *buff;
        size_t size;
        int64_t offset;

        while ((laret = archive_read_data_block(
                in.get(), &buff, &size, &offset)) == ARCHIVE_OK) {
            if (archive_write_data_block(out.get(), buff, size, offset)
                    != ARCHIVE_OK) {
                throw_exception(env, IOException,
                                "%s: Failed to write data: %s",
                                target, archive_error_string(out.get()));
                return;
            }
        }

        if (laret != ARCHIVE_EOF) {
            throw_exception(env, IOException,
                            "%s: Data copy ended without reaching EOF: %s",
                            filename, archive_error_string(in.get()));
            return;
        }
    }

    if (laret != ARCHIVE_EOF) {
        throw_exception(env, IOException,
                        "%s: Archive extraction ended without reaching EOF: %s",
                        filename, archive_error_string(in.get()));
        return;
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

    auto bytesRead = ctx->reader.read_data(ctx->buf, sizeof(ctx->buf));
    if (!bytesRead) {
        return -1;
    }

    *buffer = ctx->buf;
    return static_cast<la_ssize_t>(bytesRead.value());
}

JNIEXPORT jstring JNICALL
CLASS_METHOD(getBootImageRomId)(JNIEnv *env, jclass clazz, jstring jfilename)
{
    (void) clazz;

    Reader reader;
    Header header;
    Entry entry;

    const char *filename = env->GetStringUTFChars(jfilename, nullptr);
    if (!filename) {
        return nullptr;
    }

    auto free_filename = finally([&] {
        if (filename) {
            env->ReleaseStringUTFChars(jfilename, filename);
        }
    });

    // Open input boot image
    auto ret = reader.enable_format_all();
    if (!ret) {
        throw_exception(env, IOException,
                        "Failed to enable all boot image formats: %s",
                        ret.error().message().c_str());
        return nullptr;
    }
    ret = reader.open_filename(filename);
    if (!ret) {
        throw_exception(env, IOException,
                        "%s: Failed to open boot image for reading: %s",
                        filename, ret.error().message().c_str());
        return nullptr;
    }

    // Read header
    ret = reader.read_header(header);
    if (!ret) {
        throw_exception(env, IOException,
                        "%s: Failed to read header: %s",
                        filename, ret.error().message().c_str());
        return nullptr;
    }

    // Go to ramdisk
    ret = reader.go_to_entry(entry, ENTRY_TYPE_RAMDISK);
    if (!ret) {
        if (ret.error() == ReaderError::EndOfEntries) {
            throw_exception(env, IOException,
                            "%s: Boot image is missing ramdisk", filename);
        } else {
            throw_exception(env, IOException,
                            "%s: Failed to find ramdisk entry: %s",
                            filename, ret.error().message().c_str());
        }
        return nullptr;
    }

    ScopedArchive a(archive_read_new(), &archive_read_free);
    archive_entry *aEntry;
    LaBootImgCtx ctx;

    if (!a) {
        throw_exception(env, IOException, "Failed to allocate archive");
        return nullptr;
    }

    // Enable support for common ramdisk formats
    archive_read_support_filter_gzip(a.get());
    archive_read_support_filter_lz4(a.get());
    archive_read_support_filter_lzma(a.get());
    archive_read_support_filter_xz(a.get());
    archive_read_support_format_cpio(a.get());

    // Open ramdisk archive
    ctx.reader = std::move(reader);
    int laret = archive_read_open(a.get(), &ctx, nullptr, &laBootImgReadCb,
                                  nullptr);
    if (laret != ARCHIVE_OK) {
        throw_exception(env, IOException,
                        "%s: Failed to open ramdisk: %s",
                        filename, archive_error_string(a.get()));
        return nullptr;
    }

    while ((laret = archive_read_next_header(a.get(), &aEntry)) == ARCHIVE_OK) {
        const char *path = archive_entry_pathname(aEntry);
        if (!path) {
            throw_exception(env, IOException,
                            "%s: Ramdisk entry has no path", filename);
            return nullptr;
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
                return nullptr;
            }

            // NULL-terminate
            buf[n_read] = '\0';

            // Ensure that EOF is reached
            n_read = archive_read_data(a.get(), &dummy, 1);
            if (n_read != 0) {
                throw_exception(env, IOException,
                                "%s: /romid in ramdisk is too large",
                                filename);
                return nullptr;
            }

            return env->NewStringUTF(buf);
        }
    }

    if (laret != ARCHIVE_EOF) {
        throw_exception(env, IOException,
                        "%s: Failed to read ramdisk entry header: %s",
                        filename, archive_error_string(a.get()));
        return nullptr;
    }

    return nullptr;
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

    const char *filename1 = env->GetStringUTFChars(jfilename1, nullptr);
    if (!filename1) {
        return false;
    }

    auto free_filename1 = finally([&] {
        if (filename1) {
            env->ReleaseStringUTFChars(jfilename1, filename1);
        }
    });

    const char *filename2 = env->GetStringUTFChars(jfilename2, nullptr);
    if (!filename2) {
        return false;
    }

    auto free_filename2 = finally([&] {
        if (filename2) {
            env->ReleaseStringUTFChars(jfilename2, filename2);
        }
    });

    // Set up reader formats
    auto ret = reader1.enable_format_all();
    if (!ret) {
        throw_exception(env, IOException,
                        "Failed to enable all boot image formats: %s",
                        ret.error().message().c_str());
        return false;
    }
    ret = reader2.enable_format_all();
    if (!ret) {
        throw_exception(env, IOException,
                        "Failed to enable all boot image formats: %s",
                        ret.error().message().c_str());
        return false;
    }

    // Open boot images
    ret = reader1.open_filename(filename1);
    if (!ret) {
        throw_exception(env, IOException,
                        "%s: Failed to open boot image for reading: %s",
                        filename1, ret.error().message().c_str());
        return false;
    }
    ret = reader2.open_filename(filename2);
    if (!ret) {
        throw_exception(env, IOException,
                        "%s: Failed to open boot image for reading: %s",
                        filename2, ret.error().message().c_str());
        return false;
    }

    // Read headers
    ret = reader1.read_header(header1);
    if (!ret) {
        throw_exception(env, IOException,
                        "%s: Failed to read header: %s",
                        filename1, ret.error().message().c_str());
        return false;
    }
    ret = reader2.read_header(header2);
    if (!ret) {
        throw_exception(env, IOException,
                        "%s: Failed to read header: %s",
                        filename2, ret.error().message().c_str());
        return false;
    }

    // Compare headers
    if (header1 != header2) {
        return false;
    }

    // Count entries in first boot image
    {
        while (true) {
            ret = reader1.read_entry(entry1);
            if (!ret) {
                if (ret.error() == ReaderError::EndOfEntries) {
                    break;
                }
                throw_exception(env, IOException,
                                "%s: Failed to read entry: %s",
                                filename1, ret.error().message().c_str());
                return false;
            }
            ++entries;
        }
    }

    // Compare each entry in second image to first
    {
        while (true) {
            ret = reader2.read_entry(entry2);
            if (!ret) {
                if (ret.error() == ReaderError::EndOfEntries) {
                    break;
                }
                throw_exception(env, IOException,
                                "%s: Failed to read entry: %s",
                                filename2, ret.error().message().c_str());
                return false;
            }

            if (entries == 0) {
                // Too few entries in second image
                return false;
            }
            --entries;

            // Find the same entry in first image
            ret = reader1.go_to_entry(entry1, *entry2.type());
            if (!ret) {
                if (ret.error() == ReaderError::EndOfEntries) {
                    // Cannot be equal if entry is missing
                } else {
                    throw_exception(env, IOException,
                                    "%s: Failed to seek to entry: %s", filename1,
                                    ret.error().message().c_str());
                }
                return false;
            }

            // Compare data
            char buf1[10240];
            char buf2[10240];

            while (true) {
                auto n1 = reader1.read_data(buf1, sizeof(buf1));
                if (!n1) {
                    throw_exception(env, IOException,
                                    "%s: Failed to read data: %s", filename1,
                                    n1.error().message().c_str());
                    return false;
                } else if (n1.value() == 0) {
                    break;
                }

                auto n2 = reader2.read_data(buf2, n1.value());
                if (!n2) {
                    throw_exception(env, IOException,
                                    "%s: Failed to read data: %s", filename2,
                                    n2.error().message().c_str());
                    return false;
                }

                if (n1.value() != n2.value()
                        || memcmp(buf1, buf2, n1.value()) != 0) {
                    // Data is not equivalent
                    return false;
                }
            }
        }
    }

    return true;
}

}
