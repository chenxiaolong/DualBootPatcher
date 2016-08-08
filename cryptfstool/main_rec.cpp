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

#include <string>

#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <dlfcn.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/dm-ioctl.h>
#include <sys/wait.h>
#include <termio.h>
#include <unistd.h>

#include "mblog/logging.h"
#include "mblog/stdio_logger.h"
#include "mbutil/properties.h"

#ifndef TCSASOFT
#define TCSASOFT 0
#endif

#define LIBCRYPTFS_PATH                 "/sbin/libcryptfslollipop.so"

#define CRYPT_TYPE_PASSWORD             0
#define CRYPT_TYPE_DEFAULT              1
#define CRYPT_TYPE_PATTERN              2
#define CRYPT_TYPE_PIN                  3

#define CRYPT_TYPE_PASSWORD_STR         "password"
#define CRYPT_TYPE_DEFAULT_STR          "default"
#define CRYPT_TYPE_PATTERN_STR          "pattern"
#define CRYPT_TYPE_PIN_STR              "pin"

#define DMSETUP_PATH                    "/dev/device-mapper"
#define DMSETUP_FMT                     "/dev/block/dm-%u"
#define DMSETUP_NAME                    "userdata"

#define BLOCK_DEV_PROP                  "ro.crypto.fs_crypto_blkdev"


struct cryptfs
{
    void *handle = nullptr;
    void (*set_partition_data)(const char *, const char *, const char *) = nullptr;
    int (*cryptfs_get_password_type)(void) = nullptr;
    int (*cryptfs_check_passwd)(char *) = nullptr;
};

struct ctx
{
    int (*action_fn)(int, struct cryptfs *, void *);
    std::string path_enc;
    std::string path_hdr;
    bool allow_stdin = false;
    char password[512];
};

static void unload_libcryptfs(struct cryptfs *cfs)
{
    if (cfs->handle) {
        dlclose(cfs->handle);
        cfs->handle = nullptr;
        cfs->set_partition_data = nullptr;
        cfs->cryptfs_get_password_type = nullptr;
        cfs->cryptfs_check_passwd = nullptr;
    }
}

static bool load_libcryptfs(struct cryptfs *cfs, const char *path)
{
    if (cfs->handle) {
        return true;
    }

    cfs->handle = dlopen(path, RTLD_LOCAL);
    if (!cfs->handle) {
        LOGE("Failed to load libcryptfs: %s", dlerror());
        goto error;
    }

#define ASSIGN_CAST(dest, src) \
        dest = reinterpret_cast<decltype(dest)>(src);

    ASSIGN_CAST(cfs->set_partition_data,
                dlsym(cfs->handle, "set_partition_data"));
    ASSIGN_CAST(cfs->cryptfs_get_password_type,
                dlsym(cfs->handle, "cryptfs_get_password_type"));
    ASSIGN_CAST(cfs->cryptfs_check_passwd,
                dlsym(cfs->handle, "cryptfs_check_passwd"));

#undef ASSIGN_CAST

    if (!cfs->set_partition_data) {
        LOGE("set_partition_data@libcryptfs: %s", dlerror());
        goto error;
    }

    if (!cfs->cryptfs_get_password_type) {
        LOGE("cryptfs_get_password_type@libcryptfs: %s", dlerror());
        goto error;
    }

    if (!cfs->cryptfs_check_passwd) {
        LOGE("cryptfs_check_passwd@libcryptfs: %s", dlerror());
        goto error;
    }

    return true;

error:
    unload_libcryptfs(cfs);
    return false;
}

static int execute_action(int (*fn)(int, struct cryptfs *, void *),
                          void *userdata)
{
    struct cryptfs cfs;

    // Redirect stdout to stderr because libcryptfs uses printf's
    int orig_stdout = dup(STDOUT_FILENO);
    dup2(STDERR_FILENO, STDOUT_FILENO);

    if (!load_libcryptfs(&cfs, LIBCRYPTFS_PATH)) {
        _exit(EXIT_FAILURE);
    }

    int ret = fn(orig_stdout, &cfs, userdata);

    unload_libcryptfs(&cfs);

    // Android's printf no longer works properly after this (of course...)
    dup2(orig_stdout, STDOUT_FILENO);
    close(orig_stdout);

    return ret;
}

static void ioctl_init(struct dm_ioctl *io, size_t dataSize, const char *name,
                       unsigned flags)
{
    memset(io, 0, dataSize);
    io->data_size = dataSize;
    io->data_start = sizeof(struct dm_ioctl);
    io->version[0] = 4;
    io->version[1] = 0;
    io->version[2] = 0;
    io->flags = flags;
    if (name) {
        strncpy(io->name, name, sizeof(io->name));
    }
}

