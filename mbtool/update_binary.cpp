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

#include "update_binary.h"

#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>

#include <libmbp/logging.h>

#include "external/cppformat/format.h"
#include "installer.h"
#include "util/archive.h"
#include "util/chown.h"
#include "util/command.h"
#include "util/copy.h"
#include "util/file.h"
#include "util/logging.h"
#include "util/selinux.h"
#include "util/string.h"


namespace mb
{


class RecoveryInstaller : public Installer
{
public:
    RecoveryInstaller(std::string zip_file, int interface, int output_fd);

    virtual void display_msg(const std::string& msg) override;
    virtual std::string get_install_type() override;
    virtual ProceedState on_initialize() override;
    virtual void on_cleanup(ProceedState ret) override;

private:
    static bool patch_sepolicy();
    bool run_aroma_selection();
};


RecoveryInstaller::RecoveryInstaller(std::string zip_file, int interface, int output_fd) :
    Installer(zip_file, "/chroot", "/multiboot", interface, output_fd)
{
}

/*
 * To work around denials (on Samsung devices):
 * 1. Mount the system and data partitions
 *
 *      $ mount /system
 *      $ mount /data
 *
 * 2. Start the audit daemon
 *
 *      $ /system/bin/auditd
 *
 * 3. From another window, run mbtool's update-binary wrapper
 *
 *      $ /tmp/mbtool_recovery updater 3 1 /path/to/file_patched.zip
 *
 * 4. Pull /data/misc/audit/audit.log and run it through audit2allow
 *
 *      $ adb pull /data/misc/audit/audit.log
 *      $ grep denied audit.log | audit2allow
 *
 * 5. Allow the rule using util::selinux_add_rule()
 *
 *    Rules of the form:
 *      allow source target:class perm;
 *    Are allowed by calling:
 *      util::selinux_add_rule(&pdb, source, target, class, perm);
 *
 * --
 *
 * To view the allow rules for the currently loaded policy:
 *
 * 1. Pull the current policy file
 *
 *      $ adb pull /sys/fs/selinux/policy
 *
 * 2. View the rules (the -s, -t, -c, and -p parameters can be used to filter
 *    the rules by source, target, class, and permission, respectively)
 *
 *      $ sesearch -A policy
 */
bool RecoveryInstaller::patch_sepolicy()
{
    policydb_t pdb;

    if (policydb_init(&pdb) < 0) {
        LOGE("Failed to initialize policydb");
        return false;
    }

    if (!util::selinux_read_policy(MB_SELINUX_POLICY_FILE, &pdb)) {
        LOGE("Failed to read SELinux policy file: {}", MB_SELINUX_POLICY_FILE);
        policydb_destroy(&pdb);
        return false;
    }

    LOGD("Policy version: {}", pdb.policyvers);

    // Debugging rules (for CWM and Philz)
    util::selinux_add_rule(&pdb, "adbd",  "block_device",    "blk_file",   "relabelto");
    util::selinux_add_rule(&pdb, "adbd",  "graphics_device", "chr_file",   "relabelto");
    util::selinux_add_rule(&pdb, "adbd",  "graphics_device", "dir",        "relabelto");
    util::selinux_add_rule(&pdb, "adbd",  "input_device",    "chr_file",   "relabelto");
    util::selinux_add_rule(&pdb, "adbd",  "input_device",    "dir",        "relabelto");
    util::selinux_add_rule(&pdb, "adbd",  "rootfs",          "dir",        "relabelto");
    util::selinux_add_rule(&pdb, "adbd",  "rootfs",          "file",       "relabelto");
    util::selinux_add_rule(&pdb, "adbd",  "rootfs",          "lnk_file",   "relabelto");
    util::selinux_add_rule(&pdb, "adbd",  "system_file",     "file",       "relabelto");
    util::selinux_add_rule(&pdb, "adbd",  "tmpfs",           "file",       "relabelto");

    util::selinux_add_rule(&pdb, "rootfs", "tmpfs",          "filesystem", "associate");
    util::selinux_add_rule(&pdb, "tmpfs",  "rootfs",         "filesystem", "associate");

    if (!util::selinux_write_policy(MB_SELINUX_LOAD_FILE, &pdb)) {
        LOGE("Failed to write SELinux policy file: {}", MB_SELINUX_LOAD_FILE);
        policydb_destroy(&pdb);
        return false;
    }

    policydb_destroy(&pdb);

    return true;
}

/*!
 * \brief Run AROMA wrapper for ROM installation selection
 */
bool RecoveryInstaller::run_aroma_selection()
{
    std::vector<util::extract_info> aroma_files{
        { .from = UPDATE_BINARY, .to = in_chroot("/mb/updater") }
    };

    if (!util::extract_files2(_temp + "/aromawrapper.zip", aroma_files)) {
        LOGE("Failed to extract {}", UPDATE_BINARY);
        return false;
    }

    if (chmod(in_chroot("/mb/updater").c_str(), 0555) < 0) {
        LOGE("{}: Failed to chmod: {}",
             in_chroot("/mb/updater"), strerror(errno));
        return false;
    }

    if (!util::copy_file(_temp + "/aromawrapper.zip",
                         in_chroot("/mb/aromawrapper.zip"),
                         util::MB_COPY_ATTRIBUTES | util::MB_COPY_XATTRS)) {
        LOGE("Failed to copy {} to {}: {}",
             _temp + "/aromawrapper.zip", in_chroot("/mb/aromawrapper.zip"),
             strerror(errno));
        return false;
    }

    std::vector<std::string> argv{
        "/mb/updater",
        util::to_string(_interface),
        // Force output to stderr
        "2",
        "/mb/aromawrapper.zip"
    };

    // Stop parent process (/sbin/recovery), so AROMA doesn't fight over the
    // framebuffer. AROMA normally already does this, but when we wrap it, it
    // just stops the wraapper.
    pid_t parent = getppid();

    kill(parent, SIGSTOP);

    int ret;
    if ((ret = util::run_command_chroot(_chroot, argv)) != 0) {
        kill(parent, SIGCONT);

        if (ret < 0) {
            LOGE("Failed to execute {}: {}", "/mb/updater", strerror(errno));
        } else {
            LOGE("{} returned non-zero exit status", "/mb/updater");
        }
        return false;
    }

    kill(parent, SIGCONT);

    return true;
}

void RecoveryInstaller::display_msg(const std::string &msg)
{
    dprintf(_output_fd, "ui_print [MultiBoot] %s\n", msg.c_str());
    dprintf(_output_fd, "ui_print\n");
}

std::string RecoveryInstaller::get_install_type()
{
    // Choose install type
    if (!run_aroma_selection()) {
        display_msg("Failed to run AROMA");
        return std::string();
    }

    std::string install_type;

    if (!util::file_first_line(in_chroot("/mb/installtype"), &install_type)) {
        display_msg("Failed to determine install type");
        return std::string();
    }

    if (install_type == "cancelled") {
        return CANCELLED;
    }

    return install_type;
}

Installer::ProceedState RecoveryInstaller::on_initialize()
{
    struct stat sb;
    if (stat("/sys/fs/selinux", &sb) == 0) {
        if (!patch_sepolicy()) {
            LOGE("Failed to patch sepolicy. Trying to disable SELinux");
            int fd = open(MB_SELINUX_ENFORCE_FILE, O_WRONLY);
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

void RecoveryInstaller::on_cleanup(Installer::ProceedState ret)
{
    static const char *log_file = "/data/media/0/MultiBoot.log";

    if (ret == ProceedState::Fail) {
        if (!util::copy_file("/tmp/recovery.log", log_file, 0)) {
            LOGE("Failed to copy log file: {}", strerror(errno));
        }

        if (chmod(log_file, 0664) < 0) {
            LOGE("{}: Failed to chmod: {}", log_file, strerror(errno));
        }

        if (util::chown(log_file, "media_rw", "media_rw", 0)) {
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
    util::log_set_target(util::LogTarget::STDERR);

    mbp::setLogCallback(mbp_log_cb);

    RecoveryInstaller ri(zip_file, interface, output_fd);
    return ri.start_installation() ? EXIT_SUCCESS : EXIT_FAILURE;
}

}