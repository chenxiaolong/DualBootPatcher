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
#  include <cerrno>
#endif

#include "mbcommon/integer.h"
#include "mbcommon/locale.h"
#include "mbcommon/string.h"

#include "mbdevice/json.h"

#include "mblog/logging.h"

#include "mbpatcher/patcherconfig.h"
#include "mbpatcher/patchers/zippatcher.h"
#include "mbpatcher/private/fileutils.h"
#include "mbpatcher/private/miniziputils.h"
#include "mbpatcher/private/stringutils.h"

// minizip
#include "minizip/zip.h"

#define LOG_TAG "mbpatcher/patchers/odinpatcher"

class ar;

namespace mb
{
namespace patcher
{


const std::string OdinPatcher::Id("OdinPatcher");


OdinPatcher::OdinPatcher(PatcherConfig &pc)
    : m_pc(pc)
    , m_info(nullptr)
    , m_old_bytes(0)
    , m_bytes(0)
    , m_max_bytes(0)
    , m_cancelled(0)
    , m_error()
    , m_la_buf()
    , m_la_file()
#ifdef __ANDROID__
    , m_fd(-1)
#endif
    , m_added_files()
    , m_progress_cb()
    , m_details_cb()
    , m_userdata()
    , m_a_input(nullptr)
    , m_z_output(nullptr)
{
}

OdinPatcher::~OdinPatcher() = default;

ErrorCode OdinPatcher::error() const
{
    return m_error;
}

std::string OdinPatcher::id() const
{
    return Id;
}

void OdinPatcher::set_file_info(const FileInfo * const info)
{
    m_info = info;
}

void OdinPatcher::cancel_patching()
{
    m_cancelled = true;
}

bool OdinPatcher::patch_file(ProgressUpdatedCallback progress_cb,
                             FilesUpdatedCallback files_cb,
                             DetailsUpdatedCallback details_cb,
                             void *userdata)
{
    (void) files_cb;

    m_cancelled = false;

    assert(m_info != nullptr);

    m_progress_cb = progress_cb;
    m_details_cb = details_cb;
    m_userdata = userdata;

    m_old_bytes = 0;
    m_bytes = 0;
    m_max_bytes = 0;

    bool ret = patch_tar();

    m_progress_cb = nullptr;
    m_details_cb = nullptr;
    m_userdata = nullptr;

    if (m_a_input != nullptr) {
        close_input_archive();
    }
    if (m_z_output != nullptr) {
        close_output_archive();
    }

    if (m_cancelled) {
        m_error = ErrorCode::PatchingCancelled;
        return false;
    }

    return ret;
}

struct CopySpec
{
    std::string source;
    std::string target;
};

bool OdinPatcher::patch_tar()
{
#ifdef __ANDROID__
    static const char *prefix = "/proc/self/fd/";
    m_fd = -1;
    if (starts_with(m_info->input_path(), prefix)) {
        std::string fd_str = m_info->input_path().substr(strlen(prefix));
        if (!str_to_num(fd_str.c_str(), 10, m_fd)) {
            LOGE("Invalid fd: %s", fd_str.c_str());
            m_error = ErrorCode::FileOpenError;
            return false;
        }
        LOGD("Input path '%s' is a file descriptor: %d",
             m_info->input_path().c_str(), m_fd);
    }
#endif

    update_progress(m_bytes, m_max_bytes);

    if (!open_input_archive()) {
        return false;
    }
    if (!open_output_archive()) {
        return false;
    }

    if (m_cancelled) return false;

    // Get file size and seek back to original location
    auto current_pos = m_la_file.seek(0, SEEK_CUR);
    if (!current_pos) {
        LOGE("%s: Failed to seek: %s", m_info->input_path().c_str(),
             current_pos.error().message().c_str());
        m_error = ErrorCode::FileSeekError;
        return false;
    }

    auto seek_ret = m_la_file.seek(0, SEEK_END);
    if (!seek_ret) {
        LOGE("%s: Failed to seek: %s", m_info->input_path().c_str(),
             seek_ret.error().message().c_str());
        m_error = ErrorCode::FileSeekError;
        return false;
    }
    m_max_bytes = seek_ret.value();

    seek_ret = m_la_file.seek(static_cast<int64_t>(current_pos.value()),
                              SEEK_SET);
    if (!seek_ret) {
        LOGE("%s: Failed to seek: %s", m_info->input_path().c_str(),
             seek_ret.error().message().c_str());
        m_error = ErrorCode::FileSeekError;
        return false;
    }

    if (m_cancelled) return false;

    if (!process_contents(m_a_input, 0)) {
        return false;
    }

    std::string arch_dir(m_pc.data_directory());
    arch_dir += "/binaries/android/";
    arch_dir += m_info->device().architecture();

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
            m_pc.data_directory() + "/scripts/bb-wrapper.sh",
            "multiboot/bb-wrapper.sh"
        }, {
            m_pc.data_directory() + "/scripts/bb-wrapper.sh.sig",
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

    zipFile zf = MinizipUtils::ctx_get_zip_file(m_z_output);

    ErrorCode result;

    for (const CopySpec &spec : to_copy) {
        if (m_cancelled) return false;

        update_details(spec.target);

        result = MinizipUtils::add_file(zf, spec.target, spec.source);
        if (result != ErrorCode::NoError) {
            m_error = result;
            return false;
        }
    }

    if (m_cancelled) return false;

    update_details("multiboot/info.prop");

    const std::string info_prop =
            ZipPatcher::create_info_prop(m_info->rom_id(), false);
    result = MinizipUtils::add_file(
            zf, "multiboot/info.prop",
            std::vector<unsigned char>(info_prop.begin(), info_prop.end()));
    if (result != ErrorCode::NoError) {
        m_error = result;
        return false;
    }

    if (m_cancelled) return false;

    update_details("multiboot/device.json");

    std::string json;
    if (!device::device_to_json(m_info->device(), json)) {
        m_error = ErrorCode::MemoryAllocationError;
        return false;
    }

    result = MinizipUtils::add_file(
            zf, "multiboot/device.json",
            std::vector<unsigned char>(json.begin(), json.end()));
    if (result != ErrorCode::NoError) {
        m_error = result;
        return false;
    }

    if (m_cancelled) return false;

    return true;
}

bool OdinPatcher::process_file(archive *a, archive_entry *entry, bool sparse)
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

