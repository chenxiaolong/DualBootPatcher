/*
 * Copyright (C) 2015-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbp/patchers/odinpatcher.h"

#include <algorithm>
#include <thread>
#include <unordered_set>

#include <cassert>
#include <cinttypes>
#include <cstring>

#ifdef __ANDROID__
#include <cerrno>
#endif

// libarcihve
#include <archive.h>
#include <archive_entry.h>

#include "mblog/logging.h"

#include "mbpio/file.h"

#include "mbp/bootimage.h"
#include "mbp/cpiofile.h"
#include "mbp/patcherconfig.h"
#include "mbp/patchers/multibootpatcher.h"
#include "mbp/private/fileutils.h"
#include "mbp/private/miniziputils.h"
#include "mbp/private/stringutils.h"

// minizip
#include "minizip/zip.h"

class ar;

namespace mbp
{

/*! \cond INTERNAL */
class OdinPatcher::Impl
{
public:
    PatcherConfig *pc;
    const FileInfo *info;

    uint64_t oldBytes;
    uint64_t bytes;
    uint64_t maxBytes;

    volatile bool cancelled;

    ErrorCode error;

    unsigned char laBuf[10240];
    io::File laFile;
#ifdef __ANDROID__
    int fd = -1;
#endif

    // Callbacks
    ProgressUpdatedCallback progressCb;
    DetailsUpdatedCallback detailsCb;
    void *userData;

    // Patching
    archive *aInput = nullptr;
    MinizipUtils::ZipCtx *zOutput = nullptr;

    bool patchTar();

    bool processContents();
    bool openInputArchive();
    bool closeInputArchive();
    bool openOutputArchive();
    bool closeOutputArchive();

    void updateProgress(uint64_t bytes, uint64_t maxBytes);
    void updateDetails(const std::string &msg);

    static la_ssize_t laReadCb(archive *a, void *userdata, const void **buffer);
    static la_int64_t laSkipCb(archive *a, void *userdata, la_int64_t request);
    static int laOpenCb(archive *a, void *userdata);
    static int laCloseCb(archive *a, void *userdata);
};
/*! \endcond */


const std::string OdinPatcher::Id("OdinPatcher");


OdinPatcher::OdinPatcher(PatcherConfig * const pc)
    : m_impl(new Impl())
{
    m_impl->pc = pc;
}

OdinPatcher::~OdinPatcher()
{
}

ErrorCode OdinPatcher::error() const
{
    return m_impl->error;
}

std::string OdinPatcher::id() const
{
    return Id;
}

void OdinPatcher::setFileInfo(const FileInfo * const info)
{
    m_impl->info = info;
}

void OdinPatcher::cancelPatching()
{
    m_impl->cancelled = true;
}

bool OdinPatcher::patchFile(ProgressUpdatedCallback progressCb,
                            FilesUpdatedCallback filesCb,
                            DetailsUpdatedCallback detailsCb,
                            void *userData)
{
    (void) filesCb;

    m_impl->cancelled = false;

    assert(m_impl->info != nullptr);

    m_impl->progressCb = progressCb;
    m_impl->detailsCb = detailsCb;
    m_impl->userData = userData;

    m_impl->oldBytes = 0;
    m_impl->bytes = 0;
    m_impl->maxBytes = 0;

    bool ret = m_impl->patchTar();

    m_impl->progressCb = nullptr;
    m_impl->detailsCb = nullptr;
    m_impl->userData = nullptr;

    if (m_impl->aInput != nullptr) {
        m_impl->closeInputArchive();
    }
    if (m_impl->zOutput != nullptr) {
        m_impl->closeOutputArchive();
    }

    if (m_impl->cancelled) {
        m_impl->error = ErrorCode::PatchingCancelled;
        return false;
    }

    return ret;
}

#ifdef __ANDROID__
static bool convertToInt(const char *str, int *out)
{
    char *end;
    errno = 0;
    long num = strtol(str, &end, 10);
    if (errno == ERANGE || num < INT_MIN || num > INT_MAX
            || *str == '\0' || *end != '\0') {
        return false;
    }
    *out = (int) num;
    return true;
}
#endif

