/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#ifdef __cplusplus
#  include <cstddef>
#else
#  include <stddef.h>
#endif

MB_BEGIN_C_DECLS

MB_EXPORT wchar_t * mb_mbs_to_wcs(const char *str);
MB_EXPORT char * mb_wcs_to_mbs(const wchar_t *str);
MB_EXPORT wchar_t * mb_utf8_to_wcs(const char *str);
MB_EXPORT char * mb_wcs_to_utf8(const wchar_t *str);

MB_EXPORT wchar_t * mb_mbs_to_wcs_len(const char *str, size_t size);
MB_EXPORT char * mb_wcs_to_mbs_len(const wchar_t *str, size_t size);
MB_EXPORT wchar_t * mb_utf8_to_wcs_len(const char *str, size_t size);
MB_EXPORT char * mb_wcs_to_utf8_len(const wchar_t *str, size_t size);

MB_END_C_DECLS

#ifdef __cplusplus
namespace mb
{

MB_EXPORT inline wchar_t * mbs_to_wcs(const char *str)
{
    return mb_mbs_to_wcs(str);
}

MB_EXPORT inline char * wcs_to_mbs(const wchar_t *str)
{
    return mb_wcs_to_mbs(str);
}

MB_EXPORT inline wchar_t * utf8_to_wcs(const char *str)
{
    return mb_utf8_to_wcs(str);
}

MB_EXPORT inline char * wcs_to_utf8(const wchar_t *str)
{
    return mb_wcs_to_utf8(str);
}

MB_EXPORT inline wchar_t * mbs_to_wcs_len(const char *str, size_t size)
{
    return mb_mbs_to_wcs_len(str, size);
}

MB_EXPORT inline char * wcs_to_mbs_len(const wchar_t *str, size_t size)
{
    return mb_wcs_to_mbs_len(str, size);
}

MB_EXPORT inline wchar_t * utf8_to_wcs_len(const char *str, size_t size)
{
    return mb_utf8_to_wcs_len(str, size);
}

MB_EXPORT inline char * wcs_to_utf8_len(const wchar_t *str, size_t size)
{
    return mb_wcs_to_utf8_len(str, size);
}

}
#endif
