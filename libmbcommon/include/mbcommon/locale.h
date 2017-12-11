/*
 * Copyright (C) 2016-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/common.h"

#include <string>

#include "mbcommon/outcome.h"

namespace mb
{

MB_EXPORT oc::result<std::wstring> mbs_to_wcs_n(const char *str, size_t len);
MB_EXPORT oc::result<std::string> wcs_to_mbs_n(const wchar_t *str, size_t len);
MB_EXPORT oc::result<std::wstring> utf8_to_wcs_n(const char *str, size_t len);
MB_EXPORT oc::result<std::string> wcs_to_utf8_n(const wchar_t *str, size_t len);

MB_EXPORT oc::result<std::wstring> mbs_to_wcs(const char *str);
MB_EXPORT oc::result<std::wstring> mbs_to_wcs(const std::string &str);
MB_EXPORT oc::result<std::string> wcs_to_mbs(const wchar_t *str);
MB_EXPORT oc::result<std::string> wcs_to_mbs(const std::wstring &str);
MB_EXPORT oc::result<std::wstring> utf8_to_wcs(const char *str);
MB_EXPORT oc::result<std::wstring> utf8_to_wcs(const std::string &str);
MB_EXPORT oc::result<std::string> wcs_to_utf8(const wchar_t *str);
MB_EXPORT oc::result<std::string> wcs_to_utf8(const std::wstring &str);

}
