/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "appsync.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include <jansson.h>

#include "apk.h"
#include "packages.h"
#include "roms.h"
#include "util/chown.h"
#include "util/copy.h"
#include "util/delete.h"
#include "util/directory.h"
#include "util/finally.h"
#include "util/fts.h"
#include "util/logging.h"
#include "util/selinux.h"
#include "util/socket.h"
#include "util/string.h"

#define LOG_FILE                        "/sdcard/MultiBoot/appsync.log"

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

#define CONFIG_PATH_FMT                 "/data/media/0/MultiBoot/{}/config.json"

#define CONFIG_KEY_APP_SHARING          "app_sharing"
#define CONFIG_KEY_GLOBAL_APP_SHARING   "global"
#define CONFIG_KEY_GLOBAL_APP_SHARING_PAID "global_paid"
#define CONFIG_KEY_INDIVIDUAL_APP_SHARING "individual"
#define CONFIG_KEY_PACKAGES             "packages"
#define CONFIG_KEY_PACKAGE_ID           "pkg_id"
#define CONFIG_KEY_SHARE_APK            "share_apk"
#define CONFIG_KEY_SHARE_DATA           "share_data"

#define PACKAGES_XML                    "/data/system/packages.xml"

#define APP_DATA_SELINUX_CONTEXT        "u:object_r:app_data_file:s0"

#define APP_SHARING_APP_DIR             "/data/multiboot/_appsharing/app"
#define APP_SHARING_APP_ASEC_DIR        "/data/multiboot/_appsharing/app-asec"
#define APP_SHARING_DATA_DIR            "/data/multiboot/_appsharing/data"

#define USER_APP_DIR                    "/data/app"
#define USER_APP_ASEC_DIR               "/data/app-asec"
#define USER_DATA_DIR                   "/data/data"

namespace mb
{

struct SharedPackage {
    std::string pkg_id;
    bool share_apk;
    bool share_data;
};

static std::vector<std::shared_ptr<SharedPackage>> shared_pkgs;
bool app_sharing_global = false;
bool app_sharing_global_paid = false;
bool app_sharing_individual = false;

/*!
 * \brief Try loading the config file in /data/media/0/MultiBoot/[ROM ID]/config.json
 */
static bool load_config_file()
{
    auto rom = Roms::get_current_rom();
    if (!rom) {
        LOGE("Failed to determine current ROM");
        return false;
    }

    std::string path = fmt::format(CONFIG_PATH_FMT, rom->id);

    json_t *root;
    json_error_t error;

    root = json_load_file(path.c_str(), 0, &error);
    if (!root) {
        LOGE("JSON error on line {:d}: {}", error.line, error.text);
        return false;
    }

    auto free_on_return = util::finally([&]{
        json_decref(root);
    });

    if (!json_is_object(root)) {
        LOGE("JSON root is not an object");
        return false;
    }

    json_t *app_sharing = json_object_get(root, CONFIG_KEY_APP_SHARING);
    if (!json_is_object(app_sharing)) {
        LOGI("ROM config does not contain app sharing configuration");
        return true;
    }

    // Global app sharing
    json_t *global = json_object_get(
            app_sharing, CONFIG_KEY_GLOBAL_APP_SHARING);
    app_sharing_global = json_is_true(global);

    // Global paid app sharing
    json_t *global_paid = json_object_get(
            app_sharing, CONFIG_KEY_GLOBAL_APP_SHARING_PAID);
    app_sharing_global_paid = global_paid;

    // Individual app sharing
    json_t *individual = json_object_get(
            app_sharing, CONFIG_KEY_INDIVIDUAL_APP_SHARING);
    app_sharing_individual = individual;

    // Individual app sharing packages
    json_t *pkgs = json_object_get(
            individual, CONFIG_KEY_PACKAGES);
    if (!json_is_array(pkgs)) {
        LOGE("JSON packages object is not an array");
        return false;
    }

    for (size_t i = 0; i < json_array_size(pkgs); ++i) {
        json_t *data = json_array_get(pkgs, i);
        if (!json_is_object(data)) {
            LOGE("JSON packages array element {} is not an object", i);
            return false;
        }

        json_t *pkg_id = json_object_get(data, CONFIG_KEY_PACKAGE_ID);
        if (!json_is_string(pkg_id)) {
            LOGE("JSON package ID is not a string");
            return false;
        }

        json_t *share_apk = json_object_get(data, CONFIG_KEY_SHARE_APK);
        json_t *share_data = json_object_get(data, CONFIG_KEY_SHARE_DATA);

        std::shared_ptr<SharedPackage> pkg(new SharedPackage());
        pkg->pkg_id = json_string_value(pkg_id);
        pkg->share_apk = json_is_true(share_apk);
        pkg->share_data = json_is_true(share_data);

        LOGD("Shared package:");
        LOGD("- Package:    {}", pkg->pkg_id);
        LOGD("- Share apk:  {}", pkg->share_apk ? "true" : "false");
        LOGD("- Share data: {}", pkg->share_data ? "true" : "false");

        shared_pkgs.push_back(std::move(pkg));
    }

    return true;
}

class FixPermissions : public util::FTSWrapper {
public:
    FixPermissions(std::string path)
        : FTSWrapper(path, FTS_GroupSpecialFiles)
    {
    }