bool OdinPatcher::Impl::patchTar()
{
#ifdef __ANDROID__
    static const char *prefix = "/proc/self/fd/";
    fd = -1;
    if (StringUtils::starts_with(info->inputPath(), prefix)) {
        std::string fdStr = info->inputPath().substr(strlen(prefix));
        if (!convertToInt(fdStr.c_str(), &fd)) {
            LOGE("Invalid fd: %s", fdStr.c_str());
            error = ErrorCode::FileOpenError;
            return false;
        }
        LOGD("Input path '%s' is a file descriptor: %d",
             info->inputPath().c_str(), fd);
    }
#endif

    // Get file size for progress information
#ifdef __ANDROID__
    if (fd > 0) {
        off64_t offset = lseek64(fd, 0, SEEK_END);
        if (offset < 0 || lseek64(fd, 0, SEEK_SET) < 0) {
            LOGE("%s: Failed to seek fd: %s", info->inputPath().c_str(),
                 strerror(errno));
            error = ErrorCode::FileSeekError;
            return false;
        }
        maxBytes = offset;
    } else {
#endif

    io::File file;
    if (!file.open(info->inputPath(), io::File::OpenRead)) {
        LOGE("%s: Failed to open: %s", info->inputPath().c_str(),
             file.errorString().c_str());
        error = ErrorCode::FileOpenError;
        return false;
    }
    if (!file.seek(0, io::File::SeekEnd)) {
        LOGE("%s: Failed to seek: %s", info->inputPath().c_str(),
             file.errorString().c_str());
        error = ErrorCode::FileReadError;
        return false;
    }
    if (!file.tell(&maxBytes)) {
        LOGE("%s: Failed to get position: %s", info->inputPath().c_str(),
             file.errorString().c_str());
        error = ErrorCode::FileReadError;
        return false;
    }
    file.close();

#ifdef __ANDROID__
    }
#endif

    updateProgress(bytes, maxBytes);

    if (!openInputArchive()) {
        return false;
    }
    if (!openOutputArchive()) {
        return false;
    }

    if (cancelled) return false;

    if (!processContents()) {
        return false;
    }

    zipFile zf = MinizipUtils::ctxGetZipFile(zOutput);

    ErrorCode result;

    if (cancelled) return false;

    updateDetails("META-INF/com/google/android/update-binary.orig");

    // Add odinupdater
    result = MinizipUtils::addFile(
            zf, "META-INF/com/google/android/update-binary.orig",
            pc->dataDirectory() + "/binaries/android/"
                    + info->device()->architecture() + "/odinupdater");
    if (result != ErrorCode::NoError) {
        error = result;
        return false;
    }

    if (cancelled) return false;

    updateDetails("META-INF/com/google/android/update-binary.orig.sig");

    // Add odinupdater.sig
    result = MinizipUtils::addFile(
            zf, "META-INF/com/google/android/update-binary.orig",
            pc->dataDirectory() + "/binaries/android/"
                    + info->device()->architecture() + "/odinupdater.sig");
    if (result != ErrorCode::NoError) {
        error = result;
        return false;
    }

    if (cancelled) return false;

    updateDetails("fuse-sparse");

    // Add fuse-sparse
    result = MinizipUtils::addFile(
            zf, "fuse-sparse",
            pc->dataDirectory() + "/binaries/android/"
                    + info->device()->architecture() + "/fuse-sparse");
    if (result != ErrorCode::NoError) {
        error = result;
        return false;
    }

    if (cancelled) return false;

    updateDetails("META-INF/com/google/android/update-binary");

    // Add mbtool_recovery
    result = MinizipUtils::addFile(
            zf, "META-INF/com/google/android/update-binary",
            pc->dataDirectory() + "/binaries/android/"
                    + info->device()->architecture() + "/mbtool_recovery");
    if (result != ErrorCode::NoError) {
        error = result;
        return false;
    }

    if (cancelled) return false;

    updateDetails("META-INF/com/google/android/update-binary.sig");

    // Add mbtool_recovery.sig
    result = MinizipUtils::addFile(
            zf, "META-INF/com/google/android/update-binary",
            pc->dataDirectory() + "/binaries/android/"
                    + info->device()->architecture() + "/mbtool_recovery.sig");
    if (result != ErrorCode::NoError) {
        error = result;
        return false;
    }

    if (cancelled) return false;

    updateDetails("multiboot/bb-wrapper.sh");

    // Add bb-wrapper.sh
    result = MinizipUtils::addFile(
            zf, "multiboot/bb-wrapper.sh",
            pc->dataDirectory() + "/scripts/bb-wrapper.sh");
    if (result != ErrorCode::NoError) {
        error = result;
        return false;
    }

    if (cancelled) return false;

    updateDetails("multiboot/info.prop");

    const std::string infoProp =
            MultiBootPatcher::createInfoProp(pc, info->device(), info->romId());
    result = MinizipUtils::addFile(
            zf, "multiboot/info.prop",
            std::vector<unsigned char>(infoProp.begin(), infoProp.end()));
    if (result != ErrorCode::NoError) {
        error = result;
        return false;
    }

    if (cancelled) return false;

    updateDetails("block_devs.prop");

    std::string blockDevsProp;
    blockDevsProp += "system=";
    blockDevsProp += info->device()->systemBlockDevs()[0];
    blockDevsProp += "\n";
    blockDevsProp += "boot=";
    blockDevsProp += info->device()->bootBlockDevs()[0];
    blockDevsProp += "\n";
    result = MinizipUtils::addFile(
            zf, "block_devs.prop", std::vector<unsigned char>(
                    blockDevsProp.begin(), blockDevsProp.end()));
    if (result != ErrorCode::NoError) {
        error = result;
        return false;
    }

    if (cancelled) return false;

    return true;
}

