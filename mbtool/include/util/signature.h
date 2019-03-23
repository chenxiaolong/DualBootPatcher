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

#pragma once

#include <unordered_map>
#include <string>

#include "mbcommon/outcome.h"
#include "mbcommon/version.h"

#define TRUSTED_PROP_TIMESTAMP  "timestamp"
#define TRUSTED_PROP_VERSION    "version"

namespace mb
{

enum class SigVerifyError
{
    MissingVersion = 1,
    MismatchedVersion,
};

std::error_code make_error_code(SigVerifyError e);

const std::error_category & sigverify_error_category();

using TrustedProps = std::unordered_map<std::string, std::string>;

oc::result<TrustedProps>
verify_signature(const char *path, const char *sig_path,
                 const char *version = mb::version());

int sigverify_main(int argc, char *argv[]);

}

namespace std
{
    template<>
    struct is_error_code_enum<mb::SigVerifyError> : true_type
    {
    };
}
