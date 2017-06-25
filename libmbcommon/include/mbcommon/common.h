/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#ifdef _WIN32
#  if defined(MB_DYNAMIC_LINK)
#    if defined(MB_LIBRARY)
#      define MB_EXPORT __declspec(dllexport)
#    else
#      define MB_EXPORT __declspec(dllimport)
#    endif
#  else
#    define MB_EXPORT
#  endif
#endif

#ifndef MB_EXPORT
#  if defined(__GNUC__)
#    define MB_EXPORT __attribute__ ((visibility ("default")))
#  else
#    define MB_EXPORT
#  endif
#endif

#ifdef _MSC_VER
#  pragma warning(disable:4251) // class ... needs to have dll-interface ...
#endif

#ifdef _MSC_VER
#  define strdup _strdup
#endif

#ifdef __cplusplus
#  define MB_BEGIN_C_DECLS extern "C" {
#  define MB_END_C_DECLS }
#else
#  define MB_BEGIN_C_DECLS
#  define MB_END_C_DECLS
#endif

#if defined(__GNUC__) && defined(__clang__)
#  define MB_PRINTF(fmt_arg, var_arg) \
    __attribute__((format(printf, fmt_arg, var_arg)))
#  define MB_SCANF(fmt_arg, var_arg) \
    __attribute__((format(scanf, fmt_arg, var_arg)))
#  define MB_UNUSED __attribute__((unused))
#  define MB_NO_RETURN __attribute__((noreturn))
#else
#  define MB_PRINTF(fmtarg, firstvararg)
#  define MB_SCANF(fmtarg, firstvararg)
#  define MB_UNUSED
#  define MB_NO_RETURN
#endif