    zipFile zf = MinizipUtils::ctx_get_zip_file(m_z_output);

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
        m_error = ErrorCode::ArchiveWriteHeaderError;
        return false;
    }

    la_ssize_t n_read;
    char buf[10240];
    while ((n_read = archive_read_data(a, buf, sizeof(buf))) > 0) {
        if (m_cancelled) return false;

        mz_ret = zipWriteInFileInZip(zf, buf, static_cast<uint32_t>(n_read));
        if (mz_ret != ZIP_OK) {
            LOGE("minizip: Failed to write %s in output zip: %s",
                 zip_name.c_str(),
                 MinizipUtils::zip_error_string(mz_ret).c_str());
            m_error = ErrorCode::ArchiveWriteDataError;
            zipCloseFileInZip(zf);
            return false;
        }
    }

    if (n_read != 0) {
        LOGE("libarchive: Failed to read %s: %s",
             name, archive_error_string(a));
        m_error = ErrorCode::ArchiveReadDataError;
        zipCloseFileInZip(zf);
        return false;
    }

    // Close file in output zip
    mz_ret = zipCloseFileInZip(zf);
    if (mz_ret != ZIP_OK) {
        LOGE("minizip: Failed to close file in output zip: %s",
             MinizipUtils::zip_error_string(mz_ret).c_str());
        m_error = ErrorCode::ArchiveWriteDataError;
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

bool OdinPatcher::process_contents(archive *a, unsigned int depth)
{
    if (depth > 1) {
        LOGW("Not traversing nested archive: depth > 1");
        return true;
    }

    archive_entry *entry;
    int la_ret;

    while ((la_ret = archive_read_next_header(a, &entry)) == ARCHIVE_OK) {
        if (m_cancelled) return false;

        const char *name = archive_entry_pathname(entry);
        if (!name) {
            continue;
        }

        update_details(name);

        // Certain files may be duplicated. For example, the cache.img file is
        // shipped on both the CSC and HOME_CSC tarballs.
        if (m_added_files.find(name) != m_added_files.end()) {
            LOGV("%sSkipping duplicate file: %s", indent(depth), name);
            continue;
        }

        if (strcmp(name, "boot.img") == 0) {
            LOGV("%sHandling boot image: %s", indent(depth), name);
            m_added_files.insert(name);

            if (!process_file(a, entry, false)) {
                return false;
            }
        } else if (starts_with(name, "cache.img")
                || starts_with(name, "system.img")) {
            LOGV("%sHandling sparse image: %s", indent(depth), name);
            m_added_files.insert(name);

            if (!process_file(a, entry, true)) {
                return false;
            }
        } else if (ends_with(name, ".tar.md5") || ends_with(name, ".tar")) {
            LOGV("%sHandling nested tarball: %s", indent(depth), name);

            NestedCtx ctx(a);
            if (!ctx.nested) {
                m_error = ErrorCode::MemoryAllocationError;
                return false;
            }

            archive_read_support_format_tar(ctx.nested);

            int ret = archive_read_open2(ctx.nested, &ctx, nullptr,
                                         &la_nested_read_cb, nullptr, nullptr);
            if (ret != ARCHIVE_OK) {
                LOGE("libarchive: Failed to open nested archive: %s: %s",
                     name, archive_error_string(ctx.nested));
                m_error = ErrorCode::ArchiveReadOpenError;
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
                m_error = ErrorCode::ArchiveReadDataError;
                return false;
            }
        }
    }

    if (la_ret != ARCHIVE_EOF) {
        LOGE("libarchive: Failed to read header: %s",
             archive_error_string(a));
        m_error = ErrorCode::ArchiveReadHeaderError;
        return false;
    }

    if (m_cancelled) return false;

    return true;
}

bool OdinPatcher::open_input_archive()
{
    assert(m_a_input == nullptr);

    m_a_input = archive_read_new();

    archive_read_support_format_zip(m_a_input);
    archive_read_support_format_tar(m_a_input);
    archive_read_support_filter_gzip(m_a_input);
    archive_read_support_filter_xz(m_a_input);

    // Our callbacks use the libmbcommon File API, which supports LFS on every
    // platform. Also allows progress info by counting number of bytes read.
    int ret = archive_read_open2(m_a_input, this, &la_open_cb, &la_read_cb,
                                 &la_skip_cb, &la_close_cb);
    if (ret != ARCHIVE_OK) {
        LOGW("libarchive: Failed to open for reading: %s",
             archive_error_string(m_a_input));
        archive_read_free(m_a_input);
        m_a_input = nullptr;
        m_error = ErrorCode::ArchiveReadOpenError;
        return false;
    }

    return true;
}

bool OdinPatcher::close_input_archive()
{
    assert(m_a_input != nullptr);

    bool ret = true;

    if (archive_read_close(m_a_input) != ARCHIVE_OK) {
        LOGW("libarchive: Failed to close archive: %s",
             archive_error_string(m_a_input));
        // Don't clobber previous error
        //m_error = ErrorCode::ArchiveCloseError;
        ret = false;
    }
    archive_read_free(m_a_input);
    m_a_input = nullptr;

    return ret;
}

bool OdinPatcher::open_output_archive()
{
    assert(m_z_output == nullptr);

    m_z_output = MinizipUtils::open_output_file(m_info->output_path());

    if (!m_z_output) {
        LOGE("minizip: Failed to open for writing: %s",
             m_info->output_path().c_str());
        m_error = ErrorCode::ArchiveWriteOpenError;
        return false;
    }

    return true;
}

bool OdinPatcher::close_output_archive()
{
    assert(m_z_output != nullptr);

    int ret = MinizipUtils::close_output_file(m_z_output);
    if (ret != ZIP_OK) {
        LOGW("minizip: Failed to close archive: %s",
             MinizipUtils::zip_error_string(ret).c_str());
        // Don't clobber previous error
        //m_error = ErrorCode::ArchiveCloseError;
        return false;
    }
    m_z_output = nullptr;

    return true;
}

void OdinPatcher::update_progress(uint64_t bytes, uint64_t max_bytes)
{
    if (m_progress_cb) {
        bool should_call = true;
        if (max_bytes > 0) {
            // Rate limit... call back only if percentage exceeds 0.01%
            double old_ratio = static_cast<double>(m_old_bytes)
                    / static_cast<double>(max_bytes);
            double new_ratio = static_cast<double>(bytes)
                    / static_cast<double>(max_bytes);
            if (new_ratio - old_ratio < 0.0001) {
                should_call = false;
            }
        }
        if (should_call) {
            m_progress_cb(bytes, max_bytes, m_userdata);
            m_old_bytes = bytes;
        }
    }
}

void OdinPatcher::update_details(const std::string &msg)
{
    if (m_details_cb) {
        m_details_cb(msg, m_userdata);
    }
}

la_ssize_t OdinPatcher::la_nested_read_cb(archive *a, void *userdata,
                                          const void **buffer)
{
    (void) a;

    NestedCtx *ctx = static_cast<NestedCtx *>(userdata);

    *buffer = ctx->buf;

    return archive_read_data(ctx->parent, ctx->buf, sizeof(ctx->buf));
}

la_ssize_t OdinPatcher::la_read_cb(archive *a, void *userdata,
                                   const void **buffer)
{
    (void) a;
    auto *p = static_cast<OdinPatcher *>(userdata);
    *buffer = p->m_la_buf;

    auto bytes_read = p->m_la_file.read(p->m_la_buf, sizeof(p->m_la_buf));
    if (!bytes_read) {
        LOGE("%s: Failed to read: %s", p->m_info->input_path().c_str(),
             bytes_read.error().message().c_str());
        p->m_error = ErrorCode::FileReadError;
        return -1;
    }

    p->m_bytes += bytes_read.value();
    p->update_progress(p->m_bytes, p->m_max_bytes);
    return static_cast<la_ssize_t>(bytes_read.value());
}

la_int64_t OdinPatcher::la_skip_cb(archive *a, void *userdata,
                                   la_int64_t request)
{
    (void) a;
    auto *p = static_cast<OdinPatcher *>(userdata);

    auto seek_ret = p->m_la_file.seek(request, SEEK_CUR);
    if (!seek_ret) {
        LOGE("%s: Failed to seek: %s", p->m_info->input_path().c_str(),
             seek_ret.error().message().c_str());
        p->m_error = ErrorCode::FileSeekError;
        return -1;
    }

    p->m_bytes = static_cast<uint64_t>(
            static_cast<int64_t>(p->m_bytes) + request);
    p->update_progress(p->m_bytes, p->m_max_bytes);
    return request;
}

int OdinPatcher::la_open_cb(archive *a, void *userdata)
{
    (void) a;
    auto *p = static_cast<OdinPatcher *>(userdata);
    oc::result<void> ret = oc::success();

#ifdef _WIN32
    auto w_filename = utf8_to_wcs(p->m_info->input_path());
    if (!w_filename) {
        LOGE("%s: Failed to convert from UTF8 to WCS",
             p->m_info->input_path().c_str());
        p->m_error = ErrorCode::FileOpenError;
        return -1;
    }

    ret = p->m_la_file.open(w_filename.value(), FileOpenMode::ReadOnly);
#else
#  ifdef __ANDROID__
    if (p->m_fd >= 0) {
        ret = p->m_la_file.open(p->m_fd, false);
    } else
#  endif
    ret = p->m_la_file.open(p->m_info->input_path(), FileOpenMode::ReadOnly);
#endif

    if (!ret) {
        LOGE("%s: Failed to open: %s", p->m_info->input_path().c_str(),
             ret.error().message().c_str());
        p->m_error = ErrorCode::FileOpenError;
        return -1;
    }

    return 0;
}

int OdinPatcher::la_close_cb(archive *a, void *userdata)
{
    (void) a;
    auto *p = static_cast<OdinPatcher *>(userdata);

    auto ret = p->m_la_file.close();
    if (!ret) {
        LOGE("%s: Failed to close: %s", p->m_info->input_path().c_str(),
             ret.error().message().c_str());
        p->m_error = ErrorCode::FileCloseError;
        return -1;
    }

    return 0;
}

}
}
