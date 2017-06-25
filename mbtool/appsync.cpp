/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "appsync.h"

#include <algorithm>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <fcntl.h>
#include <getopt.h>
#include <poll.h>
#include <signal.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include <jansson.h>

#include "mbcommon/common.h"
#include "mbcommon/string.h"
#include "mbcommon/version.h"
#include "mblog/logging.h"
#include "mblog/stdio_logger.h"
#include "mbutil/autoclose/file.h"
#include "mbutil/chown.h"
#include "mbutil/command.h"
#include "mbutil/copy.h"
#include "mbutil/delete.h"
#include "mbutil/directory.h"
#include "mbutil/file.h"
#include "mbutil/finally.h"
#include "mbutil/fts.h"
#include "mbutil/properties.h"
#include "mbutil/selinux.h"
#include "mbutil/socket.h"
#include "mbutil/string.h"
#include "mbutil/time.h"

#include "appsyncmanager.h"
#include "multiboot.h"
#include "packages.h"
#include "romconfig.h"
#include "roms.h"

#define ANDROID_SOCKET_ENV_PREFIX       "ANDROID_SOCKET_"
#define ANDROID_SOCKET_DIR              "/dev/socket"

#define INSTALLD_PATH                   "/system/bin/installd"
#define SOCKET_PATH                     "installd"

// Original init.rc definition for the socket is:
//    socket installd stream 600 system system
#define INSTALLD_SOCKET_PATH            ANDROID_SOCKET_DIR "/installd.real"
#define INSTALLD_SOCKET_UID             1000 // AID_SYSTEM
#define INSTALLD_SOCKET_GID             1000 // AID_SYSTEM
#define INSTALLD_SOCKET_PERMS           0600
#define INSTALLD_SOCKET_CONTEXT         "u:object_r:installd_socket:s0"

#define COMMAND_BUF_SIZE                1024

#define PACKAGES_XML_PATH_FMT           "%s/system/packages.xml"