bool OdinPatcher::Impl::processContents()
{
    archive_entry *entry;
    int laRet;
    int mzRet;

    zipFile zf = MinizipUtils::ctxGetZipFile(zOutput);

    while ((laRet = archive_read_next_header(aInput, &entry)) == ARCHIVE_OK) {
        if (cancelled) return false;

        const char *name = archive_entry_pathname(entry);
        if (!name) {
            continue;
        }

        updateDetails(name);

        if (strcmp(name, "boot.img") == 0) {
            LOGD("Handling boot.img");

            // Boot images should never be over about 30 MiB. This check is here
            // so the patcher won't try to read a multi-gigabyte system image
            // into RAM
            la_int64_t size = archive_entry_size(entry);
            if (size > 30 * 1024 * 1024) {
                LOGE("Boot image exceeds 30 MiB: %" PRId64, size);
                error = ErrorCode::BootImageTooLargeError;
                return false;
            }

            std::vector<unsigned char> data;
            if (size > 0) {
                data.reserve(size);
            }
            char buf[10240];
            la_ssize_t n;
            while ((n = archive_read_data(aInput, buf, sizeof(buf))) > 0) {
                data.insert(data.end(), buf, buf + n);
            }
            if (n != 0) {
                LOGE("libarchive: Failed to read data: %s",
                     archive_error_string(aInput));
                error = ErrorCode::ArchiveReadDataError;
                return false;
            }

            if (!MultiBootPatcher::patchBootImage(pc, info, &data, &error)) {
                return false;
            }

            auto errorRet = MinizipUtils::addFile(zf, name, data);
            if (errorRet != ErrorCode::NoError) {
                error = errorRet;
                return false;
            }

            continue;
        } else if (!StringUtils::starts_with(name, "cache.img")
                && !StringUtils::starts_with(name, "system.img")) {
            LOGD("Skipping %s", name);
            if (archive_read_data_skip(aInput) != ARCHIVE_OK) {
                LOGE("libarchive: Failed to skip data: %s",
                     archive_error_string(aInput));
                error = ErrorCode::ArchiveReadDataError;
                return false;
            }
            continue;
        }

        LOGD("Handling %s", name);

        std::string zipName(name);
        if (StringUtils::ends_with(name, ".ext4")) {
            zipName.erase(zipName.size() - 5);
        }
        zipName += ".sparse";

        // Ha! I'll be impressed if a Samsung firmware image does NOT need zip64
        int zip64 = archive_entry_size(entry) > ((1ll << 32) - 1);

        zip_fileinfo zi;
        memset(&zi, 0, sizeof(zi));

        // Open file in output zip
        mzRet = zipOpenNewFileInZip2_64(
            zf,                    // file
            zipName.c_str(),       // filename
            &zi,                   // zip_fileinfo
            nullptr,               // extrafield_local
            0,                     // size_extrafield_local
            nullptr,               // extrafield_global
            0,                     // size_extrafield_global
            nullptr,               // comment
            Z_DEFLATED,            // method
            Z_DEFAULT_COMPRESSION, // level
            0,                     // raw
            zip64                  // zip64
        );
        if (mzRet != ZIP_OK) {
            LOGE("minizip: Failed to open new file in output zip: %s",
                 MinizipUtils::zipErrorString(mzRet).c_str());
            error = ErrorCode::ArchiveWriteHeaderError;
            return false;
        }

        la_ssize_t nRead;
        char buf[10240];
        while ((nRead = archive_read_data(aInput, buf, sizeof(buf))) > 0) {
            if (cancelled) return false;

            mzRet = zipWriteInFileInZip(zf, buf, nRead);
            if (mzRet != ZIP_OK) {
                LOGE("minizip: Failed to write %s in output zip: %s",
                     zipName.c_str(),
                     MinizipUtils::zipErrorString(mzRet).c_str());
                error = ErrorCode::ArchiveWriteDataError;
                zipCloseFileInZip(zf);
                return false;
            }
        }
        if (nRead != 0) {
            LOGE("libarchive: Failed to read %s: %s",
                 name, archive_error_string(aInput));
            error = ErrorCode::ArchiveReadDataError;
            zipCloseFileInZip(zf);
            return false;
        }

        // Close file in output zip
        mzRet = zipCloseFileInZip(zf);
        if (mzRet != ZIP_OK) {
            LOGE("minizip: Failed to close file in output zip: %s",
                 MinizipUtils::zipErrorString(mzRet).c_str());
            error = ErrorCode::ArchiveWriteDataError;
            return false;
        }
    }

    if (laRet != ARCHIVE_EOF) {
        LOGE("libarchive: Failed to read header: %s",
             archive_error_string(aInput));
        error = ErrorCode::ArchiveReadHeaderError;
        return false;
    }

    if (cancelled) return false;

    return true;
}

