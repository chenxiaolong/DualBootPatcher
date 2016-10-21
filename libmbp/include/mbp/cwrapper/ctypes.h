/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#ifdef __cplusplus
extern "C" {
#endif

struct CBootImage;
typedef struct CBootImage CBootImage;

struct CCpioFile;
typedef struct CCpioFile CCpioFile;

struct CFileInfo;
typedef struct CFileInfo CFileInfo;

struct CPatcherConfig;
typedef struct CPatcherConfig CPatcherConfig;

struct CPatcher;
typedef struct CPatcher CPatcher;
struct CAutoPatcher;
typedef struct CAutoPatcher CAutoPatcher;
struct CRamdiskPatcher;
typedef struct CRamdiskPatcher CRamdiskPatcher;

#ifdef __cplusplus
}
#endif
