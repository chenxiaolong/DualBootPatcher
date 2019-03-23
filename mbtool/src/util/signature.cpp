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
#include "mbcommon/string.h"
#include "mbcommon/version.h"

#include "mblog/logging.h"

#include "mbsign/build_key.h"
#include "mbsign/error.h"
#include "mbsign/sign.h"

#define LOG_TAG "mbtool/util/signature"

namespace mb
{

struct SigVerifyErrorCategory : std::error_category
{
    const char * name() const noexcept override;

    std::string message(int ev) const override;
};

const std::error_category & sigverify_error_category()
{
    static SigVerifyErrorCategory c;
    return c;
}

std::error_code make_error_code(SigVerifyError e)
{
    return {static_cast<int>(e), sigverify_error_category()};
}

const char * SigVerifyErrorCategory::name() const noexcept
{
    return "sigverify";
}

std::string SigVerifyErrorCategory::message(int ev) const
{
    switch (static_cast<SigVerifyError>(ev)) {
    case SigVerifyError::MissingVersion:
        return "missing version in trusted comment";
    case SigVerifyError::MismatchedVersion:
        return "mismatched version in trusted comment";
    default:
        return "(unknown sigverify error)";
    }
}

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

oc::result<TrustedProps>
verify_signature(const char *path, const char *sig_path, const char *version)
{
    OUTCOME_TRY(sig, load_signature(sig_path));
    OUTCOME_TRYV(verify_signature(path, sig, sign::build_key()));

    TrustedProps props;

    // Parse trusted comment
    for (auto const &kv : split_sv(sig.trusted.data(), ';')) {
        if (auto pos = kv.find(':'); pos != kv.npos) {
            props[std::string(kv.substr(0, pos))] = kv.substr(pos + 1);
        }
    }

    if (version) {
        if (auto it = props.find(TRUSTED_PROP_VERSION); it == props.end()) {
            return SigVerifyError::MissingVersion;
        } else if (it->second != version) {
            return SigVerifyError::MismatchedVersion;
        }
    }

    return props;
}

static void sigverify_usage(FILE *stream)
{
    fprintf(stream,
            "Usage: sigverify [option...] <input file> <signature file>\n\n"
            "Options:\n"
            "  -v, --version <VERSION>\n"
            "                   Check that trusted comment matches version\n"
            "  -V, --build-version\n"
            "                   Check that trusted comment matches build version\n"
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

    static constexpr char short_options[] = "v:Vh";

    static constexpr option long_options[] = {
        {"version",       required_argument, nullptr, 'v'},
        {"build-version", no_argument,       nullptr, 'V'},
        {"help",          no_argument,       nullptr, 'h'},
        {nullptr,         0,                 nullptr, 0},
    };

    int long_index = 0;
    const char *version = nullptr;

    while ((opt = getopt_long(argc, argv, short_options,
                              long_options, &long_index)) != -1) {
        switch (opt) {
        case 'v':
            version = optarg;
            break;

        case 'V':
            version = mb::version();
            break;

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

    if (auto r = verify_signature(path, sig_path, version)) {
        LOGV("%s: Valid signature", path);
        if (!r.value().empty()) {
            for (auto const &[k, v] : r.value()) {
                LOGV("Trusted property: %s=%s", k.c_str(), v.c_str());
            }
        }
        return EXIT_SUCCESS;
    } else if (r.error() == sign::Error::SignatureVerifyFailed) {
        LOGW("%s: Invalid signature", path);
        return EXIT_INVALID;
    } else {
        LOGE("%s: Failed to verify signature: %s",
             path, r.error().message().c_str());
        return EXIT_FAILURE;
    }
}

}
