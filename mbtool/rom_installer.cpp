/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "rom_installer.h"

#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

#include <libmbp/bootimage.h>
#include <libmbp/logging.h>

#include "external/cppformat/format.h"
#include "installer.h"
#include "util/archive.h"
#include "util/chown.h"
#include "util/command.h"
#include "util/copy.h"
#include "util/file.h"
#include "util/finally.h"
#include "util/logging.h"
#include "util/selinux.h"
#include "util/string.h"


namespace mb
{


class RomInstaller : public Installer
{
public:
    RomInstaller(std::string zip_file, std::string rom_id);

    virtual void display_msg(const std::string& msg) override;
    virtual std::string get_install_type() override;
    virtual ProceedState on_checked_device() override;
    virtual ProceedState on_pre_install() override;
    virtual ProceedState on_unmounted_filesystems() override;
    virtual void on_cleanup(ProceedState ret) override;

private:
    std::string _rom_id;

    std::string _ld_library_path;
    std::string _ld_preload;
};


RomInstaller::RomInstaller(std::string zip_file, std::string rom_id) :
    Installer(zip_file, "/chroot", "/multiboot", 3, STDOUT_FILENO),
    _rom_id(std::move(rom_id))
{
}

void RomInstaller::display_msg(const std::string &msg)
{
    printf("[MultiBoot] %s\n", msg.c_str());
}

std::string RomInstaller::get_install_type()
{
    return _rom_id;
}

Installer::ProceedState RomInstaller::on_checked_device()
{
    // /sbin is not going to be populated with anything useful in a normal boot
    // image. We can almost guarantee that a recovery image is going to be
    // installed though, so we'll open the recovery partition with libmbp and
    // extract its /sbin with libarchive into the chroot's /sbin.

    typedef std::unique_ptr<archive, int (*)(archive *)> archive_ptr;

    mbp::BootImage bi;
    if (!bi.load(_recovery_block_dev)) {
        display_msg("Failed to load recovery partition image");
        return ProceedState::Fail;
    }

    std::vector<unsigned char> ramdisk = bi.ramdiskImage();

    archive_ptr in(archive_read_new(), archive_read_free);
    archive_ptr out(archive_write_disk_new(), archive_write_free);

    if (!in | !out) {
        LOGE("Out of memory");
        return ProceedState::Fail;
    }

    archive_entry *entry;

    // Set up input
    archive_read_support_filter_gzip(in.get());
    archive_read_support_filter_lz4(in.get());
    archive_read_support_format_cpio(in.get());

    int ret = archive_read_open_memory(in.get(),
            const_cast<unsigned char *>(ramdisk.data()), ramdisk.size());
    if (ret != ARCHIVE_OK) {
        LOGW("Failed to open recovery ramdisk: {}",
             archive_error_string(in.get()));
        return ProceedState::Fail;
    }

    // Set up output
    archive_write_disk_set_options(out.get(),
                                   ARCHIVE_EXTRACT_ACL |
                                   ARCHIVE_EXTRACT_FFLAGS |
                                   ARCHIVE_EXTRACT_PERM |
                                   ARCHIVE_EXTRACT_SECURE_NODOTDOT |
                                   ARCHIVE_EXTRACT_SECURE_SYMLINKS |
                                   ARCHIVE_EXTRACT_TIME |
                                   ARCHIVE_EXTRACT_UNLINK |
                                   ARCHIVE_EXTRACT_XATTR);

    while ((ret = archive_read_next_header(in.get(), &entry)) == ARCHIVE_OK) {
        std::string path = archive_entry_pathname(entry);

        if (!util::starts_with(path, "sbin/")) {
            continue;
        }

        LOGE("Copying from recovery: {}", path);

        archive_entry_set_pathname(entry, (_chroot + "/" + path).c_str());

        if (util::archive_copy_header_and_data(in.get(), out.get(), entry) != ARCHIVE_OK) {
            return ProceedState::Fail;
        }

        archive_entry_set_pathname(entry, path.c_str());
    }

    if (ret != ARCHIVE_EOF) {
        LOGE("Archive extraction ended without reaching EOF: {}",
             archive_error_string(in.get()));
        return ProceedState::Fail;
    }

    return ProceedState::Continue;
}

Installer::ProceedState RomInstaller::on_pre_install()
{
    if (is_aroma(_temp + "/updater")) {
        display_msg("ZIP files using the AROMA installer can only be flashed "
                    "from recovery");
        return ProceedState::Cancel;
    }

    char *ld_library_path = getenv("LD_LIBRARY_PATH");
    if (ld_library_path) {
        _ld_library_path = ld_library_path;
    }

    char *ld_preload = getenv("LD_PRELOAD");
    if (ld_preload) {
        _ld_preload = ld_preload;
    }

    if (setenv("LD_LIBRARY_PATH", "/sbin", 1) < 0) {
        LOGE("Failed to set LD_LIBRARY_PATH: {}", strerror(errno));
        return ProceedState::Fail;
    }
    if (unsetenv("LD_PRELOAD") < 0) {
        LOGE("Failed to unset LD_PRELOAD: {}", strerror(errno));
        return ProceedState::Fail;
    }

    return ProceedState::Continue;
}

Installer::ProceedState RomInstaller::on_unmounted_filesystems()
{
    if (!_ld_library_path.empty()) {
        if (setenv("LD_LIBRARY_PATH", _ld_library_path.c_str(), 1) < 0) {
            LOGE("Failed to set LD_LIBRARY_PATH: {}", strerror(errno));
            return ProceedState::Fail;
        }
    }
    if (!_ld_preload.empty()) {
        if (setenv("LD_PRELOAD", _ld_preload.c_str(), 1) < 0) {
            LOGE("Failed to set LD_PRELOAD: {}", strerror(errno));
            return ProceedState::Fail;
        }
    }

    return ProceedState::Continue;
}

void RomInstaller::on_cleanup(Installer::ProceedState ret)
{
    (void) ret;

    // Fix permissions on log file
    static const char *log_file = "/data/media/0/MultiBoot.log";

    if (chmod(log_file, 0664) < 0) {
        LOGE("{}: Failed to chmod: {}", log_file, strerror(errno));
    }

    if (!util::chown(log_file, "media_rw", "media_rw", 0)) {
        LOGE("{}: Failed to chown: {}", log_file, strerror(errno));
        if (chown(log_file, 1023, 1023) < 0) {
            LOGE("{}: Failed to chown: {}", log_file, strerror(errno));
        }
    }

    if (!util::selinux_set_context(
            log_file, "u:object_r:media_rw_data_file:s0")) {
        LOGE("{}: Failed to set context: {}", log_file, strerror(errno));
    }

    display_msg("The log file was saved as MultiBoot.log on the "
                "internal storage.");
}


static void mbp_log_cb(mbp::LogLevel prio, const std::string &msg)
{
    switch (prio) {
    case mbp::LogLevel::Debug:
        LOGD("{}", msg);
        break;
    case mbp::LogLevel::Error:
        LOGE("{}", msg);
        break;
    case mbp::LogLevel::Info:
        LOGI("{}", msg);
        break;
    case mbp::LogLevel::Verbose:
        LOGV("{}", msg);
        break;
    case mbp::LogLevel::Warning:
        LOGW("{}", msg);
        break;
    }
}

static void rom_installer_usage(bool error)
{
    FILE *stream = error ? stderr : stdout;

    fprintf(stream,
            "Usage: rom-installer [zip_file] [-r romid]\n\n"
            "Options:\n"
            "  -r, --romid      ROM install type/ID (primary, dual, etc.)\n"
            "  -h, --help       Display this help message\n"
            "  --stdout         Force log output to stdout\n");
}

int rom_installer_main(int argc, char *argv[])
{
    // Make stdout unbuffered
    setvbuf(stdout, nullptr, _IONBF, 0);

    // mbtool logging
    util::log_set_target(util::LogTarget::STDOUT);

    // libmbp logging
    mbp::setLogCallback(mbp_log_cb);

    std::string rom_id;
    std::string zip_file;

    int opt;

    static struct option long_options[] = {
        {"romid", required_argument, 0, 'r'},
        {"help",  no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, "r:h", long_options, &long_index)) != -1) {
        switch (opt) {
        case 'r':
            rom_id = optarg;
            break;

        case 'h':
            rom_installer_usage(false);
            return EXIT_SUCCESS;

        default:
            rom_installer_usage(true);
            return EXIT_FAILURE;
        }
    }

