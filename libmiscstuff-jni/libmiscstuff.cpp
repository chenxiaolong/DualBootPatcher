/*
 * Copyright (C) 2014-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/vfs.h>

#include <archive.h>
#include <archive_entry.h>

#include <jni.h>

#include "mblog/android_logger.h"
#include "mblog/logging.h"


#define CLASS_METHOD(method) \
    Java_com_github_chenxiaolong_dualbootpatcher_nativelib_libmiscstuff_LibMiscStuff_ ## method

#define IOException             "java/io/IOException"
#define OutOfMemoryError        "java/lang/OutOfMemoryError"

extern "C" {

__attribute__((format(printf, 3, 4)))
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

JNIEXPORT jboolean JNICALL
CLASS_METHOD(findStringInFile)(JNIEnv *env, jclass clazz, jstring jpath,
                               jstring jstr)
{
    (void) clazz;

    jboolean ret = JNI_FALSE;
    const char *path = nullptr;
    const char *str = nullptr;
    struct stat sb;
    void *map = MAP_FAILED;
    int fd = -1;

    path = env->GetStringUTFChars(jpath, nullptr);
    if (!path) {
        goto done;
    }

    str = env->GetStringUTFChars(jstr, nullptr);
    if (!str) {
        goto done;
    }

    if ((fd = open(path, O_RDONLY)) < 0) {
        throw_exception(env, IOException,
                        "%s: Failed to open file: %s",
                        path, strerror(errno));
        goto done;
    }

    if (fstat(fd, &sb) < 0) {
        throw_exception(env, IOException,
                        "%s: Failed to stat: %s",
                        path, strerror(errno));
        goto done;
    }

    map = mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) {
        throw_exception(env, IOException,
                        "%s: Failed to mmap: %s",
                        path, strerror(errno));
        goto done;
    }

    ret = memmem(map, sb.st_size, str, strlen(str)) ? JNI_TRUE : JNI_FALSE;

done:
    if (map != MAP_FAILED) {
        munmap(map, sb.st_size);
    }
    if (fd >= 0) {
        close(fd);
    }
    if (path) {
        env->ReleaseStringUTFChars(jpath, path);
    }
    if (str) {
        env->ReleaseStringUTFChars(jstr, str);
    }

    return ret;
}

JNIEXPORT void JNICALL
CLASS_METHOD(mblogSetLogcat)(JNIEnv *env, jclass clazz)
{
    (void) env;
    (void) clazz;

    mb::log::set_log_tag("libmbp");
    mb::log::log_set_logger(std::make_shared<mb::log::AndroidLogger>());
}

JNIEXPORT jstring JNICALL
CLASS_METHOD(readLink)(JNIEnv *env, jclass clazz, jstring jpath)
{
    (void) clazz;

    jstring result = nullptr;
    const char *path = nullptr;
    size_t buf_size = 64;
    char *buf = nullptr;
    ssize_t n;

    path = env->GetStringUTFChars(jpath, nullptr);
    if (!path) {
        goto done;
    }

    buf = static_cast<char *>(malloc(buf_size));
    if (!buf) {
        throw_exception(env, OutOfMemoryError, "Out of memory");
        goto done;
    }

    for (;;) {
        n = readlink(path, buf, buf_size);
        if (n < 0) {
            throw_exception(env, IOException,
                            "%s: Failed to read link: %s",
                            path, strerror(errno));
            goto done;
        } else if ((size_t) n == buf_size) {
            char *new_buf = static_cast<char *>(realloc(buf, buf_size << 1));
            if (!new_buf) {
                throw_exception(env, OutOfMemoryError, "Out of memory");
                goto done;
            }
            buf = new_buf;
            buf_size <<= 1;
        } else {
            break;
        }
    }

    // n is always less then buf_size because the buffer size is doubled when
    // n == buf_size
    buf[n] = '\0';

    result = env->NewStringUTF(buf);

done:
    free(buf);

    if (path) {
        env->ReleaseStringUTFChars(jpath, path);
    }

    return result;
}

}