namespace mb
{

static RomConfig config;
static Packages packages;

static std::vector<RomConfigAndPackages> cfg_pkgs_list; // 'dat naming tho ;)

/*!
 * \brief Try loading the config file in /data/media/0/MultiBoot/[ROM ID]/config.json
 */
static bool load_config_files()
{
    auto current_rom = Roms::get_current_rom();
    if (!current_rom) {
        LOGE("Failed to determine current ROM");
        return false;
    }

    Roms roms;
    roms.add_installed();

    for (const std::shared_ptr<Rom> &rom : roms.roms) {
        std::string config_path = rom->config_path();
        char *packages_path = mb_format(PACKAGES_XML_PATH_FMT,
                                        rom->full_data_path().c_str());
        if (!packages_path) {
            LOGW("Out of memory");
            continue;
        }

        cfg_pkgs_list.emplace_back();
        cfg_pkgs_list.back().rom = rom;

        RomConfig &rom_config = cfg_pkgs_list.back().config;
        Packages &rom_packages = cfg_pkgs_list.back().packages;

        if (!rom_config.load_file(config_path)) {
            LOGW("%s: Failed to load config for ROM %s",
                 config_path.c_str(), rom->id.c_str());
        }
        if (!rom_packages.load_xml(packages_path)) {
            LOGW("%s: Failed to load packages for ROM %s",
                 packages_path, rom->id.c_str());
        }

        if (rom->id == current_rom->id) {
            config = rom_config;
            packages = rom_packages;
        }

        free(packages_path);
    }

    LOGD("[Config] ROM ID:                    %s", config.id.c_str());
    LOGD("[Config] ROM Name:                  %s", config.name.c_str());
    LOGD("[Config] Individual app sharing:    %s",
         config.indiv_app_sharing ? "true" : "false");

    for (const SharedPackage &pkg : config.shared_pkgs) {
        LOGD("[Config] Shared package:");
        LOGD("[Config] - Package:                 %s", pkg.pkg_id.c_str());
        LOGD("[Config] - Share data:              %s",
             pkg.share_data ? "true" : "false");
    }

    return true;
}

static bool prepare_appsync()
{
    // Detect directory locations
    AppSyncManager::detect_directories();

    if (!AppSyncManager::initialize_directories()) {
        return false;
    }

    uint64_t start = util::current_time_ms(), stop;

    for (auto it = config.shared_pkgs.begin();
            it != config.shared_pkgs.end();) {
        SharedPackage &shared_pkg = *it;

        // Ensure package is installed, so we can get its UID
        auto pkg = packages.find_by_pkg(shared_pkg.pkg_id);
        if (!pkg) {
            LOGW("Package %s won't be shared because it is not installed",
                 shared_pkg.pkg_id.c_str());
            it = config.shared_pkgs.erase(it);
            continue;
        }

        // Ensure that the data directory exists if data sharing is enabled
        if (shared_pkg.share_data
                && !AppSyncManager::create_shared_data_directory(
                        shared_pkg.pkg_id, pkg->get_uid())) {
            LOGW("Failed to create shared data directory for package %s. "
                 "App data will not be shared", shared_pkg.pkg_id.c_str());
            shared_pkg.share_data = false;
        }

        ++it;
    }

    // Ensure that the shared data is under the u:object_r:app_data_file:s0
    // context. Otherwise, apps won't be able to write to the shared directory
    bool disable_data_sharing = false;
    if (!AppSyncManager::fix_shared_data_permissions()) {
        LOGW("Failed to fix permissions on shared data directory");
        LOGW("Data sharing will be disabled for all packages");
        disable_data_sharing = true;
    }

    stop = util::current_time_ms();
    LOGD("Initialization stage 1 took %" PRIu64 "ms", stop - start);

    start = util::current_time_ms();

    // Actually share the data
    for (SharedPackage &shared_pkg : config.shared_pkgs) {
        auto pkg = packages.find_by_pkg(shared_pkg.pkg_id);

        if (disable_data_sharing) {
            shared_pkg.share_data = false;
        }

        if (shared_pkg.share_data && !AppSyncManager::mount_shared_directory(
                pkg->name, pkg->get_uid())) {
            LOGW("Failed to mount shared data directory");
            shared_pkg.share_data = false;
        }
    }

    stop = util::current_time_ms();
    LOGD("Initialization stage 2 took %" PRIu64 "ms", stop - start);

    return true;
}

/*!
 * \brief Get installd socket fd created by init from ANDROID_SOCKET_installd
 *
 * \return The fd if the environment variable was set. Otherwise, -1
 */
static int get_socket_from_env(const char *name)
{
    char key[64] = ANDROID_SOCKET_ENV_PREFIX;
    strlcat(key, name, sizeof(key));

    const char *value = getenv(key);
    if (!value) {
        return -1;
    }

    errno = 0;
    int fd = strtol(value, nullptr, 10);
    if (errno) {
        return -1;
    }

    return fd;
}

/*!
 * \brief Set ANDROID_SOCKET_installd to the new installd socket fd
 */
static void put_socket_to_env(const char *name, int fd)
{
    char key[64] = ANDROID_SOCKET_ENV_PREFIX;
    char value[64];

    strlcat(key, name, sizeof(key));
    snprintf(value, sizeof(value), "%d", fd);

    setenv(key, value, 1);
}

/*!
 * \brief Create a new socket for installd to listen on
 */
static int create_new_socket()
{
    struct sockaddr_un addr;
    int fd;

    fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (fd < 0) {
        LOGE("Failed to create socket: %s", strerror(errno));
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_LOCAL;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s",
             INSTALLD_SOCKET_PATH);

    if (unlink(addr.sun_path) < 0 && errno != ENOENT) {
        LOGE("Failed to unlink old socket: %s", strerror(errno));
        close(fd);
        return -1;
    }

    if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        LOGE("Failed to bind socket: %s", strerror(errno));
        unlink(addr.sun_path);
        close(fd);
        return -1;
    }

