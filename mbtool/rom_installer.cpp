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
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mblog/logging.h"
#include "mblog/stdio_logger.h"
#include "mbp/bootimage.h"
#include "mbutil/autoclose/archive.h"
#include "mbutil/autoclose/file.h"
#include "mbutil/archive.h"
#include "mbutil/chown.h"
#include "mbutil/command.h"
#include "mbutil/copy.h"
#include "mbutil/file.h"
#include "mbutil/finally.h"
#include "mbutil/properties.h"
#include "mbutil/selinux.h"
#include "mbutil/string.h"

#include "installer.h"
#include "multiboot.h"


static const char *sepolicy_bak_path = "/sepolicy.rom-installer";

namespace mb
{

class RomInstaller : public Installer
{
public:
    RomInstaller(std::string zip_file, std::string rom_id, std::FILE *log_fp);

    virtual void display_msg(const std::string& msg) override;
    virtual void updater_print(const std::string &msg) override;
    virtual void command_output(const std::string &line) override;
    virtual std::string get_install_type() override;
    virtual std::unordered_map<std::string, std::string> get_properties() override;
    virtual ProceedState on_checked_device() override;
    virtual ProceedState on_pre_install() override;
    virtual ProceedState on_unmounted_filesystems() override;
    virtual void on_cleanup(ProceedState ret) override;

private:
    std::string _rom_id;
    std::FILE *_log_fp;

    std::string _ld_library_path;
    std::string _ld_preload;

