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

#include "mbbootimg/format/sony_elf_error.h"

#include <string>

namespace mb::bootimg::sonyelf
{

struct SonyElfErrorCategory : std::error_category
{
    const char * name() const noexcept override;

    std::string message(int ev) const override;
};

const std::error_category & sony_elf_error_category()
{
    static SonyElfErrorCategory c;
    return c;
}

std::error_code make_error_code(SonyElfError e)
{
    return {static_cast<int>(e), sony_elf_error_category()};
}

const char * SonyElfErrorCategory::name() const noexcept
{
    return "sony_elf_format";
}

std::string SonyElfErrorCategory::message(int ev) const
{
    switch (static_cast<SonyElfError>(ev)) {
    case SonyElfError::SonyElfHeaderTooSmall:
        return "Sony ELF header too small";
    case SonyElfError::InvalidElfMagic:
        return "invalid ELF magic";
    case SonyElfError::KernelCmdlineTooLong:
        return "kernel cmdline too long";
    case SonyElfError::InvalidTypeOrFlagsField:
        return "invalid type or flags field";
    default:
        return "(unknown Sony ELF format error)";
    }
}

}