    virtual int on_changed_path() override
    {
        util::chown(_curr->fts_accpath, "system", "system", 0);
        return Action::FTS_OK;
    }

    virtual int on_reached_file() override
    {
        chmod(_curr->fts_accpath, 0644);
        return Action::FTS_OK;
    }

    virtual int on_reached_directory_pre() override
    {
        chmod(_curr->fts_accpath, 0755);
        return Action::FTS_OK;
    }
};

static std::string get_shared_apk_path(const std::string &pkg)
{
    std::string path(APP_SHARING_APP_DIR);
    path += "/";
    path += pkg;
    path += "/";
    path += "base.apk";
    return path;
}

static std::string get_shared_data_path(const std::string &pkg)
{
    std::string path(APP_SHARING_DATA_DIR);
    path += "/";
    path += pkg;
    return path;
}

static bool chown_wrapper(const std::string &path,
                          const std::shared_ptr<Package> &pkg,
                          bool recursive)
{
    if (pkg->is_shared_user) {
        return util::chown(path, pkg->shared_user_id, pkg->shared_user_id,
                           recursive ? util::MB_CHOWN_RECURSIVE : 0);
    } else {
        return util::chown(path, pkg->user_id, pkg->user_id,
                           recursive ? util::MB_CHOWN_RECURSIVE : 0);
    }
}

/*!
 * \brief Copy the apk from USER_APP_DIR to APP_SHARING_APP_DIR
 */
static bool ensure_apk_exists_in_app_sharing_dir(const std::string &pkg)
{
    std::string full_path = get_shared_apk_path(pkg);

    struct stat sb;
    if (stat(full_path.c_str(), &sb) == 0 && S_ISREG(sb.st_mode)) {
        return true;
    }

    LOGD("Package {} does not exist in the shared app directory. "
         "Attempting to copy it from {}", pkg, USER_APP_DIR);

    std::string apk_path = find_apk(USER_APP_DIR, pkg);
    if (apk_path.empty()) {
        LOGW("Failed to find package {} in {}", pkg, USER_APP_DIR);
        return false;
    }

    if (!util::mkdir_parent(full_path, 0755)) {
        LOGW("Failed to create parent directories for {}", full_path);
        return false;
    }

    // Make sure we preserve the inode when copying, so all of the shared apk's
    // hardlinks are preserved
    if (!util::copy_contents(apk_path, full_path)) {
        LOGW("Failed to copy contents from {} to {}", apk_path, full_path);
        return false;
    }

    return true;
}

static bool relink_apk_to_target(const std::shared_ptr<Package> &pkg)
{
    std::string apk_path = get_shared_apk_path(pkg->name);
    std::string target(pkg->code_path);
    bool is_directory = false;

    if (!util::ends_with(pkg->code_path, ".apk")) {
        is_directory = true;
        target += "/base.apk";
    }

    struct stat sb1;
    struct stat sb2;

    if (stat(apk_path.c_str(), &sb1) < 0) {
        LOGW("Failed to stat {}: {}", apk_path, strerror(errno));
        return false;
    }

    if (stat(target.c_str(), &sb2) == 0
            && sb1.st_dev == sb2.st_dev
            && sb1.st_ino == sb2.st_ino) {
        // File is already linked correctly
        return true;
    }

    if (is_directory) {
        // Android >=5.0 structure
        if (!util::delete_recursive(pkg->code_path)) {
            LOGW("Failed to remove {}: {}",
                 pkg->code_path, strerror(errno));
        }
        if (mkdir(pkg->code_path.c_str(), 0755) < 0) {
            LOGW("Failed to create directory {}: {}",
                 pkg->code_path, strerror(errno));
        }
        if (!chown_wrapper(pkg->code_path, pkg, true)) {
            LOGW("Failed to chown {}: {}",
                 pkg->code_path, strerror(errno));
        }
        if (link(apk_path.c_str(), target.c_str()) < 0) {
            LOGW("Failed to hard link {} to {}: {}",
                 apk_path, target, strerror(errno));
            return false;
        }
    } else {
        // Android <5.0 structure
        if (unlink(target.c_str()) < 0) {
            LOGW("Failed to unlink {}: {}",
                 target, strerror(errno));
        }
        if (link(apk_path.c_str(), target.c_str()) < 0) {
            LOGW("Failed to hard link {} to {}: {}",
                 apk_path, target, strerror(errno));
            return false;
        }
    }

    return true;
}

static bool prepare_appsync()
{
    Packages pkgs;
    if (!pkgs.load_xml(PACKAGES_XML)) {
        LOGE("Failed to load {}", PACKAGES_XML);
        return false;
    }

    for (auto it = shared_pkgs.begin(); it != shared_pkgs.end();) {
        std::shared_ptr<SharedPackage> &shared_pkg = *it;

        // Ensure package is installed, so we can get its UID
        auto pkg = pkgs.find_by_pkg(shared_pkg->pkg_id);
        if (!pkg) {
            LOGW("Package {} won't be shared because it is not installed",
                 shared_pkg->pkg_id);
            it = shared_pkgs.erase(it);
            continue;
        }

        // Ensure that the package is not a system package if we're sharing the
        // apk file
        if (shared_pkg->share_apk && ((pkg->pkg_flags & Package::FLAG_SYSTEM)
                || (pkg->pkg_flags & Package::FLAG_UPDATED_SYSTEM_APP))) {
            LOGW("Package {} is a system app or an update to a system app. "
                 "Its apk will not be shared", shared_pkg->pkg_id);
            shared_pkg->share_apk = false;
        }

        // Ensure that code_path is set to something sane
        if (!util::starts_with(pkg->code_path, "/data/")) {
            LOGW("The code_path for package {} is not in /data. "
                 "Its apk will not be shared", shared_pkg->pkg_id);
            shared_pkg->share_apk = false;
        }

        // Ensure that the apk exists in the shared directory
        if (shared_pkg->share_apk
                && !ensure_apk_exists_in_app_sharing_dir(shared_pkg->pkg_id)) {
            shared_pkg->share_apk = false;
        }

        // Ensure that the data directory exists if data sharing is enabled
        std::string data_path = get_shared_data_path(shared_pkg->pkg_id);
        if (shared_pkg->share_data && !util::mkdir_recursive(data_path, 0751)) {
            LOGW("Failed to create {}. App data will not be shared", data_path);
            shared_pkg->share_data = false;
        }

        // Ensure that the shared data directory permissions are correct
        if (shared_pkg->share_data && chmod(data_path.c_str(), 0751) < 0) {
            LOGW("Failed to chmod {}. App data will not be shared", data_path);
            shared_pkg->share_data = false;
        }
        if (shared_pkg->share_data && !chown_wrapper(data_path, pkg, true)) {
            LOGW("Failed to chown {}. App data will not be shared", data_path);
            shared_pkg->share_data = false;
        }

        ++it;
    }

    // Ensure that the shared apk permissions are correct
    if (!FixPermissions(APP_SHARING_APP_DIR).run()) {
        LOGW("Failed to fix permissions on {}", APP_SHARING_APP_DIR);
    }

    // Ensure that the shared data is under the u:object_r:app_data_file:s0
    // context. Otherwise, apps won't be able to write to the shared directory
    bool disable_data_sharing = false;
    if (!util::selinux_lset_context_recursive(
            APP_SHARING_DATA_DIR, APP_DATA_SELINUX_CONTEXT)) {
        LOGW("Failed to set context on {}: {}", APP_SHARING_DATA_DIR,
             strerror(errno));
        LOGW("Data sharing will be disabled for all packages");
        disable_data_sharing = true;
    }

    // Actually share the apk and data
    for (std::shared_ptr<SharedPackage> &shared_pkg : shared_pkgs) {
        auto pkg = pkgs.find_by_pkg(shared_pkg->pkg_id);

        if (shared_pkg->share_apk && !relink_apk_to_target(pkg)) {
            LOGW("Failed to link shared apk to user app directory");
            if (shared_pkg->share_data) {
                LOGW("To prevent issues due to the error, "
                     "data will not be shared");
            }
            continue;
        }

        if (disable_data_sharing) {
            shared_pkg->share_data = false;
        }

        if (shared_pkg->share_data) {
            std::string data_path = get_shared_data_path(shared_pkg->pkg_id);
            std::string target(USER_DATA_DIR);
            target += "/";
            target += shared_pkg->pkg_id;

            if (!util::mkdir_recursive(target, 0755)) {
                LOGW("Failed to mkdir {}: {}", target, strerror(errno));
            }
            if (!chown_wrapper(target, pkg, true)) {
                LOGW("Failed to chown {}: {}", target, strerror(errno));
            }

            LOGV("Bind mounting data directory:");
            LOGV("- Source: {}", data_path);
            LOGV("- Target: {}", target);

            if (umount(target.c_str()) < 0 && errno != EINVAL) {
                LOGW("Failed to unmount: {}", strerror(errno));
                shared_pkg->share_data = false;
            } else if (mount(data_path.c_str(), target.c_str(),
                             "", MS_BIND, "") < 0) {
                LOGW("Failed to bind mount: {}", strerror(errno));
                LOGW("To prevent damage to shared data, data sharing is "
                     "temporarily disabled for this package");
                shared_pkg->share_data = false;
            }
        }
    }

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
        LOGE("Failed to create socket: {}", strerror(errno));
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_LOCAL;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s",
             INSTALLD_SOCKET_PATH);

