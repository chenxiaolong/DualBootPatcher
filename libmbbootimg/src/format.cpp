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

#include "mbbootimg/format.h"

#include <array>

/*!
 * \namespace mb::bootimg
 *
 * \brief libmbbootimg namespace
 */

/*!
 * \file mbbootimg/format.h
 * \brief Boot image formats
 */

namespace mb::bootimg
{

// Formats documentation

/*!
 * \enum Format
 *
 * \brief Boot image formats
 */

/*!
 * \var Format::Android
 *
 * \brief Format code for Android boot images
 */

/*!
 * \var Format::Bump
 *
 * \brief Format code for Bump'ed Android boot images
 */

/*!
 * \var Format::Loki
 *
 * \brief Format code for Loki'd Android boot images
 */

/*!
 * \var Format::Mtk
 *
 * \brief Format code for MTK-style Android boot images
 */

/*!
 * \var Format::SonyElf
 *
 * \brief Format code for Sony ELF boot images
 */

/*!
 * \class Formats
 *
 * \brief Bitmask of Format variants
 */

/*!
 * \var ALL_FORMATS
 *
 * \brief Bitmask of all supported formats
 *
 * Note that this Formats object can be iterated over to get each supported
 * \ref Format variant.
 */

/*!
 * \brief Convert format to string
 *
 * \param format Format to convert
 *
 * \return Name of format
 */
std::string_view format_to_name(Format format)
{
    switch (format) {
        case Format::Android:
            return "android";
        case Format::Bump:
            return "bump";
        case Format::Loki:
            return "loki";
        case Format::Mtk:
            return "mtk";
        case Format::SonyElf:
            return "sony_elf";
        default:
            MB_UNREACHABLE("Invalid format");
    }
}

/*!
 * \brief Convert name to format
 *
 * \param name Name to convert
 *
 * \return The format, if the name is valid
 */
std::optional<Format> name_to_format(std::string_view name)
{
    if (name == "android") {
        return Format::Android;
    } else if (name == "bump") {
        return Format::Bump;
    } else if (name == "loki") {
        return Format::Loki;
    } else if (name == "mtk") {
        return Format::Mtk;
    } else if (name == "sony_elf") {
        return Format::SonyElf;
    } else {
        return std::nullopt;
    }
}

}