bool OdinPatcher::Impl::openInputArchive()
{
    assert(aInput == nullptr);

    aInput = archive_read_new();

    // TODO: Eventually support tar within a zip since many people distribute
    //       stock firmware packages in this format
    //archive_read_support_format_zip(aInput);
    archive_read_support_format_tar(aInput);
    archive_read_support_filter_gzip(aInput);
    archive_read_support_filter_xz(aInput);

    // Our callbacks use io::File, which supports LFS on Android. Also allows
    // progress info by counting number of bytes read.
    int ret = archive_read_open2(aInput, this, &Impl::laOpenCb, &Impl::laReadCb,
                                 &Impl::laSkipCb, &Impl::laCloseCb);
    if (ret != ARCHIVE_OK) {
        LOGW("libarchive: Failed to open for reading: %s",
             archive_error_string(aInput));
        archive_read_free(aInput);
        aInput = nullptr;
        error = ErrorCode::ArchiveReadOpenError;
        return false;
    }

    return true;
}

bool OdinPatcher::Impl::closeInputArchive()
{
    assert(aInput != nullptr);

    bool ret = true;

    if (archive_read_close(aInput) != ARCHIVE_OK) {
        LOGW("libarchive: Failed to close archive: %s",
             archive_error_string(aInput));
        // Don't clobber previous error
        //error = ErrorCode::ArchiveCloseError;
        ret = false;
    }
    archive_read_free(aInput);
    aInput = nullptr;

    return ret;
}

bool OdinPatcher::Impl::openOutputArchive()
{
    assert(zOutput == nullptr);

    zOutput = MinizipUtils::openOutputFile(info->outputPath());

    if (!zOutput) {
        LOGE("minizip: Failed to open for writing: %s",
             info->outputPath().c_str());
        error = ErrorCode::ArchiveWriteOpenError;
        return false;
    }

    return true;
}

bool OdinPatcher::Impl::closeOutputArchive()
{
    assert(zOutput != nullptr);

    int ret = MinizipUtils::closeOutputFile(zOutput);
    if (ret != ZIP_OK) {
        LOGW("minizip: Failed to close archive: %s",
             MinizipUtils::zipErrorString(ret).c_str());
        // Don't clobber previous error
        //error = ErrorCode::ArchiveCloseError;
        return false;
    }
    zOutput = nullptr;

    return true;
}