    chown(addr.sun_path, INSTALLD_SOCKET_UID, INSTALLD_SOCKET_GID);
    chmod(addr.sun_path, INSTALLD_SOCKET_PERMS);
    util::selinux_set_context(addr.sun_path, INSTALLD_SOCKET_CONTEXT);

    // Make sure close-on-exec is cleared to the fd remains open across execve()
    fcntl(fd, F_SETFD, 0);

    return fd;
}

/*
 * Socket messages are prefixed with 16-bit unsigned value (little-endian)
 * indicating the number of bytes that follow. The data should be treated as
 * a string and a null terminator must be added to the end.
 */

/*!
 * \brief Receive a message from a socket
 */
static bool receive_message(int fd, char *buf, std::size_t size,
                            bool is_async, int *async_id)
{
    unsigned short count;

    if (is_async) {
        assert(async_id != nullptr);

        if (!util::socket_read_int32(fd, async_id)) {
            LOGE("Failed to receive async command ID: %s", strerror(errno));
            return false;
        }
    }

    if (!util::socket_read_uint16(fd, &count)) {
        LOGE("Failed to read command size: %s", strerror(errno));
        return false;
    }

    if (count < 1 || count >= size) {
        LOGE("Invalid size %u", count);
        return false;
    }

    if (util::socket_read(fd, buf, count) != count) {
        LOGE("Failed to read command: %s", strerror(errno));
        return false;
    }

    buf[count] = 0;

    return true;
}

/*!
 * \brief Send a message to a socket
 */
static bool send_message(int fd, const char *command,
                         bool is_async, int async_id)
{
    unsigned short count = strlen(command);

    if (is_async) {
        if (!util::socket_write_int32(fd, async_id)) {
            LOGE("Failed to write async command ID: %s", strerror(errno));
            return false;
        }
    }

    if (!util::socket_write_uint16(fd, count)) {
        LOGE("Failed to write command size: %s", strerror(errno));
        return false;
    }

    if (util::socket_write(fd, command, count) != count) {
        LOGE("Failed to write command: %s", strerror(errno));
        return false;
    }

    return true;
}

/*!
 * \brief Connect to the installd socket at INSTALLD_SOCKET_PATH
 *
 * Because installd is spawned right before this function is called, this
 * function will make 5 attempts to connect to the socket. If an attempt fails,
 * it will wait 1 second before trying again.
 *
 * \return fd if the connection succeeds. Otherwise, -1
 */
static int connect_to_installd()
{
    struct sockaddr_un addr;

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_LOCAL;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s",
             INSTALLD_SOCKET_PATH);

    int fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (fd < 0) {
        LOGE("Failed to create socket: %s", strerror(errno));
        return -1;
    }

    int attempt;
    for (attempt = 0; attempt < 5; ++attempt) {
        LOGV("Connecting to installd [Attempt %d/%d]", attempt + 1, 5);
        if (connect(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
            LOGW("Failed: %s", strerror(errno));
            sleep(1);
        } else {
            break;
        }
    }

    if (attempt == 5) {
        LOGD("Failed to connect to installd after 5 attempts");
        close(fd);
        return -1;
    }

    LOGD("Connected to installd");

    return fd;
}

/*!
 * \brief Run installd
 *
 * This will fork and run /system/bin/installd.
 *
 * \return installd's pid if it was successfully launched. Otherwise -1
 */
static pid_t spawn_installd()
{
    // Spawn installd
    pid_t pid = fork();
    if (pid == 0) {
        const char *argv[] = { INSTALLD_PATH, nullptr };
        execve(argv[0], const_cast<char * const *>(argv), environ);
        LOGE("Failed to launch installd: %s", strerror(errno));
        _exit(EXIT_FAILURE);
    } else if (pid < 0) {
        LOGD("Failed to fork: %s", strerror(errno));
        return -1;
    }
    return pid;
}

