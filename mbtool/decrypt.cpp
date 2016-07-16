/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "decrypt.h"

#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <getopt.h>
#include <signal.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "mblog/logging.h"
#include "mbutil/autoclose/dir.h"
#include "mbutil/autoclose/file.h"
#include "mbutil/command.h"
#include "mbutil/copy.h"
#include "mbutil/delete.h"
#include "mbutil/finally.h"
#include "mbutil/integer.h"
#include "mbutil/mount.h"
#include "mbutil/properties.h"
#include "mbutil/string.h"

#include "voldclient.h"

#define CRYPTO_SETUP_PATH               "/raw/cache/multiboot/crypto/setup"
#define CRYPTO_FSTAB_PATH               "/raw/cache/multiboot/crypto/fstab"

#define CHROOT_PATH                     "/crypto-chroot"

#define MAX_CONNECTION_ATTEMPTS         10

static pid_t setup_pid = -1;
static pid_t setup_reader_pid = -1;
static int setup_pipe[2];
static pid_t vold_pid = -1;
static pid_t vold_reader_pid = -1;
static int vold_pipe[2];

namespace mb
{

static int log_mkdir(const char *pathname, mode_t mode)
{
    int ret = mkdir(pathname, mode);
    if (ret < 0) {
        LOGE("Failed to create %s: %s", pathname, strerror(errno));
    }
    return ret;
}

static int log_mount(const char *source, const char *target, const char *fstype,
                     long unsigned int mountflags, const void *data)
{
    int ret = mount(source, target, fstype, mountflags, data);
    if (ret < 0) {
        LOGE("Failed to mount %s (%s) at %s: %s",
             source, fstype, target, strerror(errno));
    }
    return ret;
}

static int log_umount(const char *target)
{
    int ret = umount(target);
    if (ret < 0) {
        LOGE("Failed to unmount %s: %s", target, strerror(errno));
    }
    return ret;
}

static bool log_is_mounted(const std::string &mountpoint)
{
    bool ret = util::is_mounted(mountpoint);
    if (!ret) {
        LOGE("%s is not mounted", mountpoint.c_str());
    }
    return ret;
}

static bool log_unmount_all(const std::string &dir)
{
    bool ret = util::unmount_all(dir);
    if (!ret) {
        LOGE("Failed to unmount all mountpoints within %s", dir.c_str());
    }
    return ret;
}

static bool log_delete_recursive(const std::string &path)
{
    bool ret = util::delete_recursive(path);
    if (!ret) {
        LOGE("Failed to recursively remove %s", path.c_str());
    }
    return ret;
}

static void command_output_cb(const char *line, void *data)
{
    char buf[1024];
    size_t size = strlen(line);

    if (size > sizeof(buf) - 1) {
        return;
    }

    strcpy(buf, line);

    // Don't print newline
    if (size > 0 && line[size - 1] == '\n') {
        buf[size - 1] = '\0';
    }

    LOGD("%s output: %s", static_cast<const char *>(data), buf);
}

static pid_t start_output_reader(int fd, void (*cb)(const char *, void*),
                                 void *data)
{
    pid_t pid = fork();

    if (pid < 0) {
        LOGE("Failed to fork reader process: %s", strerror(errno));
        return -1;
    } else if (pid == 0) {
        char buf[1024];

        autoclose::file fp(fdopen(fd, "rb"), fclose);

        while (fgets(buf, sizeof(buf), fp.get())) {
            cb(buf, data);
        }

        _exit(EXIT_SUCCESS);
    }

    return pid;
}

static bool create_chroot()
{
    LOGV("Creating chroot...");

    // Ensure /raw/system in mounted
    if (!log_is_mounted("/raw/system")) {
        return false;
    }

    // Unmount everything previously mounted in the chroot
    if (!log_unmount_all(CHROOT_PATH)) {
        return false;
    }

    // Remove the chroot directory if it exists
    if (!log_delete_recursive(CHROOT_PATH)) {
        return false;
    }

    // Set up directories
    if (log_mkdir(CHROOT_PATH, 0700) < 0
            || log_mkdir(CHROOT_PATH "/dev", 0755) < 0
            || log_mkdir(CHROOT_PATH "/proc", 0755) < 0
            || log_mkdir(CHROOT_PATH "/sys", 0755) < 0
            || log_mkdir(CHROOT_PATH "/data", 0755) < 0
            || log_mkdir(CHROOT_PATH "/cache", 0755) < 0
            || log_mkdir(CHROOT_PATH "/system", 0755) < 0) {
        return false;
    }

    // Mount points
    if (log_mount("/dev", CHROOT_PATH "/dev", "", MS_BIND, "") < 0
            || log_mount("/proc", CHROOT_PATH "/proc", "", MS_BIND, "") < 0
            || log_mount("/sys", CHROOT_PATH "/sys", "", MS_BIND, "") < 0
            || log_mount("tmpfs", CHROOT_PATH "/data", "tmpfs", 0, "") < 0
            || log_mount("tmpfs", CHROOT_PATH "/cache", "tmpfs", 0, "") < 0
            || log_mount("/raw/system", CHROOT_PATH "/system", "",
                         MS_BIND | MS_RDONLY, "") < 0) {
        return false;
    }

    // Copy fstab file
    char fstab_src[128];
    char fstab_dst[128];

    std::string hardware;
    util::get_property("ro.hardware", &hardware, "");

    if (access(CRYPTO_FSTAB_PATH, R_OK) == 0) {
        LOGD("Using crypto fstab file");
        strcpy(fstab_src, CRYPTO_FSTAB_PATH);
    } else {
        LOGD("Using main fstab file");
        snprintf(fstab_src, sizeof(fstab_src),
                 "/fstab.%s", hardware.c_str());
    }
    snprintf(fstab_dst, sizeof(fstab_dst),
             CHROOT_PATH "/fstab.%s", hardware.c_str());

    util::copy_file(fstab_src, fstab_dst, util::COPY_ATTRIBUTES);

    return true;
}

static bool destroy_chroot()
{
    LOGV("Destroying chroot...");

    log_umount(CHROOT_PATH "/system");
    log_umount(CHROOT_PATH "/cache");
    log_umount(CHROOT_PATH "/data");
    log_umount(CHROOT_PATH "/sys");
    log_umount(CHROOT_PATH "/proc");
    log_umount(CHROOT_PATH "/dev");

    if (!log_unmount_all(CHROOT_PATH)) {
        return false;
    }

    if (!log_delete_recursive(CHROOT_PATH)) {
        return false;
    }

    return true;
}

static int wait_for_pid(const char *name, pid_t pid)
{
    int status;

    do {
        if (waitpid(pid, &status, 0) < 0) {
            LOGE("%s (pid: %d): Failed to waitpid(): %s",
                 name, pid, strerror(errno));
            return -1;
        }
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));