    std::unordered_map<std::string, std::string> _recovery_props;
};


RomInstaller::RomInstaller(std::string zip_file, std::string rom_id,
                           std::FILE *log_fp) :
    Installer(zip_file, "/chroot", "/multiboot", 3, -1),
    _rom_id(std::move(rom_id)),
    _log_fp(log_fp)
{
}

void RomInstaller::display_msg(const std::string &msg)
{
    printf("[MultiBoot] %s\n", msg.c_str());
    LOGV("%s", msg.c_str());
}

void RomInstaller::updater_print(const std::string &msg)
{
    fprintf(_log_fp, "%s", msg.c_str());
    fflush(_log_fp);
    printf("%s", msg.c_str());
}

void RomInstaller::command_output(const std::string &line)
{
    fprintf(_log_fp, "%s", line.c_str());
    fflush(_log_fp);
}

std::string RomInstaller::get_install_type()
{
    return _rom_id;
}

std::unordered_map<std::string, std::string> RomInstaller::get_properties()
{
    static std::vector<std::string> needed_props{
        "ro.product.device",
        "ro.build.product",
        "ro.bootloader"
    };

    std::unordered_map<std::string, std::string> props(_recovery_props);

    // Copy required properties from system if they don't exist in the
    // properties file
    for (auto const &prop : needed_props) {
        if (props.find(prop) != props.end() && !props[prop].empty()) {
            continue;
        }

        std::string value;
        util::get_property(prop, &value, "");
        if (!value.empty()) {
            props[prop] = value;

            LOGD("Property '%s' does not exist in recovery's default.prop",
                 prop.c_str());
            LOGD("- '%s'='%s' will be set in chroot environment",
                 prop.c_str(), value.c_str());
        }
    }

    return props;
}

Installer::ProceedState RomInstaller::on_checked_device()
{
    // /sbin is not going to be populated with anything useful in a normal boot
    // image. We can almost guarantee that a recovery image is going to be
    // installed though, so we'll open the recovery partition with libmbp and
    // extract its /sbin with libarchive into the chroot's /sbin.

    // We don't need the recovery partition if the device doesn't use one
    bool combined =
            _device->flags() & mbp::Device::FLAG_HAS_COMBINED_BOOT_AND_RECOVERY;

    // We don't need the recovery partition if the device doesn't use one
    if (!combined && _recovery_block_dev.empty()) {
        display_msg("Could not determine the recovery block device");
        return ProceedState::Fail;
    }

    mbp::BootImage bi;
    if (!bi.loadFile(_recovery_block_dev)) {
        display_msg("Failed to load recovery partition image");
        return ProceedState::Fail;
    }

    const unsigned char *ramdisk_data;
    std::size_t ramdisk_size;
    mbp::CpioFile innerCpio;

    bi.ramdiskImageC(&ramdisk_data, &ramdisk_size);

    // If _recovery_block_dev is set to the FOTAKernel partition, then we'll use
    // that instead of the combined ramdisk from the boot partition
    if (combined && _recovery_block_dev.empty()) {
        if (!innerCpio.load(ramdisk_data, ramdisk_size)) {
            display_msg("Failed to load ramdisk from combined boot image");
            return ProceedState::Fail;
        }

        if (!innerCpio.exists("sbin/ramdisk-recovery.cpio")) {
            display_msg("Could not find recovery ramdisk in combined boot image");
            return ProceedState::Fail;
        }

        innerCpio.contentsC("sbin/ramdisk-recovery.cpio",
                            &ramdisk_data, &ramdisk_size);
    } else {
        bi.ramdiskImageC(&ramdisk_data, &ramdisk_size);
    }

    autoclose::archive in(archive_read_new(), archive_read_free);
    autoclose::archive out(archive_write_disk_new(), archive_write_free);

    if (!in | !out) {
        LOGE("Out of memory");
        return ProceedState::Fail;
    }

    archive_entry *entry;

    // Set up input
    archive_read_support_filter_gzip(in.get());
    archive_read_support_filter_lzop(in.get());
    archive_read_support_filter_lz4(in.get());
    archive_read_support_filter_lzma(in.get());
    archive_read_support_filter_xz(in.get());
    archive_read_support_format_cpio(in.get());

    int ret = archive_read_open_memory(in.get(),
            const_cast<unsigned char *>(ramdisk_data), ramdisk_size);
    if (ret != ARCHIVE_OK) {
        LOGW("Failed to open recovery ramdisk: %s",
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

        if (path == "default.prop") {
            path = "default.recovery.prop";
        } else if (!util::starts_with(path, "sbin/")) {
            continue;
        }

        LOGE("Copying from recovery: %s", path.c_str());

        archive_entry_set_pathname(entry, in_chroot(path).c_str());

        if (util::libarchive_copy_header_and_data(
                in.get(), out.get(), entry) != ARCHIVE_OK) {
            return ProceedState::Fail;
        }

        archive_entry_set_pathname(entry, path.c_str());
    }

    if (ret != ARCHIVE_EOF) {
        LOGE("Archive extraction ended without reaching EOF: %s",
             archive_error_string(in.get()));
        return ProceedState::Fail;
    }

    // Load recovery properties
    util::file_get_all_properties(
            in_chroot("/default.recovery.prop"), &_recovery_props);

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
        LOGE("Failed to set LD_LIBRARY_PATH: %s", strerror(errno));
        return ProceedState::Fail;
    }
    if (unsetenv("LD_PRELOAD") < 0) {
        LOGE("Failed to unset LD_PRELOAD: %s", strerror(errno));
        return ProceedState::Fail;
    }

    return ProceedState::Continue;
}

Installer::ProceedState RomInstaller::on_unmounted_filesystems()
{
    if (!_ld_library_path.empty()) {
        if (setenv("LD_LIBRARY_PATH", _ld_library_path.c_str(), 1) < 0) {
            LOGE("Failed to set LD_LIBRARY_PATH: %s", strerror(errno));
            return ProceedState::Fail;
        }
    }
    if (!_ld_preload.empty()) {
        if (setenv("LD_PRELOAD", _ld_preload.c_str(), 1) < 0) {
            LOGE("Failed to set LD_PRELOAD: %s", strerror(errno));
            return ProceedState::Fail;
        }
    }

    return ProceedState::Continue;
}

void RomInstaller::on_cleanup(Installer::ProceedState ret)
{
    (void) ret;

    display_msg("The log file was saved as MultiBoot.log on the "
                "internal storage.");
}

static bool backup_sepolicy(const std::string backup_path)
{
    if (!util::copy_contents(SELINUX_POLICY_FILE, backup_path)) {
        fprintf(stderr, "Failed to backup current SELinux policy: %s\n",
                strerror(errno));
        return false;
    }

    return true;
}

static bool restore_sepolicy(const std::string &backup_path)
{
    int fd = open(backup_path.c_str(), O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open backup SELinux policy file: %s\n",
                strerror(errno));
        return false;
    }

    auto close_fd = util::finally([&] {
        close(fd);
    });

    struct stat sb;

    if (fstat(fd, &sb) < 0) {
        fprintf(stderr, "Failed to stat backup SELinux policy file: %s\n",
                strerror(errno));
        return false;
    }