    if (argc - optind != 1) {
        rom_installer_usage(true);
        return EXIT_FAILURE;
    }

    zip_file = argv[optind];

    if (rom_id.empty()) {
        fprintf(stderr, "-r/--romid must be specified\n");
        return EXIT_FAILURE;
    }

    if (zip_file.empty()) {
        fprintf(stderr, "Invalid zip file path\n");
        return EXIT_FAILURE;
    }


    // Make sure install type is valid
    std::vector<std::shared_ptr<Rom>> roms;
    mb_roms_add_builtin(&roms);

    if (!mb_find_rom_by_id(&roms, rom_id)) {
        fprintf(stderr, "Invalid ROM ID: %s\n", rom_id.c_str());
        return EXIT_FAILURE;
    }

    // TODO: Don't overwrite current ROM
    auto rom = mb_get_current_rom();
    if (!rom) {
        fprintf(stderr, "Could not determine current ROM\n");
        return EXIT_FAILURE;
    }

    if (rom->id == rom_id) {
        fprintf(stderr, "Can't install over current ROM (%s)\n",
                rom_id.c_str());
        return EXIT_FAILURE;
    }


    if (geteuid() != 0) {
        LOGE("rom-installer must be run as root");
        return EXIT_FAILURE;
    }


    // Mount / writable
    if (mount("", "/", "", MS_REMOUNT, "") < 0) {
        LOGE("Failed to remount rootfs as rw: {}", strerror(errno));
    }

    auto remount_ro = util::finally([&] {
        if (mount("", "/", "", MS_REMOUNT | MS_RDONLY, "") < 0) {
            LOGE("Failed to remount rootfs as ro: {}", strerror(errno));
        }
    });


    // Until we get a good set of rules to allow, just temporarily set SELinux
    // to permissive mode until installation is finished
    int enforcing = -1;
    if (!util::selinux_get_enforcing(&enforcing)) {
        LOGE("Failed to get SELinux enforcing status");
    }

    LOGD("Current SELinux enforcing status: {:d}", enforcing);

    auto restore_selinux = util::finally([&] {
        if (enforcing >= 0) {
            if (!util::selinux_set_enforcing(enforcing)) {
                LOGE("Failed to set SELinux enforcing to {:d}", 0);
            }
        }
    });

    if (!util::selinux_set_enforcing(0)) {
        LOGE("Failed to set SELinux enforcing to {:d}", enforcing);
    }

    // Start installing!
    RomInstaller ri(zip_file, rom_id);
    return ri.start_installation() ? EXIT_SUCCESS : EXIT_FAILURE;
}

}