    if (WIFEXITED(status)) {
        LOGV("%s (pid: %d): Exited with status: %d",
             name, pid, WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        LOGV("%s (pid: %d): Killed by signal: %d",
             name, pid, WTERMSIG(status));
    }

    return status;
}

static bool start_setup_script()
{
    struct stat sb;
    if (stat(CRYPTO_SETUP_PATH, &sb) < 0) {
        LOGI("Setup script is missing. Assuming it's not necessary");
        return true;
    }

    if (setup_pid >= 0) {
        return true;
    }

    if (!util::copy_file(CRYPTO_SETUP_PATH, CHROOT_PATH "/setup", 0)) {
        LOGE("Failed to copy setup script to chroot: %s", strerror(errno));
        return false;
    }

    LOGV("Starting setup script...");

    if (pipe(setup_pipe) < 0) {
        LOGE("Failed to pipe: %s", strerror(errno));
        return false;
    }

    int pid = fork();
    if (pid == 0) {
        if (chroot(CHROOT_PATH) < 0) {
            LOGE("%s: Failed to chroot: %s", CHROOT_PATH, strerror(errno));
            _exit(EXIT_FAILURE);
        }

        if (chdir("/") < 0) {
            LOGE("%s: Failed to chdir: %s", "/", strerror(errno));
            _exit(EXIT_FAILURE);
        }

        dup2(setup_pipe[1], STDOUT_FILENO);
        dup2(setup_pipe[1], STDERR_FILENO);
        close(setup_pipe[0]);
        close(setup_pipe[1]);

        chmod("/setup", 0700);
        execl("/setup", "/setup", nullptr);
        LOGE("Failed to exec setup script: %s", strerror(errno));
        _exit(127);
    } else if (pid > 0) {
        LOGV("Started setup script (pid: %d)", pid);
        LOGV("Waiting for SIGSTOP or exit");

        close(setup_pipe[1]);
        setup_reader_pid = start_output_reader(
                setup_pipe[0], &command_output_cb, (void *) "Setup script");
        close(setup_pipe[0]);

        int status;
        if (waitpid(pid, &status, WUNTRACED) < 0) {
            LOGW("Failed to waidpid: %s", strerror(errno));
        } else if (WIFEXITED(status)) {
            mb::log::log(WEXITSTATUS(status) == 0
                         ? mb::log::LogLevel::Info
                         : mb::log::LogLevel::Error,
                         "Setup script exited with status code %d",
                         WEXITSTATUS(status));
            return WEXITSTATUS(status) == 0;
        } else if (WIFSIGNALED(status)) {
            LOGE("Setup script killed by signal %d", WTERMSIG(status));
        } else if (WIFSTOPPED(status)) {
            LOGV("Setup script sent SIGSTOP; sending SIGCONT");
            LOGV("Setup script will continue in the background");
            kill(pid, SIGCONT);
            setup_pid = pid;
            return true;
        }

        return false;
    } else {
        LOGE("Failed to fork: %s", strerror(errno));
        return false;
    }
}

static bool stop_setup_script()
{
    if (setup_pid < 0) {
        return true;
    }

    LOGV("Stopping setup script... (pid: %d)", setup_pid);

    // Clear pid when returning
    auto clear_pid = util::finally([]{
        setup_pid = -1;
    });

    kill(setup_pid, SIGTERM);

    return wait_for_pid("Setup script reader", setup_reader_pid) != -1
            && wait_for_pid("Setup script", setup_pid) != -1;
}

static bool start_voldwrapper()
{
    if (vold_pid >= 0) {
        return true;
    }

    LOGV("Starting voldwrapper...");

    if (pipe(vold_pipe) < 0) {
        LOGE("Failed to pipe: %s", strerror(errno));
        return false;
    }

    vold_pid = fork();
    if (vold_pid == 0) {
        if (chroot(CHROOT_PATH) < 0) {
            LOGE("%s: Failed to chroot: %s", CHROOT_PATH, strerror(errno));
            _exit(EXIT_FAILURE);
        }

        if (chdir("/") < 0) {
            LOGE("%s: Failed to chdir: %s", "/", strerror(errno));
            _exit(EXIT_FAILURE);
        }

        dup2(vold_pipe[1], STDOUT_FILENO);
        dup2(vold_pipe[1], STDERR_FILENO);
        close(vold_pipe[0]);
        close(vold_pipe[1]);

        execl("/proc/self/exe", "voldwrapper", nullptr);
        LOGE("Failed to exec voldwrapper: %s", strerror(errno));
        _exit(127);
    } else if (vold_pid > 0) {
        LOGV("Started voldwrapper (pid: %d)", vold_pid);

        close(vold_pipe[1]);
        vold_reader_pid = start_output_reader(
                vold_pipe[0], &command_output_cb, (void *) "voldwrapper");
        close(vold_pipe[0]);

        return true;
    } else {
        LOGE("Failed to fork: %s", strerror(errno));
        return false;
    }
}

static bool stop_voldwrapper()
{
    if (vold_pid < 0) {
        return true;
    }

    LOGV("Stopping voldwrapper... (pid: %d)", vold_pid);

    // Clear pid when returning
    auto clear_pid = util::finally([]{
        vold_pid = -1;
    });

    kill(vold_pid, SIGTERM);

    return wait_for_pid("voldwrapper reader", vold_reader_pid) != -1
            && wait_for_pid("voldwrapper", vold_pid) != -1;
}

bool decrypt_init()
{
    bool ret = create_chroot();
    if (!ret) {
        LOGE("Failed to initialize chroot");
        return false;
    }

    if (!start_setup_script()) {
        return false;
    }

    if (!start_voldwrapper()) {
        return false;
    }

    // Wait up to 10 seconds for vold
    VoldConnection conn;
    for (int i = 1; i <= MAX_CONNECTION_ATTEMPTS; ++i) {
        LOGD("Testing connection to vold (attempt %d/%d)",
             i, MAX_CONNECTION_ATTEMPTS);
        if (conn.connect()) {
            break;
        } else {
            LOGW("Connection failed. Retrying in one second...");
            sleep(1);
        }
    }

    return true;
}

bool decrypt_cleanup()
{
    bool ret = true;

    if (!stop_voldwrapper()) {
        ret = false;
    }

    if (!stop_setup_script()) {
        ret = false;
    }

    if (!destroy_chroot()) {
        LOGE("Failed to clean up chroot");
        ret = false;
    }

    return ret;
}

const char * decrypt_get_pw_type()
{
    VoldEvent event;
    VoldConnection conn;

    if (!conn.connect()) {
        LOGE("Failed to connect to vold");
        return nullptr;
    }

    if (!conn.execute_command({ "cryptfs", "getpwtype" }, &event)) {
        LOGE("Failed to execute vold command");
        return nullptr;
    }

    VoldResponse code = static_cast<VoldResponse>(event.code);
    const char *code_str = vold_response_str(code);

    LOGD("Vold 'getpwtype' response code: %d (%s)",
         event.code, code_str ? code_str : "<unknown>");

    if (code == VoldResponse::PasswordTypeResult) {
        LOGD("Vold daemon supports 'getpwtype' command");

        if (event.message == CRYPTFS_PW_TYPE_DEFAULT) {
            return CRYPTFS_PW_TYPE_DEFAULT;
        } else if (event.message == CRYPTFS_PW_TYPE_PASSWORD) {
            return CRYPTFS_PW_TYPE_PASSWORD;
        } else if (event.message == CRYPTFS_PW_TYPE_PATTERN) {
            return CRYPTFS_PW_TYPE_PATTERN;
        } else if (event.message == CRYPTFS_PW_TYPE_PIN) {
            return CRYPTFS_PW_TYPE_PIN;
        } else {
            LOGW("Unknown password type: '%s'", event.message.c_str());
        }
    } else if (code == VoldResponse::CommandSyntaxError) {
        LOGD("Vold daemon does not support 'getpwtype' command");
        LOGD("Assuming password type is 'password'");

        return CRYPTFS_PW_TYPE_PASSWORD;
    }

    return nullptr;
}

bool decrypt_userdata(const char *password)
{
    VoldEvent event;
    VoldConnection conn;

    if (!conn.connect()) {
        LOGE("Failed to connect to vold");
        return false;
    }

    if (!conn.execute_command({ "cryptfs", "checkpw", password }, &event)) {
        LOGE("Failed to execute vold command");
        return false;
    }

    VoldResponse code = static_cast<VoldResponse>(event.code);
    const char *code_str = vold_response_str(code);

    LOGD("Vold 'checkpw' response code: %d (%s)",
         event.code, code_str ? code_str : "<unknown>");

    int ret;
    if (util::str_to_snum(event.message.c_str(), 10, &ret)) {
        if (ret == 0) {
            LOGD("Vold successully decrypted device");
            return true;
        } else if (ret < 0) {
            LOGE("Vold failed to decrypt device");
        } else {
            LOGW("Vold reports %d incorrect password attempts", ret);
        }
    } else {
        LOGE("Invalid message in vold event");
    }

    // Do something
    return false;
}

static void decrypt_usage(FILE *stream)
{
    fprintf(stream,
            "Usage: decrypt [OPTION]... <password>\n\n"
            "Options:\n"
            "  -h, --help       Display this help message\n");
}

int decrypt_main(int argc, char *argv[])
{
    int opt;

    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, "h", long_options, &long_index)) != -1) {
        switch (opt) {
        case 'h':
            decrypt_usage(stdout);
            return EXIT_SUCCESS;

        default:
            decrypt_usage(stderr);
            return EXIT_FAILURE;
        }
    }

    if (argc - optind != 1) {
        decrypt_usage(stderr);
        return EXIT_FAILURE;
    }

    const char *password = argv[optind];

    bool ret = decrypt_init();
    if (!ret) {
        fprintf(stderr, "Failed to initialize\n");
    } else {
        const char *password_type = decrypt_get_pw_type();
        printf("Detected password type: %s\n",
               password_type ? password_type : "<unknown>");

        ret = decrypt_userdata(password);
        if (ret) {
            printf("successully decrypted userdata\n");
        } else {
            fprintf(stderr, "Failed to decrypt userdata\n");
        }
    }

    if (!decrypt_cleanup()) {
        fprintf(stderr, "Failed to clean up\n");
    }

    return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}

}