char * dmsetup_path(const char *name)
{
    char buf[4096];
    struct dm_ioctl *io = reinterpret_cast<struct dm_ioctl *>(buf);
    int fd;
    char * ret = nullptr;
    unsigned int minor;

    fd = open(DMSETUP_PATH, O_RDWR);
    if (fd < 0) {
        LOGE("%s: Failed to open device: %s", DMSETUP_PATH, strerror(errno));
        goto done;
    }

    ioctl_init(io, sizeof(buf), name, 0);
    if (ioctl(fd, DM_DEV_STATUS, io) != 0) {
        goto done;
    }

    minor = (io->dev & 0xff) | ((io->dev >> 12) & 0xfff00);
    if (asprintf(&ret, DMSETUP_FMT, minor) < 0) {
        ret = nullptr;
        goto done;
    }

done:
    close(fd);
    return ret;
}

static int action_get_pw_type(int fd, struct cryptfs *cfs, void *userdata)
{
    struct ctx *ctx = static_cast<struct ctx *>(userdata);

    cfs->set_partition_data(ctx->path_enc.c_str(), ctx->path_hdr.c_str(),
                            "ext4");

    int type = cfs->cryptfs_get_password_type();
    if (type < 0) {
        LOGW("Could not determine password type. Assuming 'password' type");
        type = CRYPT_TYPE_PASSWORD;
    }

    switch (type) {
    case CRYPT_TYPE_PASSWORD:
        dprintf(fd, CRYPT_TYPE_PASSWORD_STR "\n");
        break;
    case CRYPT_TYPE_DEFAULT:
        dprintf(fd, CRYPT_TYPE_DEFAULT_STR "\n");
        break;
    case CRYPT_TYPE_PATTERN:
        dprintf(fd, CRYPT_TYPE_PATTERN_STR "\n");
        break;
    case CRYPT_TYPE_PIN:
        dprintf(fd, CRYPT_TYPE_PIN_STR "\n");
        break;
    default:
        LOGE("Unknown password type: %d", type);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int action_decrypt(int fd, struct cryptfs *cfs, void *userdata)
{
    struct ctx *ctx = static_cast<struct ctx *>(userdata);

    // Check if already decrypted
    char *path = dmsetup_path(DMSETUP_NAME);
    if (path) {
        LOGW("dmsetup device '%s' already exists", DMSETUP_NAME);
        dprintf(fd, "%s\n", path);
        free(path);
        return EXIT_SUCCESS;
    }

    cfs->set_partition_data(ctx->path_enc.c_str(), ctx->path_hdr.c_str(),
                            "ext4");

    int ret = cfs->cryptfs_check_passwd(ctx->password);
    if (ret != 0) {
        LOGE("%s: Failed to decrypt file", ctx->path_enc.c_str());
        return EXIT_FAILURE;
    }

    char block_dev[PROP_VALUE_MAX];
    mb::util::property_get(BLOCK_DEV_PROP, block_dev, "");

    if (block_dev[0]) {
        dprintf(fd, "%s\n", block_dev);
    } else {
        path = dmsetup_path(DMSETUP_NAME);
        if (path) {
            dprintf(fd, "%s\n", path);
            free(path);
        } else {
            LOGE("Failed to get block device path");
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

/*!
 * \brief Get password from TTY or stdin
 *
 * This function is partially based on LGPLv2.1+ licensed getpass.c from glibc.
 *
 * \return - Size of password read if input length < \a size
 *         - -1 if an error occurs or if \a size > INT_MAX
 *         - \a size if input is too long
 */
static int get_password(const char *prompt, bool allow_stdin,
                        char *buf, size_t size)
{
    FILE *f_in;
    FILE *f_out;
    struct termios t_old;
    struct termios t_new;
    bool tty_changed;

    if (size > INT_MAX || size == 0) {
        errno = ERANGE;
        return -1;
    }

    f_in = fopen("/dev/tty", "w+ce");
    if (!f_in) {
        if (!allow_stdin) {
            return -1;
        }
        f_in = stdin;
        f_out = stderr;
    } else {
        f_out = f_in;
    }

    flockfile(f_out);

    // Disable TTY echo
    if (tcgetattr(fileno(f_in), &t_old) == 0) {
        t_new = t_old;
        t_new.c_lflag &= ~(ECHO | ISIG);
        tty_changed = (tcsetattr(fileno(f_in), TCSAFLUSH | TCSASOFT, &t_new) == 0);
    } else {
        tty_changed = false;
    }

    fprintf(f_out, "%s", prompt);
    fflush(f_out);

    int saved_errno = 0;
    int c;
    int pos = 0;

    while (pos < size) {
        c = fgetc(f_in);
        if (c == EOF || c == '\n') {
            if (c == EOF && ferror(f_in)) {
                saved_errno = errno;
                pos = -1;
            } else {
                buf[pos] = '\0';
            }
            break;
        }
        buf[pos++] = c;
    }

    if (c != EOF && c != '\n') {
        // Password is too long
        pos = size;
    }

    if (pos < 0 || pos == size) {
        // Error occurred
        memset(buf, 0, size);
    }

    if (tty_changed) {
        // Write the newline that was not echoed
        fprintf(f_out, "\n");

        // Restore terminal settings
        tcsetattr(fileno(f_in), TCSAFLUSH | TCSASOFT, &t_old);
    }

    funlockfile(f_out);

    if (f_in != stdin) {
        // Close TTY
        fclose(f_in);
    }

    return pos;
}

static void redact_mem(void *password, size_t size)
{
    static const char redacted[] = "REDACTED";

    while (size > 0) {
        size_t n = (size > sizeof(redacted) - 1)
                ? (sizeof(redacted) - 1) : size;
        memcpy(password, redacted, n);
        password = (char *) password + n;
        size -= n;
    }
}

static const char Usage[] =
    "Usage: cryptfstool_rec <action> [<args> ...]\n"
    "\n"
    "Actions:\n"
    "  getpwtype      Get password type\n"
    "  decrypt [<password>]\n"
    "                 Decrypt an encrypted partition\n"
    "                 If password parameter is omitted, a password prompt will\n"
    "                 be shown\n"
    "\n"
    "Options:\n"
    "  --path <path>\n"
    "                 Path to encrypted image/partition\n"
    "                 (Required argument)\n"
    "  --header <path>\n"
    "                 Path to crypto header file/partition or \"footer\"\n"
    "                 if it is at the end of the encrypted file/partition\n"
    "                 (Required argument)\n"
    "  --allow-stdin  Allow reading password from stdin instead of the TTY\n";

#define PARSE_ARGS_CONTINUE             -1

static int parse_args(struct ctx *ctx, int argc, char *argv[])
{
    int opt;

    enum {
        OPT_HEADER      = 1000,
        OPT_PATH        = 1001,
        OPT_ALLOW_STDIN = 1002,
    };

    static const char short_options[] = "h";
    static struct option long_options[] = {
        { "header",      required_argument, nullptr, OPT_HEADER      },
        { "path",        required_argument, nullptr, OPT_PATH        },
        { "allow-stdin", required_argument, nullptr, OPT_ALLOW_STDIN },
        { "help",        no_argument,       nullptr, 'h'             },
        { nullptr,       0,                 nullptr, 0               }
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, short_options,
                              long_options, &long_index)) != -1) {
        switch (opt) {
        case OPT_HEADER:
            ctx->path_hdr = optarg;
            break;

        case OPT_PATH:
            ctx->path_enc = optarg;
            break;

        case OPT_ALLOW_STDIN:
            ctx->allow_stdin = true;
            break;

        case 'h':
            fputs(Usage, stdout);
            return EXIT_SUCCESS;

        default:
            fputs(Usage, stderr);
            return EXIT_FAILURE;
        }
    }

    if (argc - optind == 0) {
        fputs(Usage, stderr);
        return EXIT_FAILURE;
    }

    const char *action = argv[optind++];

    if (strcmp(action, "getpwtype") == 0) {
        if (argc - optind != 0) {
            fputs(Usage, stderr);
            return EXIT_FAILURE;
        }

        ctx->action_fn = action_get_pw_type;
    } else if (strcmp(action, "decrypt") == 0) {
        if (argc - optind == 0) {
            int n = get_password("Password: ", ctx->allow_stdin,
                                 ctx->password, sizeof(ctx->password));
            if (n < 0) {
                fprintf(stderr, "Failed to read password\n");
                redact_mem(ctx->password, sizeof(ctx->password));
                return EXIT_FAILURE;
            } else if (n == sizeof(ctx->password)) {
                fprintf(stderr, "Password is too long\n");
                redact_mem(ctx->password, sizeof(ctx->password));
                return EXIT_FAILURE;
            }
        } else if (argc - optind == 1) {
            size_t size = strlcpy(ctx->password, argv[optind],
                                  sizeof(ctx->password));

            // Clear argument to avoid it showing up in /proc/<pid>/cmdline
            redact_mem(argv[optind], strlen(argv[optind]));

            if (size >= sizeof(ctx->password)) {
                fprintf(stderr, "Password is too long\n");
                redact_mem(ctx->password, sizeof(ctx->password));
                return EXIT_FAILURE;
            }
        } else {
            fputs(Usage, stderr);
            return EXIT_FAILURE;
        }

        ctx->action_fn = action_decrypt;
    } else {
        fprintf(stderr, "Invalid action: %s\n", action);
        return EXIT_FAILURE;
    }

    if (ctx->path_enc.empty()) {
        fprintf(stderr, "Missing --path argument\n");
        return EXIT_FAILURE;
    }
    if (ctx->path_hdr.empty()) {
        fprintf(stderr, "Missing --header argument\n");
        return EXIT_FAILURE;
    }

    return PARSE_ARGS_CONTINUE;
}

int main(int argc, char *argv[])
{
    struct ctx ctx;

    int args_ret = parse_args(&ctx, argc, argv);
    if (args_ret != PARSE_ARGS_CONTINUE) {
        return args_ret;
    }

    // Set up logging
    mb::log::log_set_logger(
            std::make_shared<mb::log::StdioLogger>(stderr, false));

    // Make rootfs writable
    if (unshare(CLONE_NEWNS) < 0) {
        fprintf(stderr, "Failed to unshare mount namespace: %s\n",
                strerror(errno));
        return EXIT_FAILURE;
    }

    int status = execute_action(ctx.action_fn, &ctx);
    bool ret = status != -1 && WIFEXITED(status) && WEXITSTATUS(status) == 0;
    redact_mem(ctx.password, sizeof(ctx.password));

    return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
