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

#include "util/signature.h"

#include <cstdlib>

#include <getopt.h>

#include "mbcommon/file/standard.h"
#include "mblog/logging.h"
#include "mbsign/build_key.h"
#include "mbsign/error.h"
#include "mbsign/sign.h"

#define LOG_TAG "mbtool/util/signature"

namespace mb
{

static oc::result<sign::Signature> load_signature(const char *path)
{
    StandardFile file;

    OUTCOME_TRYV(file.open(path, FileOpenMode::ReadOnly));

    return sign::load_signature(file);
}

static oc::result<void> verify_signature(const char *path,
                                         const sign::Signature &sig,
                                         const sign::PublicKey &key)
{
    StandardFile file;

    OUTCOME_TRYV(file.open(path, FileOpenMode::ReadOnly));

    return sign::verify_file(file, sig, key);
}

SigVerifyResult verify_signature(const char *path, const char *sig_path)
{
    auto sig = load_signature(sig_path);
    if (!sig) {
        LOGE("%s: Failed to load signature: %s",
             sig_path, sig.error().message().c_str());
        return SigVerifyResult::Failure;
    }

    if (auto r = verify_signature(path, sig.value(), sign::build_key()); !r) {
        if (r.error() == sign::Error::SignatureVerifyFailed) {
            return SigVerifyResult::Invalid;
        } else {
            LOGE("%s: Failed to verify signature: %s",
                 path, r.error().message().c_str());
            return SigVerifyResult::Failure;
        }
    }

    return SigVerifyResult::Valid;
}

static void sigverify_usage(FILE *stream)
{
    fprintf(stream,
            "Usage: sigverify [option...] <input file> <signature file>\n\n"
            "Options:\n"
            "  -h, --help       Display this help message\n"
            "\n"
            "Exit codes:\n"
            "  0: Signature is valid\n"
            "  1: An error occurred while checking the signature\n"
            "  2: Signature is invalid\n");
}

//#define EXIT_SUCCESS
//#define EXIT_FAILURE
#define EXIT_INVALID 2

int sigverify_main(int argc, char *argv[])
{
    int opt;

    static struct option long_options[] = {
        {"help",      no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, "h", long_options, &long_index)) != -1) {
        switch (opt) {
        case 'h':
            sigverify_usage(stdout);
            return EXIT_SUCCESS;

        default:
            sigverify_usage(stderr);
            return EXIT_FAILURE;
        }
    }

    if (argc - optind != 2) {
        sigverify_usage(stderr);
        return EXIT_FAILURE;
    }

    const char *path = argv[optind];
    const char *sig_path = argv[optind + 1];

    if (!sign::initialize()) {
        LOGE("Failed to initialize libmbsign");
        return EXIT_FAILURE;
    }

    SigVerifyResult result = verify_signature(path, sig_path);

    switch (result) {
    case SigVerifyResult::Valid:
        return EXIT_SUCCESS;
    case SigVerifyResult::Invalid:
        return EXIT_INVALID;
    case SigVerifyResult::Failure:
    default:
        return EXIT_FAILURE;
    }
}

}