static std::vector<std::string> parse_args(const char *cmdline)
{
    std::vector<char> buf(cmdline, cmdline + strlen(cmdline) + 1);
    std::vector<std::string> args;

    char *temp;
    char *token;

    token = strtok_r(buf.data(), " ", &temp);
    while (token) {
        args.push_back(token);

        token = strtok_r(nullptr, " ", &temp);
    }

    return args;
}

static std::string args_to_string(const std::vector<std::string> &args)
{
    std::string output("[");
    for (size_t i = 0; i < args.size(); ++i) {
        if (i != 0) {
            output += ", ";
        }
        output += args[i];
    }
    output += "]";
    return output;
}

static bool do_remove(const std::vector<std::string> &args)
{
#define TAG "[remove] "
    const std::string &pkgname = args[0];
    MB_UNUSED
    const int userid = strtol(args[1].c_str(), nullptr, 10);

    for (auto it = config.shared_pkgs.begin();
            it != config.shared_pkgs.end(); ++it) {
        const SharedPackage &shared_pkg = *it;

        if (shared_pkg.pkg_id != pkgname) {
            continue;
        }

        // Need to remove from the shared_pkgs list to prevent the linklib hook
        // from triggering if the user decides to reinstall a package that was
        // previously shared.
        config.shared_pkgs.erase(it);

        if (shared_pkg.share_data) {
            // If data is shared, make sure the data directory is unmounted
            // before Android wipes it clean
            LOGV(TAG "Attempting to unmount shared data directory");
            if (!AppSyncManager::unmount_shared_directory(pkgname)) {
                return false;
            }
        } else {
            LOGD(TAG "Data is not shared");
        }

        return true;
    }

    LOGD(TAG "Package is not shared");
    return true;
#undef TAG
}

struct CommandInfo {
    const char *name;
    unsigned int nargs;
    bool (*func)(const std::vector<std::string> &args);
};

static struct CommandInfo cmds[] = {
    { "remove",  2, do_remove }
};

static void handle_command(const std::vector<std::string> &args)
{
    for (std::size_t i = 0; i < sizeof(cmds) / sizeof(cmds[0]); ++i) {
        if (args[0] == cmds[i].name) {
            if (args.size() - 1 != cmds[i].nargs) {
                LOGE("%s requires %u arguments (%zu given)",
                     cmds[i].name, cmds[i].nargs, args.size() - 1);
                LOGE("%s command won't be hooked", cmds[i].name);
            } else {
                LOGD("Hooking %s command", cmds[i].name);
                cmds[i].func(std::vector<std::string>(
                        args.begin() + 1, args.end()));
            }
        }
    }
}

static bool handle_installd_event(int client_fd, int installd_fd,
                                  bool is_async)
{
    // Use the same buffer size as installd
    char buf[COMMAND_BUF_SIZE];

    uint64_t time_start, time_stop;
    uint64_t time_start_installd, time_stop_installd;
    uint64_t time_start_send, time_stop_send;

    int async_id;

    time_start = util::current_time_ms();

    time_start_installd = util::current_time_ms();
    if (!receive_message(
            installd_fd, buf, sizeof(buf), is_async, &async_id)) {
        LOGE("Failed to receive reply from installd");
        return false;
    }
    time_stop_installd = util::current_time_ms();

    std::vector<std::string> args = parse_args(buf);
    LOGD("Received async (probably) reply: %s", args_to_string(args).c_str());

    time_start_send = util::current_time_ms();
    if (!send_message(client_fd, buf, is_async, async_id)) {
        LOGE("Failed to send reply to client");
        return false;
    }
    time_stop_send = util::current_time_ms();

    time_stop = util::current_time_ms();

    LOGD("Command stats:");
    LOGD("- Time to send result back to client:  %" PRIu64 "ms",
         time_stop_send - time_start_send);
    LOGD("- Time to receive reply from installd: %" PRIu64 "ms",
         time_stop_installd - time_start_installd);
    LOGD("- Time to complete entire proxy logic: %" PRIu64 "ms",
         time_stop - time_start);
    LOGD("---");

    return true;
}

