/*
 * Copyright (C) 2015-2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/version.h"

#include "mbcommon/integer.h"
#include "mbcommon/string.h"

#define MBP_VERSION R"(@MBP_VERSION@)"
#define GIT_VERSION R"(@GIT_VERSION@)"

namespace mb
{

const char * version()
{
    return MBP_VERSION;
}

const char * git_version()
{
    return GIT_VERSION;
}

int version_compare(const char *version1, const char *version2)
{
    auto split1 = split_sv(version1, ".");
    auto split2 = split_sv(version2, ".");
    auto it1 = split1.begin();
    auto it2 = split2.begin();

    for (; it1 != split1.end() && it2 != split2.end(); ++it1, ++it2) {
        unsigned int num1;
        unsigned int num2;
        int cmp;

        if (str_to_num(*it1, 10, num1) && str_to_num(*it2, 10, num2)) {
            cmp = static_cast<int>(num1 - num2);
        } else {
            cmp = it1->compare(*it2);
        }

        if (cmp != 0) {
            return cmp;
        }
    }

    if (it1 == split1.end() && it2 == split2.end()) {
        return 0;
    } else if (it1 == split1.end()) {
        return -1;
    } else {
        return 1;
    }
}

}
