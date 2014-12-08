/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#ifdef _WIN32
#  if defined(LIBMBP_LIBRARY)
#    define MBP_EXPORT __declspec(dllexport)
#  else
#    define MBP_EXPORT __declspec(dllimport)
#  endif
#endif

#ifndef MBP_EXPORT
#  if defined(__GNUC__)
#    define MBP_EXPORT __attribute__ ((visibility ("default")))
#  else
#    define MBP_EXPORT
#  endif
#endif

#ifdef _MSC_VER
#  pragma warning(disable:4251) // class ... needs to have dll-interface ...
#endif

#ifdef _MSC_VER
#  define strdup _strdup
#endif
