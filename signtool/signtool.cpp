/*
 * Copyright (C) 2016-2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <memory>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "mbcommon/file/standard.h"
#include "mbcommon/version.h"

#include "mbsign/sign.h"

using namespace mb;
using namespace mb::sign;

static void usage(FILE *stream) noexcept
{
    fprintf(stream,
            "Usage: signtool <secret key file> [[<input file> <output signature file>] ...]\n\n"
            "NOTE: This is not a general purpose tool for signing files!\n"
            "It is only meant for use during the build process.\n");
}

static SecureUniquePtr<char> get_passphrase() noexcept
{
    char *pass = getenv("MBSIGN_PASSPHRASE");
    if (!pass) {
        return nullptr;
    }

    size_t pass_size = strlen(pass);

    auto result = make_secure_array<char>(pass_size + 1);
    if (!result) {
        return nullptr;
    }

    strcpy(result.get(), pass);
    memset(pass, '*', pass_size);

    return result;
}

static oc::result<SecretKey> load_secret_key(const char *filename,
                                             const char *passphrase)
{
    StandardFile file;

    OUTCOME_TRYV(file.open(filename, FileOpenMode::ReadOnly));

    return load_secret_key(file, passphrase);
}

static oc::result<void> add_version_prop(Signature &sig, const SecretKey &key)
{
    std::string append(";version:");
    append += version();

    if (append.size() >= sig.trusted.size() - strlen(sig.trusted.data())) {
        return std::errc::invalid_argument;
    }

    strcat(sig.trusted.data(), append.c_str());

    return update_global_signature(sig, key);
}

int main(int argc, char *argv[])
{
    if (!mb::sign::initialize()) {
        fprintf(stderr, "Failed to initialize libmbsign\n");
        return EXIT_FAILURE;
    }

    if (argc < 4 || argc & 1) {
        usage(stderr);
        return EXIT_FAILURE;
    }

    const char *file_skey = argv[1];

    auto pass = get_passphrase();
    if (!pass) {
        fprintf(stderr,
                "The MBSIGN_PASSPHRASE environment variable is not set\n");
        return EXIT_FAILURE;
    }

    auto skey = load_secret_key(file_skey, pass.get());
    if (!skey) {
        fprintf(stderr, "%s: Failed to load secret key: %s\n",
                file_skey, skey.error().message().c_str());
        return EXIT_FAILURE;
    }

    for (int i = 2; i < argc; i += 2) {
        const char *file_input = argv[i];
        const char *file_output = argv[i + 1];

        StandardFile file;

        if (auto r = file.open(file_input, FileOpenMode::ReadOnly); !r) {
            fprintf(stderr, "%s: Failed to open for reading: %s\n",
                    file_input, r.error().message().c_str());
            return EXIT_FAILURE;
        }

        auto sig = sign_file(file, skey.value());
        if (!sig) {
            fprintf(stderr, "%s: Failed to sign file: %s\n",
                    file_input, sig.error().message().c_str());
            return EXIT_FAILURE;
        }

        if (auto r = file.close(); !r) {
            fprintf(stderr, "%s: Failed to close file: %s\n",
                    file_input, r.error().message().c_str());
            return EXIT_FAILURE;
        }

        if (auto r = file.open(file_output, FileOpenMode::WriteOnly); !r) {
            fprintf(stderr, "%s: Failed to open for writing: %s\n",
                    file_output, r.error().message().c_str());
            return EXIT_FAILURE;
        }

        if (auto r = add_version_prop(sig.value(), skey.value()); !r) {
            fprintf(stderr, "%s: Failed to update trusted comment: %s\n",
                    file_output, r.error().message().c_str());
            return EXIT_FAILURE;
        }

        if (auto r = save_signature(file, sig.value()); !r) {
            fprintf(stderr, "%s: Failed to write signature: %s\n",
                    file_output, r.error().message().c_str());
            return EXIT_FAILURE;
        }

        if (auto r = file.close(); !r) {
            fprintf(stderr, "%s: Failed to close file: %s\n",
                    file_output, r.error().message().c_str());
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