    if (unlink(addr.sun_path) < 0 && errno != ENOENT) {
        LOGE("Failed to unlink old socket: {}", strerror(errno));
        close(fd);
        return -1;
    }

    if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        LOGE("Failed to bind socket: {}", strerror(errno));
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
static bool receive_message(int fd, char *buf, size_t size)
{
    unsigned short count;

    if (!util::socket_read_uint16(fd, &count)) {
        LOGE("Failed to read command size");
        return false;
    }

    if (count < 1 || count >= size) {
        LOGE("Invalid size {:d}", count);
        return false;
    }

    if (util::socket_read(fd, buf, count) != count) {
        LOGE("Failed to read command");
        return false;
    }

    buf[count] = 0;

    return true;
}

/*!
 * \brief Send a message to a socket
 */
static bool send_message(int fd, const char *command)
{
    unsigned short count = strlen(command);

    if (!util::socket_write_uint16(fd, count)) {
        LOGE("Failed to write command size");
        return false;
    }

    if (util::socket_write(fd, command, count) != count) {
        LOGE("Failed to write command");
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
        LOGE("Failed to create socket: {}", strerror(errno));
        return -1;
    }

    int attempt;
    for (attempt = 0; attempt < 5; ++attempt) {
        LOGV("Connecting to installd [Attempt {}/{}]", attempt + 1, 5);
        if (connect(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
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
        LOGE("Failed to launch installd: {}", strerror(errno));
        _exit(EXIT_FAILURE);
    } else if (pid < 0) {
        LOGD("Failed to fork: {}", strerror(errno));
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

static bool do_install(const std::vector<std::string> &args)
{
#define TAG "[install] "
    const std::string &pkgname = args[0];
    const int uid = strtol(args[1].c_str(), nullptr, 10);
    const int gid = strtol(args[2].c_str(), nullptr, 10);
    const std::string &seinfo = args[3];

    LOGD(TAG "pkgname = {}", pkgname);
    LOGD(TAG "uid = {}", uid);
    LOGD(TAG "gid = {}", gid);
    LOGD(TAG "seinfo = {}", seinfo);

    std::string apk = find_apk(USER_APP_DIR, pkgname);
    if (apk.empty()) {
        LOGW(TAG "Could not find apk in {}", USER_APP_DIR);
        LOGW(TAG "This package might be a paid app, which is not supported by appsync yet");
        return false;
    } else {
        LOGD(TAG "Found apk at {}", apk);
    }

    return true;
#undef TAG
}

static bool do_remove(const std::vector<std::string> &args)
{
#define TAG "[remove] "
    const std::string &pkgname = args[0];
    const int userid = strtol(args[1].c_str(), nullptr, 10);

    LOGD(TAG "pkgname = {}", pkgname);
    LOGD(TAG "userid = {}", userid);

    std::string apk = find_apk(USER_APP_DIR, pkgname);
    if (apk.empty()) {
        LOGW(TAG "Could not find apk in {}", USER_APP_DIR);
    } else {
        LOGW(TAG "Found apk at {}", apk);
    }

    return true;
#undef TAG
}

struct CommandInfo {
    const char *name;
    unsigned int nargs;
    bool (*func)(const std::vector<std::string> &args);
};

static struct CommandInfo cmds[] = {
    { "install", 4, do_install },
    { "remove",  2, do_remove },
};

static void handle_command(const std::vector<std::string> &args)
{
    for (std::size_t i = 0; i < sizeof(cmds) / sizeof(cmds[0]); ++i) {
        if (args[0] == cmds[i].name) {
            if (args.size() - 1 != cmds[i].nargs) {
                LOGE("{} requires {} arguments ({} given)",
                     cmds[i].name, cmds[i].nargs, args.size() - 1);
                LOGE("{} command won't be hooked", cmds[i].name);
            } else {
                LOGD("Hooking {} command", cmds[i].name);
                cmds[i].func(std::vector<std::string>(
                        args.begin() + 1, args.end()));
            }
        }
    }
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
            LOGE("Failed to accept client connection: {}", strerror(errno));
            return false;
        }

        auto close_client_fd = util::finally([&]{
            LOGD("Closing client connection");
            close(client_fd);
        });

        LOGD("Accepted new client connection");

        LOGD("Launching installd");
        pid_t pid = spawn_installd();
        if (pid < 0) {
            continue;
        }

        // Kill installd when we exit
        auto kill_installd = util::finally([&]{
            kill(pid, SIGINT);

            int status;

            do {
                if (waitpid(pid, &status, 0) < 0) {
                    LOGE("Failed to waitpid(): {}", strerror(errno));
                    break;
                }
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        });

        // Connect to installd
        int installd_fd = connect_to_installd();
        if (installd_fd < 0) {
            continue;
        }

        auto close_installd_fd = util::finally([&]{
            LOGD("Closing installd connection");
            close(installd_fd);
        });

        // Use the same buffer size as installd
        char buf[COMMAND_BUF_SIZE];

        while (true) {
            if (!receive_message(client_fd, buf, sizeof(buf))) {
                break;
            }

            std::vector<std::string> args = parse_args(buf);

            bool log_result = true;

            if (args.empty()) {
                LOGE("Invalid command (empty message)");
            } else {
                const std::string &cmd = args[0];

                if (cmd == "ping"
                        || cmd == "freecache") {
                    LOGD("Received unimportant command: [{}, ...]", cmd);
                } else if (cmd == "getsize") {
                    // Get size is so annoying we don't want it to show... EVER!
                    log_result = false;
                } else if (cmd == "install"
                        || cmd == "dexopt"
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
                    LOGD("Received command: {}", args_to_string(args));

                    if (can_appsync) {
                        handle_command(args);
                    }
                } else {
                    LOGW("Unrecognized command: {}", args_to_string(args));
                }
            }

            if (!send_message(installd_fd, buf)) {
                break;
            }

            if (!receive_message(installd_fd, buf, sizeof(buf))) {
                break;
            }

            args = parse_args(buf);

            if (log_result) {
                LOGD("Sending reply: {}", args_to_string(args));
            }

            if (!send_message(client_fd, buf)) {
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

    fcntl(orig_fd, F_SETFD, FD_CLOEXEC);

    // Listen on the original socket
    if (listen(orig_fd, 5) < 0) {
        LOGE("Failed to listen on socket: {}", strerror(errno));
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

    // Put the new fd in the environment
    put_socket_to_env(SOCKET_PATH, new_fd);

    LOGD("Ready! Waiting for connections");

    // Start processing commands!
    if (!proxy_process(orig_fd, can_appsync)) {
        return false;
    }

    return true;
}

/*!
 * \brief Patch SEPolicy to allow installd to connect to our fd
 */
static bool patch_sepolicy()
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

    util::selinux_add_rule(&pdb, "installd", "init", "unix_stream_socket", "accept");
    util::selinux_add_rule(&pdb, "installd", "init", "unix_stream_socket", "listen");
    util::selinux_add_rule(&pdb, "installd", "init", "unix_stream_socket", "read");
    util::selinux_add_rule(&pdb, "installd", "init", "unix_stream_socket", "write");

    if (!util::selinux_write_policy(MB_SELINUX_LOAD_FILE, &pdb)) {
        LOGE("Failed to write SELinux policy file: {}", MB_SELINUX_LOAD_FILE);
        policydb_destroy(&pdb);
        return false;
    }

    policydb_destroy(&pdb);

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
           "This tool implements app (apk and data) sharing by hijacking the connection\n"
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


    typedef std::unique_ptr<std::FILE, int (*)(std::FILE *)> file_ptr;

    if (!util::mkdir_parent(LOG_FILE, 0775) && errno != EEXIST) {
        fprintf(stderr, "Failed to create parent directory of %s: %s\n",
                LOG_FILE, strerror(errno));
        return EXIT_FAILURE;
    }

    file_ptr fp(fopen(LOG_FILE, "w"), fclose);
    if (!fp) {
        fprintf(stderr, "Failed to open log file %s: %s\n",
                LOG_FILE, strerror(errno));
        return EXIT_FAILURE;
    }

    // mbtool logging
    util::log_set_logger(std::make_shared<util::StdioLogger>(fp.get()));

    LOGI("=== APPSYNC VERSION {} ===", MBP_VERSION);


    // Allow installd to read and write to our socket
    struct stat sb;
    if (stat("/sys/fs/selinux", &sb) < 0) {
        LOGV("SELinux not supported. No need to modify policy");
    } else {
        LOGV("Patching SELinux policy to allow installd connection");
        if (!patch_sepolicy()) {
            LOGW("Failed to patch current SELinux policy");
        }
    }

    bool can_appsync = false;

    // Try to load config file
    if (!load_config_file()) {
        LOGW("Failed to load configuration file; app sharing will not work");
        LOGW("Continuing to proxy installd anyway...");
    } else {
        can_appsync = prepare_appsync();
        if (!can_appsync) {
            LOGW("appsync preparation failed. "
                 "App sharing is completely disabled");
        }
    }

    return hijack_socket(can_appsync) ? EXIT_SUCCESS : EXIT_FAILURE;
}

}