    void *map = mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) {
        fprintf(stderr, "Failed to mmap backup SELinux policy file: %s\n",
                strerror(errno));
        return false;
    }

    auto unmap_map = util::finally([&] {
        munmap(map, sb.st_size);
    });

    // Write policy
    int fd_out = open(SELINUX_LOAD_FILE, O_WRONLY);
    if (fd_out < 0) {
        fprintf(stderr, "Failed to open %s: %s\n",
                SELINUX_LOAD_FILE, strerror(errno));
        return false;
    }

    auto close_fd_out = util::finally([&] {
        close(fd_out);
    });

    if (write(fd_out, map, sb.st_size) < 0) {
        fprintf(stderr, "Failed to write to %s: %s\n",
                SELINUX_LOAD_FILE, strerror(errno));
        return false;
    }

    return true;
}

static bool patch_sepolicy()
{
    policydb_t pdb;

    if (policydb_init(&pdb) < 0) {
        fprintf(stderr, "Failed to initialize policydb\n");
        return false;
    }

    if (!util::selinux_read_policy(SELINUX_POLICY_FILE, &pdb)) {
        fprintf(stderr, "Failed to read SELinux policy file: %s\n",
                SELINUX_POLICY_FILE);
        policydb_destroy(&pdb);
        return false;
    }

    printf("SELinux policy version: %d\n", pdb.policyvers);

    util::selinux_make_all_permissive(&pdb);

    if (!util::selinux_write_policy(SELINUX_LOAD_FILE, &pdb)) {
        fprintf(stderr, "Failed to write SELinux policy file: %s\n",
                SELINUX_LOAD_FILE);
        policydb_destroy(&pdb);
        return false;
    }

    policydb_destroy(&pdb);

    return true;
}

static void rom_installer_usage(bool error)
{
    FILE *stream = error ? stderr : stdout;

    fprintf(stream,
            "Usage: rom-installer [zip_file] [-r romid]\n\n"
            "Options:\n"
            "  -r, --romid      ROM install type/ID (primary, dual, etc.)\n"
            "  -h, --help       Display this help message\n");
}

int rom_installer_main(int argc, char *argv[])
{
    // Make stdout unbuffered
    setvbuf(stdout, nullptr, _IONBF, 0);

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


    // Translate paths
    char *emu_source_path = getenv("EMULATED_STORAGE_SOURCE");
    char *emu_target_path = getenv("EMULATED_STORAGE_TARGET");
    if (emu_source_path && emu_target_path) {
        if (util::starts_with(zip_file, emu_target_path)) {
            printf("Zip path uses EMULATED_STORAGE_TARGET\n");
            zip_file.erase(0, strlen(emu_target_path));
            zip_file.insert(0, emu_source_path);
        }
    }


    // Make sure install type is valid
    if (!Roms::is_valid(rom_id)) {
        fprintf(stderr, "Invalid ROM ID: %s\n", rom_id.c_str());
        return EXIT_FAILURE;
    }

    auto rom = Roms::get_current_rom();
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
        fprintf(stderr, "rom-installer must be run as root\n");
        return EXIT_FAILURE;
    }


    // Since many stock ROMs, most notably TouchWiz, don't allow setting SELinux
    // to be globally permissive, we'll do the next best thing: modify the
    // policy to make every type permissive.

    bool selinux_supported = true;
    bool backup_successful = true;

    struct stat sb;
    if (stat("/sys/fs/selinux", &sb) < 0) {
        printf("SELinux not supported. No need to modify policy\n");
        selinux_supported = false;
    }

    if (selinux_supported) {
        backup_successful = backup_sepolicy(sepolicy_bak_path);

        printf("Patching SELinux policy to make all types permissive\n");
        if (!patch_sepolicy()) {
            fprintf(stderr, "Failed to patch current SELinux policy\n");
        }
    }

    auto restore_selinux = util::finally([&] {
        if (selinux_supported && backup_successful) {
            printf("Restoring backup SELinux policy\n");
            if (!restore_sepolicy(sepolicy_bak_path)) {
                fprintf(stderr, "Failed to restore SELinux policy\n");
            }
        }
    });


    autoclose::file fp(autoclose::fopen(MULTIBOOT_LOG_INSTALLER, "wb"));
    if (!fp) {
        fprintf(stderr, "Failed to open %s: %s\n",
                MULTIBOOT_LOG_INSTALLER, strerror(errno));
        return EXIT_FAILURE;
    }

    fix_multiboot_permissions();

    // Close stdin
    int fd = open("/dev/null", O_RDONLY);
    if (fd >= 0) {
        dup2(fd, STDIN_FILENO);
        close(fd);
    }

    // mbtool logging
    log::log_set_logger(std::make_shared<log::StdioLogger>(fp.get(), false));

    // Start installing!
    RomInstaller ri(zip_file, rom_id, fp.get());
    return ri.start_installation() ? EXIT_SUCCESS : EXIT_FAILURE;
}

}