void OdinPatcher::Impl::updateProgress(uint64_t bytes, uint64_t maxBytes)
{
    if (progressCb) {
        bool shouldCall = true;
        if (maxBytes > 0) {
            // Rate limit... call back only if percentage exceeds 0.01%
            double oldRatio = (double) oldBytes / maxBytes;
            double newRatio = (double) bytes / maxBytes;
            if (newRatio - oldRatio < 0.0001) {
                shouldCall = false;
            }
        }
        if (shouldCall) {
            progressCb(bytes, maxBytes, userData);
            oldBytes = bytes;
        }
    }
}

void OdinPatcher::Impl::updateDetails(const std::string &msg)
{
    if (detailsCb) {
        detailsCb(msg, userData);
    }
}

la_ssize_t OdinPatcher::Impl::laReadCb(archive *a, void *userdata,
                                       const void **buffer)
{
    (void) a;
    Impl *impl = static_cast<Impl *>(userdata);
    *buffer = impl->laBuf;
    uint64_t bytesRead;
    bool ret = true;

#ifdef __ANDROID__
    if (impl->fd >= 0) {
        ssize_t n = read(impl->fd, impl->laBuf, sizeof(impl->laBuf));
        if (n < 0) {
            LOGE("%s: Failed to read: %s", impl->info->inputPath().c_str(),
                 strerror(errno));
            ret = false;
        } else {
            bytesRead = n;
        }
    } else {
#endif
        ret = impl->laFile.read(impl->laBuf, sizeof(impl->laBuf), &bytesRead);
        if (!ret && impl->laFile.error() == io::File::ErrorEndOfFile) {
            ret = true;
        }
        if (!ret) {
            LOGE("%s: Failed to read: %s", impl->info->inputPath().c_str(),
                 impl->laFile.errorString().c_str());
        }
#ifdef __ANDROID__
    }
#endif

    if (ret) {
        impl->bytes += bytesRead;
        impl->updateProgress(impl->bytes, impl->maxBytes);
    } else {
        impl->error = ErrorCode::FileReadError;
    }

    return ret ? (la_ssize_t) bytesRead : -1;
}

la_int64_t OdinPatcher::Impl::laSkipCb(archive *a, void *userdata,
                                       la_int64_t request)
{
    (void) a;
    Impl *impl = static_cast<Impl *>(userdata);
    bool ret = true;

#ifdef __ANDROID__
    if (impl->fd >= 0) {
        if (lseek64(impl->fd, request, SEEK_CUR) < 0) {
            LOGE("%s: Failed to seek: %s", impl->info->inputPath().c_str(),
                 strerror(errno));
            ret = false;
        }
    } else {
#endif
        if (!impl->laFile.seek(request, io::File::SeekCurrent)) {
            LOGE("%s: Failed to seek: %s", impl->info->inputPath().c_str(),
                impl->laFile.errorString().c_str());
            ret = false;
        }
#ifdef __ANDROID__
    }
#endif

    if (ret) {
        impl->bytes += request;
        impl->updateProgress(impl->bytes, impl->maxBytes);
    } else {
        impl->error = ErrorCode::FileSeekError;
    }

    return ret ? request : -1;
}

int OdinPatcher::Impl::laOpenCb(archive *a, void *userdata)
{
    (void) a;
    Impl *impl = static_cast<Impl *>(userdata);

#ifdef __ANDROID__
    if (impl->fd >= 0) {
        return 0;
    }
#endif

    bool ret = impl->laFile.open(impl->info->inputPath(), io::File::OpenRead);
    if (!ret) {
        LOGE("%s: Failed to open: %s", impl->info->inputPath().c_str(),
             impl->laFile.errorString().c_str());
        impl->error = ErrorCode::FileOpenError;
    }
    return ret ? 0 : -1;
}

int OdinPatcher::Impl::laCloseCb(archive *a, void *userdata)
{
    (void) a;
    Impl *impl = static_cast<Impl *>(userdata);

#ifdef __ANDROID__
    if (impl->fd >= 0) {
        return 0;
    }
#endif

    bool ret = impl->laFile.close();
    if (!ret) {
        LOGE("%s: Failed to close: %s", impl->info->inputPath().c_str(),
             impl->laFile.errorString().c_str());
        impl->error = ErrorCode::FileCloseError;
    }
    return ret ? 0 : -1;
}

}