static bool handle_android_event(int client_fd, int installd_fd,
                                 bool can_appsync, bool is_async)
{
    // Use the same buffer size as installd
    char buf[COMMAND_BUF_SIZE];

    uint64_t time_start, time_stop;
    uint64_t time_start_installd, time_stop_installd;
    uint64_t time_start_hook = 0, time_stop_hook = 0;
    uint64_t time_start_send, time_stop_send;

    int async_id;

    if (!receive_message(client_fd, buf, sizeof(buf), is_async, &async_id)) {
        LOGE("Failed to receive request from client");
        return false;
    }

    time_start = util::current_time_ms();

    std::vector<std::string> args = parse_args(buf);

    bool log_result = true;

    if (args.empty()) {
        LOGE("Invalid command (empty message)");
    } else {
        const std::string &cmd = args[0];

        if (cmd == "ping"
                || cmd == "freecache") {
            LOGD("Received unimportant command: [%s, ...]", cmd.c_str());
        } else if (cmd == "aapt"
                || cmd == "aapt_with_common") {
            LOGD("Received CyanogenMod-specific command: %s",
                 args_to_string(args).c_str());
        } else if (cmd == "rmrcl"
                || cmd == "asyncDexopt"
                || cmd == "changeDexOwner") {
            LOGD("Received Touchwiz-specific command: %s",
                 args_to_string(args).c_str());
            if (cmd == "asyncDexopt") {
                LOGD("Expecting future installd reply for 'asyncDexopt'");
            }
        } else if (cmd == "getsize") {
            // Get size is so annoying we don't want it to show... EVER!
            log_result = false;
        } else if (cmd == "install"
                || cmd == "dexopt"
                || cmd == "markbootcomplete"
                || cmd == "movedex"
                || cmd == "rmdex"
                || cmd == "remove"
                || cmd == "rename"
                || cmd == "fixuid"
                || cmd == "rmcache"
                || cmd == "rmcodecache"
                || cmd == "rmuserdata"
                || cmd == "movefiles"
                || cmd == "linklib"
                || cmd == "mkuserdata"
                || cmd == "mkuserconfig"
                || cmd == "rmuser"
                || cmd == "idmap"
                || cmd == "restorecondata"
                || cmd == "patchoat") {
            LOGD("Received command: %s", args_to_string(args).c_str());

            if (can_appsync) {
                time_start_hook = util::current_time_ms();
                handle_command(args);
                time_stop_hook = util::current_time_ms();
            }
        } else {
            LOGW("Unrecognized command: %s", args_to_string(args).c_str());
        }
    }

    time_start_installd = util::current_time_ms();
    if (!send_message(installd_fd, buf, is_async, async_id)) {
        LOGE("Failed to send request to installd");
        return false;
    }
    if (!receive_message(
            installd_fd, buf, sizeof(buf), is_async, &async_id)) {
        LOGE("Failed to receive reply from installd");
        return false;
    }
    time_stop_installd = util::current_time_ms();

    args = parse_args(buf);

    if (log_result) {
        LOGD("Sending reply: %s", args_to_string(args).c_str());
    }

    time_start_send = util::current_time_ms();
    if (!send_message(client_fd, buf, is_async, async_id)) {
        LOGE("Failed to send reply to client");
        return false;
    }
    time_stop_send = util::current_time_ms();

    time_stop = util::current_time_ms();

    if (log_result) {
        LOGD("Command stats:");
        LOGD("- Time to send result back to client:  %" PRIu64 "ms",
             time_stop_send - time_start_send);
        LOGD("- Time to complete installd command:   %" PRIu64 "ms",
             time_stop_installd - time_start_installd);
        if (can_appsync) {
            LOGD("- Time to hook installd command:       %" PRIu64 "ms",
                 time_stop_hook - time_start_hook);
        }
        LOGD("- Time to complete entire proxy logic: %" PRIu64 "ms",
             time_stop - time_start);
        LOGD("---");
    }

    return true;
}

