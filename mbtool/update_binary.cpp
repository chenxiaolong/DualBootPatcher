/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "update_binary.h"

#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mblog/logging.h"
#include "mblog/stdio_logger.h"
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
#include "sepolpatch.h"


namespace mb
{


class RecoveryInstaller : public Installer
{
public:
    RecoveryInstaller(std::string zip_file, int interface, int output_fd);

    virtual void display_msg(const std::string& msg) override;
    virtual std::string get_install_type() override;
    virtual std::unordered_map<std::string, std::string> get_properties() override;
    virtual ProceedState on_initialize() override;
    virtual ProceedState on_set_up_chroot() override;
    virtual void on_cleanup(ProceedState ret) override;
};


RecoveryInstaller::RecoveryInstaller(std::string zip_file, int interface, int output_fd) :
    Installer(zip_file, "/chroot", "/multiboot", interface, output_fd, 0)
{
}

void RecoveryInstaller::display_msg(const std::string &msg)
{
    dprintf(_output_fd, "ui_print [MultiBoot] %s\n", msg.c_str());
    dprintf(_output_fd, "ui_print\n");
}

std::string RecoveryInstaller::get_install_type()
{
    if (_prop.find("mbtool.installer.install-location") != _prop.end()) {
        std::string location = _prop["mbtool.installer.install-location"];
        display_msg("Installing to " + location);
        return location;
    } else {
        display_msg("Installation location not specified");
        return CANCELLED;
    }
}

std::unordered_map<std::string, std::string> RecoveryInstaller::get_properties()
{
    // Copy the recovery's properties
    std::unordered_map<std::string, std::string> props;
    util::property_get_all(props);
    return props;
}

Installer::ProceedState RecoveryInstaller::on_initialize()
{
    struct stat sb;
    if (stat("/sys/fs/selinux", &sb) == 0) {
        if (!patch_loaded_sepolicy(SELinuxPatch::CWM_RECOVERY)) {
            LOGE("Failed to patch sepolicy. Trying to disable SELinux");
            int fd = open(SELINUX_ENFORCE_FILE, O_WRONLY | O_CLOEXEC);
            if (fd >= 0) {
                write(fd, "0", 1);
                close(fd);
            } else {
                LOGE("Failed to set SELinux to permissive mode");
                display_msg("Could not patch or disable SELinux");
            }
        }
    }

    return ProceedState::Continue;
}

RecoveryInstaller::ProceedState RecoveryInstaller::on_set_up_chroot()
{
    // Copy /etc/fstab
    util::copy_file("/etc/fstab", in_chroot("/etc/fstab"),
                    util::COPY_ATTRIBUTES | util::COPY_XATTRS);

    // Copy /etc/recovery.fstab
    util::copy_file("/etc/recovery.fstab", in_chroot("/etc/recovery.fstab"),
                    util::COPY_ATTRIBUTES | util::COPY_XATTRS);

    return ProceedState::Continue;
}

void RecoveryInstaller::on_cleanup(Installer::ProceedState ret)
{
    (void) ret;

    if (!util::copy_file("/tmp/recovery.log", MULTIBOOT_LOG_INSTALLER, 0)) {
        LOGE("Failed to copy log file: %s", strerror(errno));
    }

    fix_multiboot_permissions();

    display_msg("The log file was saved as MultiBoot.log on the "
                "internal storage.");
}

static void update_binary_usage(int error)
{
    FILE *stream = error ? stderr : stdout;

    fprintf(stream,
            "Usage: update-binary [interface version] [output fd] [zip file]\n\n"
            "This tool wraps the real update-binary program by mounting the correct\n"
            "partitions in a chroot environment and then calls the real program.\n"
            "The real update-binary must be META-INF/com/google/android/update-binary.orig\n"
            "in the zip file.\n\n"
            "Note: The interface version argument is completely ignored.\n");
}

int update_binary_main(int argc, char *argv[])
{
    if (unshare(CLONE_NEWNS) < 0) {
        fprintf(stderr, "unshare() failed: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    if (mount("", "/", "", MS_PRIVATE | MS_REC, "") < 0) {
        fprintf(stderr, "Failed to set private mount propagation: %s\n",
                strerror(errno));
        return false;
    }

    // Make stdout unbuffered
    setvbuf(stdout, nullptr, _IONBF, 0);

    int opt;

    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, "h", long_options, &long_index)) != -1) {
        switch (opt) {
        case 'h':
            update_binary_usage(0);
            return EXIT_SUCCESS;

        default:
            update_binary_usage(1);
            return EXIT_FAILURE;
        }
    }

    if (argc - optind != 3) {
        update_binary_usage(1);
        return EXIT_FAILURE;
    }

    int interface;
    int output_fd;
    const char *zip_file;

    char *ptr;

    interface = strtol(argv[1], &ptr, 10);
    if (*ptr != '\0' || interface < 0) {
        fprintf(stderr, "Invalid interface");
        return EXIT_FAILURE;
    }

    output_fd = strtol(argv[2], &ptr, 10);
    if (*ptr != '\0' || output_fd < 0) {
        fprintf(stderr, "Invalid output fd");
        return EXIT_FAILURE;
    }

    zip_file = argv[3];

    // stdout is messed up when it's appended to /tmp/recovery.log
    log::log_set_logger(std::make_shared<log::StdioLogger>(stderr, false));

    RecoveryInstaller ri(zip_file, interface, output_fd);
    return ri.start_installation() ? EXIT_SUCCESS : EXIT_FAILURE;
}

}
