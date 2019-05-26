/*
 * Copyright (C) 2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbbootimg/format/loki_error.h"

#include <string>

namespace mb::bootimg::loki
{

struct LokiErrorCategory : std::error_category
{
    const char * name() const noexcept override;

    std::string message(int ev) const override;
};

const std::error_category & loki_error_category()
{
    static LokiErrorCategory c;
    return c;
}

std::error_code make_error_code(LokiError e)
{
    return {static_cast<int>(e), loki_error_category()};
}

const char * LokiErrorCategory::name() const noexcept
{
    return "loki_format";
}

std::string LokiErrorCategory::message(int ev) const
{
    switch (static_cast<LokiError>(ev)) {
    case LokiError::PageSizeCannotBeZero:
        return "page size cannot be zero";
    case LokiError::LokiHeaderTooSmall:
        return "Loki header is too small";
    case LokiError::InvalidLokiMagic:
        return "invalid Loki magic";
    case LokiError::InvalidKernelAddress:
        return "invalid kernel address";
    case LokiError::RamdiskOffsetGreaterThanAbootOffset:
        return "ramdisk offset greater than aboot offset";
    case LokiError::FailedToDetermineRamdiskSize:
        return "failed to determine ramdisk size";
    case LokiError::NoRamdiskGzipHeaderFound:
        return "no ramdisk gzip header found";
    case LokiError::ShellcodeNotFound:
        return "Loki shellcode not found";
    case LokiError::ShellcodePatchFailed:
        return "failed to patch Loki shellcode";
    case LokiError::AbootImageTooSmall:
        return "aboot image is too small";
    case LokiError::AbootImageTooLarge:
        return "aboot image is too large";
    case LokiError::UnsupportedAbootImage:
        return "unsupported aboot image";
    case LokiError::AbootFunctionNotFound:
        return "failed to find aboot function";
    case LokiError::AbootFunctionOutOfRange:
        return "aboot function out of range";
    default:
        return "(unknown Loki format error)";
    }
}

}