/**
 * \brief Main function for capturing and relaying the daemon commands
 *
 * This function will not return under normal conditions. It runs through the
 * following loop infinitely:
 *
 * 1. Accept a connection on the original installd socket
 * 2. Run installd
 * 3. Connect to the installd
 * 4. Capture and proxy commands
 *
 * If installd crashes or connection between mbtool and installd breaks in some
 * way, it will simply loop again. If this function fails to accept a connection
 * on the original socket, then it will return false.
 *
 * \return False if accepting the socket connection fails. Otherwise, does not
 *         return
 */
static bool proxy_process(int fd, bool can_appsync)
{
    while (true) {
        int client_fd = accept(fd, nullptr, nullptr);
        if (client_fd < 0) {
            LOGE("Failed to accept client connection: %s", strerror(errno));
            return false;
        }

        auto close_client_fd = util::finally([&]{
            LOGD("Closing client connection");
            close(client_fd);
        });

        LOGD("Accepted new client connection");

        // Connect to installd
        int installd_fd = connect_to_installd();
        if (installd_fd < 0) {
            return false;
        }

        auto close_installd_fd = util::finally([&]{
            LOGD("Closing installd connection");
            close(installd_fd);
        });

        // Check if we're using some variant of the CyanogenMood async installd
        // NOTE: We'll effectively make the connection sychronous because we
        //       always wait for a reply before receiving the next command.
        // See: https://github.com/CyanogenMod/android_frameworks_native/commit/8124b181d4b5a3a44796fdb0e3ea4e4171f102c7
        bool is_async = util::file_find_one_of(
                INSTALLD_PATH, { "failed to read transaction id" });
        LOGD("installd is CyanogenMod async version: %d", is_async);

        LOGD("---");

        struct pollfd fds[2];
        memset(fds, 0, sizeof(fds));

        fds[0].fd = client_fd;
        fds[0].events = POLLIN;
        fds[1].fd = installd_fd;
        fds[1].events = POLLIN;

        while (true) {
            fds[0].revents = 0;
            fds[1].revents = 1;

            if (poll(fds, 2, -1) < 0) {
                LOGE("Failed to poll() fds: %s", strerror(errno));
                break;
            }

            if (fds[0].revents & POLLIN && !handle_android_event(
                    client_fd, installd_fd, can_appsync, is_async)) {
                break;
            }
            if (fds[1].revents & POLLIN && !handle_installd_event(
                    client_fd, installd_fd, is_async)) {
                break;
            }
        }
    }

    // Not reached
    return true;
}

/*!
 * \brief Set up the environment for proxying the installd socket
 */
