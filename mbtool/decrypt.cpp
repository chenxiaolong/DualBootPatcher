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

#include <vector>

#include <cassert>
#include <cstdlib>
#include <cstring>

#include <getopt.h>
#include <sys/wait.h>
#include <unistd.h>

#include "mbdevice/json.h"
#include "mbdevice/validate.h"
#include "mblog/logging.h"
#include "mblog/stdio_logger.h"
#include "mbutil/command.h"
#include "mbutil/file.h"
#include "mbutil/properties.h"

#include "multiboot.h"
#include "signature.h"

#define CRYPTFSTOOL_PATH        "/raw/cache/multiboot/crypto/cryptfstool"
#define CRYPTFSTOOL_REC_PATH    "/raw/cache/multiboot/crypto/cryptfstool_rec"

static bool verbose = true;

namespace mb
{

static bool check_signature(const char *path, const char *sig_path)
{
    SigVerifyResult result;
    result = verify_signature(path, sig_path);
    switch (result) {
    case SigVerifyResult::VALID:
        return true;
    case SigVerifyResult::INVALID:
        LOGE("%s: Invalid signature", path);
        return false;
    case SigVerifyResult::FAILURE:
        LOGE("%s: Failed to check signature", path);
        return false;
    default:
        assert(false);
        return false;
    }
}

static bool signature_ok()
{
    return check_signature(CRYPTFSTOOL_REC_PATH, CRYPTFSTOOL_REC_PATH ".sig")
            && check_signature(CRYPTFSTOOL_PATH, CRYPTFSTOOL_PATH ".sig");
}

struct paths
{
    std::string userdata;
    std::string header;
    std::string recovery;
};

static bool find_paths(struct paths *paths)
{
    std::unordered_map<std::string, std::string> props;
    util::file_get_all_properties(DEFAULT_PROP_PATH, &props);

    std::string userdata;
    std::string header;
    std::string recovery;

    std::vector<unsigned char> contents;
    util::file_read_all(DEVICE_JSON_PATH, &contents);
    contents.push_back('\0');

    MbDeviceJsonError error;
    Device *device = mb_device_new_from_json((char *) contents.data(), &error);
    if (!device) {
        LOGE("%s: Failed to load device definition", DEVICE_JSON_PATH);
        return false;
    } else if (mb_device_validate(device) != 0) {
        LOGE("%s: Device definition validation failed", DEVICE_JSON_PATH);
        mb_device_free(device);
        return false;
    }

    auto userdata_list = mb_device_data_block_devs(device);
    auto recovery_list = mb_device_recovery_block_devs(device);

    if (userdata_list) {
        for (auto it = userdata_list; *it; ++it) {
            if (access(*it, R_OK) == 0) {
                userdata = *it;
                break;
            }
        }
    }

    if (recovery_list) {
        for (auto it = recovery_list; *it; ++it) {
            if (access(*it, R_OK) == 0) {
                recovery = *it;
                break;
            }
        }
    }

    mb_device_free(device);

    header = props[PROP_CRYPTFS_HEADER_PATH];

    if (userdata.empty()) {
        LOGE("Encrypted partition path could not be detected");
        return false;
    }
    if (recovery.empty()) {
        LOGE("Recovery partition path could not be detected");
        return false;
    }
    if (header.empty()) {
        LOGE("Cryptfs header path could not be detected");
        return false;
    }

    paths->userdata.swap(userdata);
    paths->recovery.swap(recovery);
    paths->header.swap(header);

    return true;
}

static void save_first_line(const char *line, bool error, void *userdata)
{
    std::string *result = static_cast<std::string *>(userdata);

    size_t size = strlen(line);

    // Avoid needless copies if not logging the output
    if (verbose) {
        std::string copy;
        if (size > 0 && line[size - 1] == '\n') {
            copy.assign(line, line + size - 1);
        } else {
            copy.assign(line, line + size);
        }

        LOGD("[cryptfstool/%s] %s", error ? "stderr" : "stdout", copy.c_str());

        if (result->empty() && size > 0 && !error) {
            *result = copy;
        }
    } else {
        if (result->empty() && size > 0 && !error) {
            if (line[size - 1] == '\n') {
                result->assign(line, line + size - 1);
            } else {
                result->assign(line, line + size);
            }
        }
    }
}

std::string decrypt_get_pw_type()
{
    struct paths paths;
    if (!find_paths(&paths)) {
        return std::string();
    }

    if (!signature_ok()) {
        return std::string();
    }

    // Race condition...

    const char *argv[] = {
        CRYPTFSTOOL_PATH,
        "getpwtype",
        "--path", paths.userdata.c_str(),
        "--header", paths.header.c_str(),
        nullptr
    };

    char recovery_path_env[100];
    snprintf(recovery_path_env, sizeof(recovery_path_env),
             "CRYPTFS_RECOVERY_PATH=%s", paths.recovery.c_str());

    const char *envp[] = {
        "CRYPTFS_TOOL_PATH=" CRYPTFSTOOL_REC_PATH,
        recovery_path_env,
        nullptr
    };

    std::string pw_type;
    int status = util::run_command(argv[0], argv, envp, nullptr,
                                   &save_first_line, &pw_type);

    if (status >= 0 && WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        if (pw_type == CRYPTFS_PW_TYPE_DEFAULT) {
            return CRYPTFS_PW_TYPE_DEFAULT;
        } else if (pw_type == CRYPTFS_PW_TYPE_PASSWORD) {
            return CRYPTFS_PW_TYPE_PASSWORD;
        } else if (pw_type == CRYPTFS_PW_TYPE_PATTERN) {
            return CRYPTFS_PW_TYPE_PATTERN;
        } else if (pw_type == CRYPTFS_PW_TYPE_PIN) {
            return CRYPTFS_PW_TYPE_PIN;
        } else {
            LOGW("Unknown password type: '%s'", pw_type.c_str());
        }
    }

    return std::string();
}

std::string decrypt_userdata(const char *password)
{
    struct paths paths;
    if (!find_paths(&paths)) {
        return std::string();
    }

    if (!signature_ok()) {
        return std::string();
    }

    // Race condition...

    const char *argv[] = {
        CRYPTFSTOOL_PATH,
        "decrypt", password,
        "--path", paths.userdata.c_str(),
        "--header", paths.header.c_str(),
        nullptr
    };

    char recovery_path_env[100];
    snprintf(recovery_path_env, sizeof(recovery_path_env),
             "CRYPTFS_RECOVERY_PATH=%s", paths.recovery.c_str());

    const char *envp[] = {
        "CRYPTFS_TOOL_PATH=" CRYPTFSTOOL_REC_PATH,
        recovery_path_env,
        nullptr
    };

    std::string block_dev;
    int status = util::run_command(argv[0], argv, envp, nullptr,
                                   &save_first_line, &block_dev);

    if (status >= 0 && WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        return block_dev;
    }

    return std::string();
}

static void decrypt_usage(FILE *stream)
{
    fprintf(stream,
            "Usage: decrypt [OPTION]... <password>\n\n"
            "Options:\n"
            "  -q, --quiet      Hide cryptfstool output\n"
            "  -h, --help       Display this help message\n");
}

int decrypt_main(int argc, char *argv[])
{
    int opt;

    static const char short_options[] = "qh";

    static struct option long_options[] = {
        {"quiet", no_argument, 0, 'q'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, short_options,
                              long_options, &long_index)) != -1) {
        switch (opt) {
        case 'q':
            verbose = false;
            break;

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

    // Set up logging
    mb::log::log_set_logger(
            std::make_shared<mb::log::StdioLogger>(stderr, false));

    const char *password = argv[optind];

    std::string password_type = decrypt_get_pw_type();
    printf("Detected password type: %s\n",
           password_type.empty() ? "<unknown>" : password_type.c_str());

    std::string block_dev = decrypt_userdata(password);
    if (!block_dev.empty()) {
        printf("Successully decrypted userdata: %s\n", block_dev.c_str());
        return EXIT_SUCCESS;
    } else {
        fprintf(stderr, "Failed to decrypt userdata\n");
        return EXIT_FAILURE;
    }
}

}
