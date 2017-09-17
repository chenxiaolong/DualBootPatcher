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

#include "mbpatcher/patchers/odinpatcher.h"

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

#include "mbcommon/locale.h"
#include "mbcommon/string.h"

#include "mbdevice/json.h"

#include "mblog/logging.h"

#include "mbpatcher/patcherconfig.h"
#include "mbpatcher/patchers/zippatcher.h"
#include "mbpatcher/private/fileutils.h"
#include "mbpatcher/private/miniziputils.h"
#include "mbpatcher/private/stringutils.h"

#if defined(__ANDROID__)
#  include "mbcommon/file/fd.h"
#else
#  include "mbcommon/file/standard.h"
#endif

// minizip
#include "minizip/zip.h"

#define LOG_TAG "mbpatcher/patchers/odinpatcher"

class ar;

namespace mb
{
namespace patcher
{

/*! \cond INTERNAL */
class OdinPatcherPrivate
{
public:
    PatcherConfig *pc;
    const FileInfo *info;

    uint64_t old_bytes;
    uint64_t bytes;
    uint64_t max_bytes;

    volatile bool cancelled;

    ErrorCode error;

    unsigned char la_buf[10240];
#ifdef __ANDROID__
    FdFile la_file;
    int fd = -1;
#else
    StandardFile la_file;
#endif

    std::unordered_set<std::string> added_files;

    // Callbacks
    OdinPatcher::ProgressUpdatedCallback progress_cb;
    OdinPatcher::DetailsUpdatedCallback details_cb;
    void *userdata;

    // Patching
    archive *a_input = nullptr;
    MinizipUtils::ZipCtx *z_output = nullptr;

    bool patch_tar();

    bool process_file(archive *a, archive_entry *entry, bool sparse);
    bool process_contents(archive *a, unsigned int depth);
    bool open_input_archive();
    bool close_input_archive();
    bool open_output_archive();
    bool close_output_archive();

    void update_progress(uint64_t bytes, uint64_t max_bytes);
    void update_details(const std::string &msg);

    static la_ssize_t la_nested_read_cb(archive *a, void *userdata,
                                        const void **buffer);

