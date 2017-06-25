/*
 * Copyright (C) 2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "ramdisk_patcher.h"

#include <algorithm>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/stat.h>
#include <unistd.h>

#include "mbcommon/string.h"
#include "mblog/logging.h"
#include "mbutil/copy.h"
#include "mbutil/path.h"

#include "installer_util.h"

namespace mb
{

static bool _rp_write_rom_id(const std::string &dir, const std::string &rom_id)
{
    std::string path(dir);
    path += "/romid";

    FILE *fp = fopen(path.c_str(), "wb");
    if (!fp) {
        LOGE("%s: Failed to open for writing: %s",
             path.c_str(), strerror(errno));
        return false;
    }

    if (fputs(rom_id.c_str(), fp) == EOF) {
        LOGE("%s: Failed to write ROM ID: %s",
             path.c_str(), strerror(errno));
        fclose(fp);
        return false;
    }

    if (fchmod(fileno(fp), 0664) < 0) {
        LOGE("%s: Failed to chmod: %s",
             path.c_str(), strerror(errno));
        fclose(fp);
        return false;
    }

    if (fclose(fp) < 0) {
        LOGE("%s: Failed to close file: %s",
             path.c_str(), strerror(errno));
        return false;
    }

    return true;
}

std::function<RamdiskPatcherFn>
rp_write_rom_id(const std::string &rom_id)
{
    using namespace std::placeholders;

    return std::bind(_rp_write_rom_id, _1, rom_id);
}

static bool _rp_patch_default_prop(const std::string &dir,
                                   const std::string &device_id,
                                   bool use_fuse_exfat)
{
    char *tmp_path = nullptr;
    int tmp_fd = -1;
    FILE *fp_in = nullptr;
    FILE *fp_out = nullptr;
    char *buf = nullptr;
    size_t buf_size = 0;
    ssize_t n;
    bool ret = false;

    std::string path(dir);
    path += "/default.prop";

    tmp_path = mb_format("%s.XXXXXX", path.c_str());
    if (!tmp_path) {
        LOGE("Out of memory");
        goto done;
    }

    tmp_fd = mkstemp(tmp_path);
    if (tmp_fd < 0) {
        LOGE("%s: Failed to create temporary file: %s",
             path.c_str(), strerror(errno));
        goto done;
    }

    fp_in = fopen(path.c_str(), "rb");
    if (!fp_in) {
        LOGE("%s: Failed to open for reading: %s",
             path.c_str(), strerror(errno));
        goto done;
    }

    fp_out = fdopen(tmp_fd, "wb");
    if (!fp_out) {
        LOGE("%s: Failed to open for writing: %s",
             tmp_path, strerror(errno));
        goto done;
    }

    tmp_fd = -1;

    while ((n = getline(&buf, &buf_size, fp_in)) >= 0) {
        // Remove old multiboot properties
        if (mb_starts_with(buf, "ro.patcher.")) {
            continue;
        }

        if (fwrite(buf, 1, n, fp_out) != static_cast<size_t>(n)) {
            LOGE("%s: Failed to write file: %s",
                 tmp_path, strerror(errno));
            goto done;
        }
    }

    // Write new properties
    if (fputc('\n', fp_out) == EOF
            || fprintf(fp_out, "ro.patcher.device=%s\n", device_id.c_str()) < 0
            || fprintf(fp_out, "ro.patcher.use_fuse_exfat=%s\n",
                       use_fuse_exfat ? "true" : "false") < 0) {
        LOGE("%s: Failed to write properties: %s",
             tmp_path, strerror(errno));
        goto done;
    }

    if (fclose(fp_out) < 0) {
        LOGE("%s: Failed to close file: %s",
             tmp_path, strerror(errno));
        goto done;
    }

    fp_out = nullptr;

    if (!InstallerUtil::replace_file(path.c_str(), tmp_path)) {
        goto done;
    }

    ret = true;

done:
    if (tmp_fd >= 0) {
        close(tmp_fd);
    }
    if (fp_in) {
        fclose(fp_in);
    }
    if (fp_out) {
        fclose(fp_out);
    }

    unlink(tmp_path);

    free(tmp_path);
    free(buf);

    return ret;
}

std::function<RamdiskPatcherFn>
rp_patch_default_prop(const std::string &device_id, bool use_fuse_exfat)
{
    using namespace std::placeholders;

    return std::bind(_rp_patch_default_prop, _1, device_id, use_fuse_exfat);
}

static bool _rp_add_binaries(const std::string &dir,
                             const std::string &binaries_dir)
{
    struct CopySpec
    {
        std::string from;
        std::string to;
        mode_t perm;
    };

    std::vector<CopySpec> to_copy{
        { "file-contexts-tool",     "sbin/file-contexts-tool",     0750 },
        { "file-contexts-tool.sig", "sbin/file-contexts-tool.sig", 0640 },
        { "fsck-wrapper",           "sbin/fsck-wrapper",           0750 },
        { "fsck-wrapper.sig",       "sbin/fsck-wrapper.sig",       0640 },
        { "mbtool",                 "mbtool",                      0750 },
        { "mbtool.sig",             "mbtool.sig",                  0640 },
        { "mount.exfat",            "sbin/mount.exfat",            0750 },
        { "mount.exfat.sig",        "sbin/mount.exfat.sig",        0640 },
    };

    for (auto const &item : to_copy) {
        std::string source(binaries_dir);
        source += "/";
        source += item.from;
        std::string target(dir);
        target += "/";
        target += item.to;

        if (!util::copy_file(source, target, 0)) {
            return false;
        }
        if (chmod(target.c_str(), item.perm) < 0) {
            LOGE("%s: Failed to chmod: %s", target.c_str(), strerror(errno));
            return false;
        }
    }

    return true;
}

std::function<RamdiskPatcherFn>
rp_add_binaries(const std::string &binaries_dir)
{
    using namespace std::placeholders;

    return std::bind(_rp_add_binaries, _1, binaries_dir);
}

static bool _rp_symlink_fuse_exfat(const std::string &dir)
{
    std::string fsck_exfat(dir);
    fsck_exfat += "/sbin/fsck.exfat";
    std::string fsck_exfat_sig(fsck_exfat);
    fsck_exfat_sig += ".sig";

    if ((unlink(fsck_exfat.c_str()) < 0 && errno != ENOENT)
            || symlink("mount.exfat", fsck_exfat.c_str()) < 0
            || (unlink(fsck_exfat_sig.c_str()) < 0 && errno != ENOENT)
            || symlink("mount.exfat.sig", fsck_exfat_sig.c_str()) < 0) {
        LOGE("Failed to symlink exfat fsck binaries: %s", strerror(errno));
        return false;
    }

    return true;
}

std::function<RamdiskPatcherFn>
rp_symlink_fuse_exfat()
{
    return _rp_symlink_fuse_exfat;
}

static bool _rp_symlink_init(const std::string &dir)
{
    std::string target{dir};
    target += "/init";
    std::string real_init{dir};
    real_init += "/init.orig";

    struct stat sb;

    // If this is a Sony device that doesn't use sbin/ramdisk.cpio for the
    // combined ramdisk, we'll have to explicitly allow their init executable to
    // run first. We'll use a relatively strong heuristic to prevent false
    // positives with other devices.
    //
    // See:
    // * https://github.com/chenxiaolong/DualBootPatcher/issues/533
    // * https://github.com/sonyxperiadev/device-sony-common-init
    {
        std::string sony_init_wrapper(dir);
        sony_init_wrapper += "/sbin/init_sony";
        std::string sony_real_init(dir);
        sony_real_init += "/init.real";
        std::string sony_symlink_target;

        // Check that /init is a symlink and that /init.real exists
        if (lstat(target.c_str(), &sb) == 0 && S_ISLNK(sb.st_mode)
                && util::read_link(target, &sony_symlink_target)
                && lstat(sony_real_init.c_str(), &sb) == 0) {
            std::vector<std::string> haystack{util::path_split(sony_symlink_target)};
            std::vector<std::string> needle{util::path_split("sbin/init_sony")};

            util::normalize_path(&haystack);

            // Check that init points to some path with "sbin/init_sony" in it
            auto const it = std::search(haystack.cbegin(), haystack.cend(),
                                        needle.cbegin(), needle.cend());
            if (it != haystack.cend()) {
                target.swap(sony_real_init);
            }
        }
    }

    LOGD("[init] Target init path: %s", target.c_str());
    LOGD("[init] Real init path: %s", real_init.c_str());

    if (lstat(real_init.c_str(), &sb) < 0) {
        if (errno != ENOENT) {
            LOGE("%s: Failed to access file: %s",
                 real_init.c_str(), strerror(errno));
            return false;
        }

        if (rename(target.c_str(), real_init.c_str()) < 0) {
            LOGE("%s: Failed to rename file: %s",
                 target.c_str(), strerror(errno));
            return false;
        }

        if (symlink("/mbtool", target.c_str()) < 0) {
            LOGE("%s: Failed to symlink mbtool: %s",
                 target.c_str(), strerror(errno));
            return false;
        }
    }

    return true;
}

std::function<RamdiskPatcherFn>
rp_symlink_init()
{
    return _rp_symlink_init;
}

static bool _rp_add_device_json(const std::string &dir,
                                const std::string &device_json_file)
{
    std::string device_json(dir);
    device_json += "/device.json";

    if (!util::copy_file(device_json_file, device_json, 0)) {
        return false;
    }

    if (chmod(device_json.c_str(), 0644) < 0) {
        LOGE("%s: Failed to chmod file: %s",
             device_json.c_str(), strerror(errno));
        return false;
    }

    return true;
}

std::function<RamdiskPatcherFn>
rp_add_device_json(const std::string &device_json_file)
{
    using namespace std::placeholders;

    return std::bind(_rp_add_device_json, _1, device_json_file);
}

}
