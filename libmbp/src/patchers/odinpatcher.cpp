/*
 * Copyright (C) 2015-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/file/fd.h"
#include "mbcommon/file/filename.h"
#include "mbcommon/locale.h"
#include "mbcommon/string.h"

#include "mbdevice/json.h"

#include "mblog/logging.h"

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

typedef std::unique_ptr<MbFile, decltype(mb_file_free) *> ScopedMbFile;

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
    ScopedMbFile laFile{mb_file_new(), &mb_file_free};
#ifdef __ANDROID__
    int fd = -1;
#endif

    std::unordered_set<std::string> added_files;

    // Callbacks
    ProgressUpdatedCallback progressCb;
    DetailsUpdatedCallback detailsCb;
    void *userData;

    // Patching
    archive *aInput = nullptr;
    MinizipUtils::ZipCtx *zOutput = nullptr;

    bool patchTar();

    bool processFile(archive *a, archive_entry *entry, bool sparse);
    bool processContents(archive *a, int depth);
    bool openInputArchive();
    bool closeInputArchive();
    bool openOutputArchive();
    bool closeOutputArchive();

    void updateProgress(uint64_t bytes, uint64_t maxBytes);
    void updateDetails(const std::string &msg);

    static la_ssize_t laNestedReadCb(archive *a, void *userdata, const void **buffer);

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

struct CopySpec {
    std::string source;
    std::string target;
};

bool OdinPatcher::Impl::patchTar()
{
#ifdef __ANDROID__
    static const char *prefix = "/proc/self/fd/";
    fd = -1;
    if (mb_starts_with(info->inputPath().c_str(), prefix)) {
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

    updateProgress(bytes, maxBytes);

    if (!openInputArchive()) {
        return false;
    }
    if (!openOutputArchive()) {
        return false;
    }

    if (cancelled) return false;

    // Get file size and seek back to original location
    uint64_t currentPos;
    if (mb_file_seek(laFile.get(), 0, SEEK_CUR, &currentPos) != MB_FILE_OK
            || mb_file_seek(laFile.get(), 0, SEEK_END, &maxBytes) != MB_FILE_OK
            || mb_file_seek(laFile.get(), currentPos, SEEK_SET, nullptr)
                    != MB_FILE_OK) {
        LOGE("%s: Failed to seek: %s", info->inputPath().c_str(),
             mb_file_error_string(laFile.get()));
        error = ErrorCode::FileSeekError;
        return false;
    }

    if (cancelled) return false;

    if (!processContents(aInput, 0)) {
        return false;
    }

    std::string archDir(pc->dataDirectory());
    archDir += "/binaries/android/";
    archDir += mb_device_architecture(info->device());

    std::vector<CopySpec> toCopy{
        {
            archDir + "/odinupdater",
            "META-INF/com/google/android/update-binary.orig"
        }, {
            archDir + "/odinupdater.sig",
            "META-INF/com/google/android/update-binary.orig.sig"
        }, {
            archDir + "/fuse-sparse",
            "fuse-sparse"
        }, {
            archDir + "/fuse-sparse.sig",
            "fuse-sparse.sig"
        }, {
            archDir + "/mbtool_recovery",
            "META-INF/com/google/android/update-binary"
        }, {
            archDir + "/mbtool_recovery.sig",
            "META-INF/com/google/android/update-binary.sig"
        }, {
            pc->dataDirectory() + "/scripts/bb-wrapper.sh",
            "multiboot/bb-wrapper.sh"
        }, {
            pc->dataDirectory() + "/scripts/bb-wrapper.sh.sig",
            "multiboot/bb-wrapper.sh.sig"
        }
    };

    std::vector<std::string> binaries{
        "file-contexts-tool",
        "file-contexts-tool.sig",
        "fsck-wrapper",
        "fsck-wrapper.sig",
        "mbtool",
        "mbtool.sig",
        "mount.exfat",
        "mount.exfat.sig",
    };

    for (auto const &binary : binaries) {
        toCopy.push_back({archDir + "/" + binary,
                          "multiboot/binaries/" + binary});
    }

    zipFile zf = MinizipUtils::ctxGetZipFile(zOutput);

    ErrorCode result;

    for (const CopySpec &spec : toCopy) {
        if (cancelled) return false;

        updateDetails(spec.target);

        result = MinizipUtils::addFile(zf, spec.target, spec.source);
        if (result != ErrorCode::NoError) {
            error = result;
            return false;
        }
    }

    if (cancelled) return false;

    updateDetails("multiboot/info.prop");

    const std::string infoProp =
            MultiBootPatcher::createInfoProp(pc, info->romId(), false);
    result = MinizipUtils::addFile(
            zf, "multiboot/info.prop",
            std::vector<unsigned char>(infoProp.begin(), infoProp.end()));
    if (result != ErrorCode::NoError) {
        error = result;
        return false;
    }

    if (cancelled) return false;

    updateDetails("multiboot/device.json");

    char *json = mb_device_to_json(info->device());
    if (!json) {
        error = ErrorCode::MemoryAllocationError;
        return false;
    }

    result = MinizipUtils::addFile(
            zf, "multiboot/device.json",
            std::vector<unsigned char>(json, json + strlen(json)));
    free(json);

    if (result != ErrorCode::NoError) {
        error = result;
        return false;
    }

    if (cancelled) return false;

    return true;
}

bool OdinPatcher::Impl::processFile(archive *a, archive_entry *entry,
                                    bool sparse)
{
    const char *name = archive_entry_pathname(entry);
    std::string zipName(name);

    if (sparse) {
        if (mb_ends_with(name, ".ext4")) {
            zipName.erase(zipName.size() - 5);
        }
        zipName += ".sparse";
    }

    // Ha! I'll be impressed if a Samsung firmware image does NOT need zip64
    int zip64 = archive_entry_size(entry) > ((1ll << 32) - 1);

    zip_fileinfo zi;
    memset(&zi, 0, sizeof(zi));

    zipFile zf = MinizipUtils::ctxGetZipFile(zOutput);

    // Open file in output zip
    int mzRet = zipOpenNewFileInZip2_64(
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
    while ((nRead = archive_read_data(a, buf, sizeof(buf))) > 0) {
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
             name, archive_error_string(a));
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

    return true;
}

static const char * indent(unsigned int depth)
{
    static char buf[16];
    memset(buf, ' ', sizeof(buf));

    if (depth * 2 < sizeof(buf) - 1) {
        buf[depth * 2] = '\0';
    } else {
        buf[sizeof(buf) - 1] = '\0';
    }

    return buf;
}

struct NestedCtx
{
    archive *nested;
    archive *parent;
    char buf[10240];

    NestedCtx(archive *a) : nested(archive_read_new()), parent(a)
    {
    }

    ~NestedCtx()
    {
        if (nested) {
            archive_read_free(nested);
        }
    }
};

bool OdinPatcher::Impl::processContents(archive *a, int depth)
{
    if (depth > 1) {
        LOGW("Not traversing nested archive: depth > 1");
        return true;
    }

    archive_entry *entry;
    int laRet;

    while ((laRet = archive_read_next_header(a, &entry)) == ARCHIVE_OK) {
        if (cancelled) return false;

        const char *name = archive_entry_pathname(entry);
        if (!name) {
            continue;
        }

        updateDetails(name);

        // Certain files may be duplicated. For example, the cache.img file is
        // shipped on both the CSC and HOME_CSC tarballs.
        if (added_files.find(name) != added_files.end()) {
            LOGV("%sSkipping duplicate file: %s", indent(depth), name);
            continue;
        }

        if (strcmp(name, "boot.img") == 0) {
            LOGV("%sHandling boot image: %s", indent(depth), name);
            added_files.insert(name);

            if (!processFile(a, entry, false)) {
                return false;
            }
        } else if (mb_starts_with(name, "cache.img")
                || mb_starts_with(name, "system.img")) {
            LOGV("%sHandling sparse image: %s", indent(depth), name);
            added_files.insert(name);

            if (!processFile(a, entry, true)) {
                return false;
            }
        } else if (mb_ends_with(name, ".tar.md5")) {
            LOGV("%sHandling nested tarball: %s", indent(depth), name);

            NestedCtx ctx(a);
            if (!ctx.nested) {
                error = ErrorCode::MemoryAllocationError;
                return false;
            }

            archive_read_support_format_tar(ctx.nested);

            int ret = archive_read_open2(ctx.nested, &ctx, nullptr,
                                         &laNestedReadCb, nullptr, nullptr);
            if (ret != ARCHIVE_OK) {
                LOGE("libarchive: Failed to open nested archive: %s: %s",
                     name, archive_error_string(ctx.nested));
                error = ErrorCode::ArchiveReadOpenError;
                return false;
            }

            if (!processContents(ctx.nested, depth + 1)) {
                return false;
            }
        } else {
            LOGD("%sSkipping unneeded file: %s", indent(depth), name);

            if (archive_read_data_skip(a) != ARCHIVE_OK) {
                LOGE("libarchive: Failed to skip data: %s",
                     archive_error_string(a));
                error = ErrorCode::ArchiveReadDataError;
                return false;
            }
        }
    }

    if (laRet != ARCHIVE_EOF) {
        LOGE("libarchive: Failed to read header: %s",
             archive_error_string(a));
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

    archive_read_support_format_zip(aInput);
    archive_read_support_format_tar(aInput);
    archive_read_support_filter_gzip(aInput);
    archive_read_support_filter_xz(aInput);

    // Our callbacks use the MbFile API, which supports LFS on every platform.
    // Also allows progress info by counting number of bytes read.
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

la_ssize_t OdinPatcher::Impl::laNestedReadCb(archive *a, void *userdata,
                                             const void **buffer)
{
    (void) a;

    NestedCtx *ctx = static_cast<NestedCtx *>(userdata);

    *buffer = ctx->buf;

    return archive_read_data(ctx->parent, ctx->buf, sizeof(ctx->buf));
}

la_ssize_t OdinPatcher::Impl::laReadCb(archive *a, void *userdata,
                                       const void **buffer)
{
    (void) a;
    Impl *impl = static_cast<Impl *>(userdata);
    *buffer = impl->laBuf;
    size_t bytesRead;

    if (mb_file_read(impl->laFile.get(), impl->laBuf, sizeof(impl->laBuf),
                     &bytesRead) != MB_FILE_OK) {
        LOGE("%s: Failed to read: %s", impl->info->inputPath().c_str(),
             mb_file_error_string(impl->laFile.get()));
        impl->error = ErrorCode::FileReadError;
        return -1;
    }

    impl->bytes += bytesRead;
    impl->updateProgress(impl->bytes, impl->maxBytes);
    return static_cast<la_ssize_t>(bytesRead);
}

la_int64_t OdinPatcher::Impl::laSkipCb(archive *a, void *userdata,
                                       la_int64_t request)
{
    (void) a;
    Impl *impl = static_cast<Impl *>(userdata);

    if (mb_file_seek(impl->laFile.get(), request, SEEK_CUR, nullptr)
            != MB_FILE_OK) {
        LOGE("%s: Failed to seek: %s", impl->info->inputPath().c_str(),
             mb_file_error_string(impl->laFile.get()));
        impl->error = ErrorCode::FileSeekError;
        return -1;
    }

    impl->bytes += request;
    impl->updateProgress(impl->bytes, impl->maxBytes);
    return request;
}

int OdinPatcher::Impl::laOpenCb(archive *a, void *userdata)
{
    (void) a;
    Impl *impl = static_cast<Impl *>(userdata);
    int ret;

#ifdef _WIN32
    std::unique_ptr<wchar_t, decltype(free) *> wFilename{
            mb::utf8_to_wcs(impl->info->inputPath().c_str()), free};
    if (!wFilename) {
        LOGE("%s: Failed to convert from UTF8 to WCS",
             impl->info->inputPath().c_str());
        impl->error = ErrorCode::FileOpenError;
        return -1;
    }

    ret = mb_file_open_filename_w(impl->laFile.get(), wFilename.get(),
                                  MB_FILE_OPEN_READ_ONLY);
#else
#  ifdef __ANDROID__
    if (impl->fd >= 0) {
        ret = mb_file_open_fd(impl->laFile.get(), impl->fd, false);
    } else
#  endif
    ret = mb_file_open_filename(impl->laFile.get(),
                                impl->info->inputPath().c_str(),
                                MB_FILE_OPEN_READ_ONLY);
#endif

    if (ret != MB_FILE_OK) {
        LOGE("%s: Failed to open: %s", impl->info->inputPath().c_str(),
             mb_file_error_string(impl->laFile.get()));
        impl->error = ErrorCode::FileOpenError;
        return -1;
    }

    return 0;
}

int OdinPatcher::Impl::laCloseCb(archive *a, void *userdata)
{
    (void) a;
    Impl *impl = static_cast<Impl *>(userdata);

    int ret = mb_file_close(impl->laFile.get());
    if (ret != MB_FILE_OK) {
        LOGE("%s: Failed to close: %s", impl->info->inputPath().c_str(),
             mb_file_error_string(impl->laFile.get()));
        impl->error = ErrorCode::FileCloseError;
        return -1;
    }

    return 0;
}

}
