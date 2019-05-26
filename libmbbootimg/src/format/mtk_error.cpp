/*
 * Copyright (C) 2017-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbbootimg/format/mtk_error.h"

#include <string>

namespace mb::bootimg::mtk
{

struct MtkErrorCategory : std::error_category
{
    const char * name() const noexcept override;

    std::string message(int ev) const override;
};

const std::error_category & mtk_error_category()
{
    static MtkErrorCategory c;
    return c;
}

std::error_code make_error_code(MtkError e)
{
    return {static_cast<int>(e), mtk_error_category()};
}

const char * MtkErrorCategory::name() const noexcept
{
    return "mtk_format";
}

std::string MtkErrorCategory::message(int ev) const
{
    switch (static_cast<MtkError>(ev)) {
    case MtkError::MtkHeaderNotFound:
        return "MTK header not found";
    case MtkError::MismatchedKernelSizeInHeaders:
        return "mismatched kernel size in Android and MTK headers";
    case MtkError::MismatchedRamdiskSizeInHeaders:
        return "mismatched ramdisk size in Android and MTK headers";
    case MtkError::EntryTooLargeToFitMtkHeader:
        return "entry size too large to fit MTK header";
    case MtkError::InvalidEntrySizeForMtkHeader:
        return "invalid entry size for MTK header";
    case MtkError::MtkHeaderOffsetTooLarge:
        return "MTK header offset too large";
    default:
        return "(unknown MTK format error)";
    }
}

}