    static la_ssize_t la_read_cb(archive *a, void *userdata,
                                 const void **buffer);
    static la_int64_t la_skip_cb(archive *a, void *userdata,
                                 la_int64_t request);
    static int la_open_cb(archive *a, void *userdata);
    static int la_close_cb(archive *a, void *userdata);
};
/*! \endcond */


const std::string OdinPatcher::Id("OdinPatcher");


OdinPatcher::OdinPatcher(PatcherConfig * const pc)
    : _priv_ptr(new OdinPatcherPrivate())
{
    MB_PRIVATE(OdinPatcher);
    priv->pc = pc;
}

OdinPatcher::~OdinPatcher()
{
}

ErrorCode OdinPatcher::error() const
{
    MB_PRIVATE(const OdinPatcher);
    return priv->error;
}

std::string OdinPatcher::id() const
{
    return Id;
}

void OdinPatcher::set_file_info(const FileInfo * const info)
{
    MB_PRIVATE(OdinPatcher);
    priv->info = info;
}

void OdinPatcher::cancel_patching()
{
    MB_PRIVATE(OdinPatcher);
    priv->cancelled = true;
}

bool OdinPatcher::patch_file(ProgressUpdatedCallback progress_cb,
                             FilesUpdatedCallback files_cb,
                             DetailsUpdatedCallback details_cb,
                             void *userdata)
{
    (void) files_cb;

    MB_PRIVATE(OdinPatcher);

    priv->cancelled = false;

    assert(priv->info != nullptr);

    priv->progress_cb = progress_cb;
    priv->details_cb = details_cb;
    priv->userdata = userdata;

    priv->old_bytes = 0;
    priv->bytes = 0;
    priv->max_bytes = 0;

    bool ret = priv->patch_tar();

    priv->progress_cb = nullptr;
    priv->details_cb = nullptr;
    priv->userdata = nullptr;

    if (priv->a_input != nullptr) {
        priv->close_input_archive();
    }
    if (priv->z_output != nullptr) {
        priv->close_output_archive();
    }

    if (priv->cancelled) {
        priv->error = ErrorCode::PatchingCancelled;
        return false;
    }

    return ret;
}

#ifdef __ANDROID__
static bool convert_to_int(const char *str, int *out)
{
    char *end;
    errno = 0;
    long num = strtol(str, &end, 10);
    if (errno == ERANGE || num < INT_MIN || num > INT_MAX
            || *str == '\0' || *end != '\0') {
        return false;
    }
    *out = static_cast<int>(num);
    return true;
}
#endif

struct CopySpec
{
    std::string source;
    std::string target;
};

bool OdinPatcherPrivate::patch_tar()
{
#ifdef __ANDROID__
    static const char *prefix = "/proc/self/fd/";
    fd = -1;
    if (starts_with(info->input_path(), prefix)) {
        std::string fd_str = info->input_path().substr(strlen(prefix));
        if (!convert_to_int(fd_str.c_str(), &fd)) {
            LOGE("Invalid fd: %s", fd_str.c_str());
            error = ErrorCode::FileOpenError;
            return false;
        }
        LOGD("Input path '%s' is a file descriptor: %d",
             info->input_path().c_str(), fd);
    }
#endif

    update_progress(bytes, max_bytes);

    if (!open_input_archive()) {
        return false;
    }
    if (!open_output_archive()) {
        return false;
    }

    if (cancelled) return false;

    // Get file size and seek back to original location
    uint64_t current_pos;
    if (!la_file.seek(0, SEEK_CUR, &current_pos)
            || !la_file.seek(0, SEEK_END, &max_bytes)
            || !la_file.seek(static_cast<int64_t>(current_pos), SEEK_SET,
                             nullptr)) {
        LOGE("%s: Failed to seek: %s", info->input_path().c_str(),
             la_file.error_string().c_str());
        error = ErrorCode::FileSeekError;
        return false;
    }

    if (cancelled) return false;

    if (!process_contents(a_input, 0)) {
        return false;
    }

    std::string arch_dir(pc->data_directory());
    arch_dir += "/binaries/android/";
    arch_dir += info->device().architecture();

    std::vector<CopySpec> to_copy {
        {
            arch_dir + "/odinupdater",
            "META-INF/com/google/android/update-binary.orig"
        }, {
            arch_dir + "/odinupdater.sig",
            "META-INF/com/google/android/update-binary.orig.sig"
        }, {
            arch_dir + "/fuse-sparse",
            "fuse-sparse"
        }, {
            arch_dir + "/fuse-sparse.sig",
            "fuse-sparse.sig"
        }, {
            arch_dir + "/mbtool_recovery",
            "META-INF/com/google/android/update-binary"
        }, {
            arch_dir + "/mbtool_recovery.sig",
            "META-INF/com/google/android/update-binary.sig"
        }, {
            pc->data_directory() + "/scripts/bb-wrapper.sh",
            "multiboot/bb-wrapper.sh"
        }, {
            pc->data_directory() + "/scripts/bb-wrapper.sh.sig",
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
        to_copy.push_back({arch_dir + "/" + binary,
                          "multiboot/binaries/" + binary});
    }

    zipFile zf = MinizipUtils::ctx_get_zip_file(z_output);

    ErrorCode result;

    for (const CopySpec &spec : to_copy) {
        if (cancelled) return false;

        update_details(spec.target);

        result = MinizipUtils::add_file(zf, spec.target, spec.source);
        if (result != ErrorCode::NoError) {
            error = result;
            return false;
        }
    }

    if (cancelled) return false;

    update_details("multiboot/info.prop");

    const std::string info_prop =
            ZipPatcher::create_info_prop(pc, info->rom_id(), false);
    result = MinizipUtils::add_file(
            zf, "multiboot/info.prop",
            std::vector<unsigned char>(info_prop.begin(), info_prop.end()));
    if (result != ErrorCode::NoError) {
        error = result;
        return false;
    }

    if (cancelled) return false;

    update_details("multiboot/device.json");

    std::string json;
    if (!device::device_to_json(info->device(), json)) {
        error = ErrorCode::MemoryAllocationError;
        return false;
    }

    result = MinizipUtils::add_file(
            zf, "multiboot/device.json",
            std::vector<unsigned char>(json.begin(), json.end()));
    if (result != ErrorCode::NoError) {
        error = result;
        return false;
    }

    if (cancelled) return false;

    return true;
}

bool OdinPatcherPrivate::process_file(archive *a, archive_entry *entry,
                                      bool sparse)
{
    const char *name = archive_entry_pathname(entry);
    std::string zip_name(name);

    if (sparse) {
        if (ends_with(name, ".ext4")) {
            zip_name.erase(zip_name.size() - 5);
        }
        zip_name += ".sparse";
    }

    // Ha! I'll be impressed if a Samsung firmware image does NOT need zip64
    int zip64 = archive_entry_size(entry) > ((1ll << 32) - 1);

    zip_fileinfo zi;
    memset(&zi, 0, sizeof(zi));

    zipFile zf = MinizipUtils::ctx_get_zip_file(z_output);

    // Open file in output zip
    int mz_ret = zipOpenNewFileInZip2_64(
        zf,                    // file
        zip_name.c_str(),      // filename
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
    if (mz_ret != ZIP_OK) {
        LOGE("minizip: Failed to open new file in output zip: %s",
             MinizipUtils::zip_error_string(mz_ret).c_str());
        error = ErrorCode::ArchiveWriteHeaderError;
        return false;
    }

    la_ssize_t n_read;
    char buf[10240];
    while ((n_read = archive_read_data(a, buf, sizeof(buf))) > 0) {
        if (cancelled) return false;

        mz_ret = zipWriteInFileInZip(zf, buf, static_cast<uint32_t>(n_read));
        if (mz_ret != ZIP_OK) {
            LOGE("minizip: Failed to write %s in output zip: %s",
                 zip_name.c_str(),
                 MinizipUtils::zip_error_string(mz_ret).c_str());
            error = ErrorCode::ArchiveWriteDataError;
            zipCloseFileInZip(zf);
            return false;
        }
    }

    if (n_read != 0) {
        LOGE("libarchive: Failed to read %s: %s",
             name, archive_error_string(a));
        error = ErrorCode::ArchiveReadDataError;
        zipCloseFileInZip(zf);
        return false;
    }

    // Close file in output zip
    mz_ret = zipCloseFileInZip(zf);
    if (mz_ret != ZIP_OK) {
        LOGE("minizip: Failed to close file in output zip: %s",
             MinizipUtils::zip_error_string(mz_ret).c_str());
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

bool OdinPatcherPrivate::process_contents(archive *a, unsigned int depth)
{
    if (depth > 1) {
        LOGW("Not traversing nested archive: depth > 1");
        return true;
    }

    archive_entry *entry;
    int la_ret;

    while ((la_ret = archive_read_next_header(a, &entry)) == ARCHIVE_OK) {
        if (cancelled) return false;

        const char *name = archive_entry_pathname(entry);
        if (!name) {
            continue;
        }

        update_details(name);

        // Certain files may be duplicated. For example, the cache.img file is
        // shipped on both the CSC and HOME_CSC tarballs.
        if (added_files.find(name) != added_files.end()) {
            LOGV("%sSkipping duplicate file: %s", indent(depth), name);
            continue;
        }

        if (strcmp(name, "boot.img") == 0) {
            LOGV("%sHandling boot image: %s", indent(depth), name);
            added_files.insert(name);

            if (!process_file(a, entry, false)) {
                return false;
            }
        } else if (starts_with(name, "cache.img")
                || starts_with(name, "system.img")) {
            LOGV("%sHandling sparse image: %s", indent(depth), name);
            added_files.insert(name);

            if (!process_file(a, entry, true)) {
                return false;
            }
        } else if (ends_with(name, ".tar.md5") || ends_with(name, ".tar")) {
            LOGV("%sHandling nested tarball: %s", indent(depth), name);

            NestedCtx ctx(a);
            if (!ctx.nested) {
                error = ErrorCode::MemoryAllocationError;
                return false;
            }

            archive_read_support_format_tar(ctx.nested);

            int ret = archive_read_open2(ctx.nested, &ctx, nullptr,
                                         &la_nested_read_cb, nullptr, nullptr);
            if (ret != ARCHIVE_OK) {
                LOGE("libarchive: Failed to open nested archive: %s: %s",
                     name, archive_error_string(ctx.nested));
                error = ErrorCode::ArchiveReadOpenError;
                return false;
            }

            if (!process_contents(ctx.nested, depth + 1)) {
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

    if (la_ret != ARCHIVE_EOF) {
        LOGE("libarchive: Failed to read header: %s",
             archive_error_string(a));
        error = ErrorCode::ArchiveReadHeaderError;
        return false;
    }

    if (cancelled) return false;

    return true;
}

bool OdinPatcherPrivate::open_input_archive()
{
    assert(a_input == nullptr);

    a_input = archive_read_new();

    archive_read_support_format_zip(a_input);
    archive_read_support_format_tar(a_input);
    archive_read_support_filter_gzip(a_input);
    archive_read_support_filter_xz(a_input);

    // Our callbacks use the libmbcommon File API, which supports LFS on every
    // platform. Also allows progress info by counting number of bytes read.
    int ret = archive_read_open2(a_input, this, &la_open_cb, &la_read_cb,
                                 &la_skip_cb, &la_close_cb);
    if (ret != ARCHIVE_OK) {
        LOGW("libarchive: Failed to open for reading: %s",
             archive_error_string(a_input));
        archive_read_free(a_input);
        a_input = nullptr;
        error = ErrorCode::ArchiveReadOpenError;
        return false;
    }

    return true;
}

bool OdinPatcherPrivate::close_input_archive()
{
    assert(a_input != nullptr);

    bool ret = true;

    if (archive_read_close(a_input) != ARCHIVE_OK) {
        LOGW("libarchive: Failed to close archive: %s",
             archive_error_string(a_input));
        // Don't clobber previous error
        //error = ErrorCode::ArchiveCloseError;
        ret = false;
    }
    archive_read_free(a_input);
    a_input = nullptr;

    return ret;
}

bool OdinPatcherPrivate::open_output_archive()
{
    assert(z_output == nullptr);

    z_output = MinizipUtils::open_output_file(info->output_path());

    if (!z_output) {
        LOGE("minizip: Failed to open for writing: %s",
             info->output_path().c_str());
        error = ErrorCode::ArchiveWriteOpenError;
        return false;
    }

    return true;
}

bool OdinPatcherPrivate::close_output_archive()
{
    assert(z_output != nullptr);

    int ret = MinizipUtils::close_output_file(z_output);
    if (ret != ZIP_OK) {
        LOGW("minizip: Failed to close archive: %s",
             MinizipUtils::zip_error_string(ret).c_str());
        // Don't clobber previous error
        //error = ErrorCode::ArchiveCloseError;
        return false;
    }
    z_output = nullptr;

    return true;
}

void OdinPatcherPrivate::update_progress(uint64_t bytes, uint64_t max_bytes)
{
    if (progress_cb) {
        bool should_call = true;
        if (max_bytes > 0) {
            // Rate limit... call back only if percentage exceeds 0.01%
            double old_ratio = static_cast<double>(old_bytes)
                    / static_cast<double>(max_bytes);
            double new_ratio = static_cast<double>(bytes)
                    / static_cast<double>(max_bytes);
            if (new_ratio - old_ratio < 0.0001) {
                should_call = false;
            }
        }
        if (should_call) {
            progress_cb(bytes, max_bytes, userdata);
            old_bytes = bytes;
        }
    }
}

void OdinPatcherPrivate::update_details(const std::string &msg)
{
    if (details_cb) {
        details_cb(msg, userdata);
    }
}

la_ssize_t OdinPatcherPrivate::la_nested_read_cb(archive *a, void *userdata,
                                                 const void **buffer)
{
    (void) a;

    NestedCtx *ctx = static_cast<NestedCtx *>(userdata);

    *buffer = ctx->buf;

    return archive_read_data(ctx->parent, ctx->buf, sizeof(ctx->buf));
}

la_ssize_t OdinPatcherPrivate::la_read_cb(archive *a, void *userdata,
                                          const void **buffer)
{
    (void) a;
    auto *priv = static_cast<OdinPatcherPrivate *>(userdata);
    *buffer = priv->la_buf;
    size_t bytes_read;

    if (!priv->la_file.read(priv->la_buf, sizeof(priv->la_buf), bytes_read)) {
        LOGE("%s: Failed to read: %s", priv->info->input_path().c_str(),
             priv->la_file.error_string().c_str());
        priv->error = ErrorCode::FileReadError;
        return -1;
    }

    priv->bytes += bytes_read;
    priv->update_progress(priv->bytes, priv->max_bytes);
    return static_cast<la_ssize_t>(bytes_read);
}

la_int64_t OdinPatcherPrivate::la_skip_cb(archive *a, void *userdata,
                                          la_int64_t request)
{
    (void) a;
    auto *priv = static_cast<OdinPatcherPrivate *>(userdata);

    if (!priv->la_file.seek(request, SEEK_CUR, nullptr)) {
        LOGE("%s: Failed to seek: %s", priv->info->input_path().c_str(),
             priv->la_file.error_string().c_str());
        priv->error = ErrorCode::FileSeekError;
        return -1;
    }

    priv->bytes = static_cast<uint64_t>(
            static_cast<int64_t>(priv->bytes) + request);
    priv->update_progress(priv->bytes, priv->max_bytes);
    return request;
}

int OdinPatcherPrivate::la_open_cb(archive *a, void *userdata)
{
    (void) a;
    auto *priv = static_cast<OdinPatcherPrivate *>(userdata);
    bool ret;

#ifdef _WIN32
    std::wstring w_filename;
    if (!utf8_to_wcs(w_filename, priv->info->input_path())) {
        LOGE("%s: Failed to convert from UTF8 to WCS",
             priv->info->input_path().c_str());
        priv->error = ErrorCode::FileOpenError;
        return -1;
    }

    ret = priv->la_file.open(w_filename, FileOpenMode::READ_ONLY);
#else
#  ifdef __ANDROID__
    if (priv->fd >= 0) {
        ret = priv->la_file.open(priv->fd, false);
    } else
#  endif
    ret = priv->la_file.open(priv->info->input_path(), FileOpenMode::READ_ONLY);
#endif

    if (!ret) {
        LOGE("%s: Failed to open: %s", priv->info->input_path().c_str(),
             priv->la_file.error_string().c_str());
        priv->error = ErrorCode::FileOpenError;
        return -1;
    }

    return 0;
}

int OdinPatcherPrivate::la_close_cb(archive *a, void *userdata)
{
    (void) a;
    auto *priv = static_cast<OdinPatcherPrivate *>(userdata);

    if (!priv->la_file.close()) {
        LOGE("%s: Failed to close: %s", priv->info->input_path().c_str(),
             priv->la_file.error_string().c_str());
        priv->error = ErrorCode::FileCloseError;
        return -1;
    }

    return 0;
}

}
}