static bool hijack_socket(bool can_appsync)
{
    LOGD("Starting appsync");

    int orig_fd = get_socket_from_env(SOCKET_PATH);
    if (orig_fd < 0) {
        LOGE("Failed to get socket fd from environment");
        return false;
    }

    LOGD("Got socket fd from environment: %d", orig_fd);

    fcntl(orig_fd, F_SETFD, FD_CLOEXEC);

    // Listen on the original socket
    if (listen(orig_fd, 5) < 0) {
        LOGE("Failed to listen on socket: %s", strerror(errno));
        return false;
    }

    // Create a new socket for installd
    int new_fd = create_new_socket();
    if (new_fd < 0) {
        LOGE("Failed to create new socket for installd");
        return false;
    }

    auto close_new_fd = util::finally([&]{
        close(new_fd);
    });

    LOGD("Putting new socket fd for installd to environment: %d", new_fd);

    // Put the new fd in the environment
    put_socket_to_env(SOCKET_PATH, new_fd);

    LOGD("Launching installd");
    pid_t pid = spawn_installd();
    if (pid < 0) {
        return false;
    }

    // Kill installd when we exit
    auto kill_installd = util::finally([&]{
        kill(pid, SIGINT);

        int status;

        do {
            if (waitpid(pid, &status, 0) < 0) {
                LOGE("Failed to waitpid(): %s", strerror(errno));
                break;
            }
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    });

    LOGD("Ready! Waiting for connections");

    // Start processing commands!
    if (!proxy_process(orig_fd, can_appsync)) {
        return false;
    }

    return true;
}

static void appsync_usage(bool error)
{
    FILE *stream = error ? stderr : stdout;

    fprintf(stream,
            "Usage: appsync [OPTION]...\n\n"
            "Options:\n"
            "  -h, --help    Display this help message\n"
            "\n"
            "This tool implements app data sharing by hijacking the connection\n"
            "between the Android Java framework and the installd daemon. It captures the\n"
            "incoming messages and performs the needed operations. Messages between the\n"
            "framework and the installd daemon are not altered in any way, though this may\n"
            "change in the future.\n"
            "\n"
            "To use this tool, the installd definition in /init.rc should be modified to\n"
            "point to mbtool. For example, the following original definition:\n"
            "\n"
            "    service installd /system/bin/installd\n"
            "        class main\n"
            "        socket installd stream 600 system system\n"
            "\n"
            "would be changed to the following:\n"
            "\n"
            "    service installd /appsync\n"
            "        class main\n"
            "        socket installd stream 600 system system\n"
            "\n"
            "It is not necessary to keep the installd definition because appsync will run\n"
            "/system/bin/installd automatically.\n");
}

int appsync_main(int argc, char *argv[])
{
    int opt;

    static struct option long_options[] = {
        {"help",   no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, "h", long_options, &long_index)) != -1) {
        switch (opt) {
        case 'h':
            appsync_usage(false);
            return EXIT_SUCCESS;

        default:
            appsync_usage(true);
            return EXIT_FAILURE;
        }
    }

    // There should be no other arguments
    if (argc - optind != 0) {
        appsync_usage(true);
        return EXIT_FAILURE;
    }


    if (!util::mkdir_parent(MULTIBOOT_LOG_APPSYNC, 0775) && errno != EEXIST) {
        fprintf(stderr, "Failed to create parent directory of %s: %s\n",
                MULTIBOOT_LOG_APPSYNC, strerror(errno));
        return EXIT_FAILURE;
    }

    autoclose::file fp(autoclose::fopen(MULTIBOOT_LOG_APPSYNC, "we"));
    if (!fp) {
        fprintf(stderr, "Failed to open log file %s: %s\n",
                MULTIBOOT_LOG_APPSYNC, strerror(errno));
        return EXIT_FAILURE;
    }

    fix_multiboot_permissions();

    // mbtool logging
    log::log_set_logger(std::make_shared<log::StdioLogger>(fp.get(), true));

    LOGI("=== APPSYNC VERSION %s ===", version());

    LOGI("Calling restorecon on /data/media/obb");
    const char *restorecon[] =
            { "restorecon", "-R", "-F", "/data/media/obb", nullptr };
    util::run_command(restorecon[0], restorecon, nullptr, nullptr, nullptr,
                      nullptr);

    bool can_appsync = false;

    // Try to load config file
    if (!load_config_files()) {
        LOGW("Failed to load configuration file; app sharing will not work");
        LOGW("Continuing to proxy installd anyway...");
    } else {
        if (config.indiv_app_sharing) {
            uint64_t start = util::current_time_ms();
            can_appsync = prepare_appsync();
            uint64_t stop = util::current_time_ms();
            if (!can_appsync) {
                LOGW("appsync preparation failed. "
                     "App sharing is completely disabled");
            }
            LOGD("Entire appsync preparation took %" PRIu64 "ms", stop - start);
        }
    }

    return hijack_socket(can_appsync) ? EXIT_SUCCESS : EXIT_FAILURE;
}

}
