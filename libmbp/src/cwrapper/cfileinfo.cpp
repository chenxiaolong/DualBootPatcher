/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbp/cwrapper/cfileinfo.h"

#include <cassert>

#include "mbp/cwrapper/private/util.h"

#include "mbp/fileinfo.h"


#define CAST(x) \
    assert(x != nullptr); \
    mbp::FileInfo *fi = reinterpret_cast<mbp::FileInfo *>(x);
#define CCAST(x) \
    assert(x != nullptr); \
    const mbp::FileInfo *fi = reinterpret_cast<const mbp::FileInfo *>(x);


/*!
 * \file cfileinfo.h
 * \brief C Wrapper for FileInfo
 *
 * Please see the documentation for FileInfo from the C++ API for more details.
 * The C functions directly correspond to the Fileinfo member functions.
 *
 * \sa FileInfo
 */

extern "C" {

/*!
 * \brief Create a new FileInfo object.
 *
 * \note The returned object must be freed with mbp_fileinfo_destroy().
 *
 * \return New CFileInfo
 */
CFileInfo * mbp_fileinfo_create(void)
{
    return reinterpret_cast<CFileInfo *>(new mbp::FileInfo());
}

/*!
 * \brief Destroys a CFileInfo object.
 *
 * \param info CFileInfo to destroy
 */
void mbp_fileinfo_destroy(CFileInfo *info)
{
    CAST(info);
    delete fi;
}

/*!
 * \brief File to be patched
 *
 * \param info CFileInfo object
 *
 * \note The returned string is dynamically allocated. It should be free()'d
 *       when it is no longer needed.
 *
 * \return File path
 *
 * \sa FileInfo::inputPath()
 */
char * mbp_fileinfo_input_path(const CFileInfo *info)
{
    CCAST(info);
    return string_to_cstring(fi->inputPath());
}

/*!
 * \brief Set file to be patched
 *
 * \param info CFileInfo object
 * \param path File path
 *
 * \sa FileInfo::setInputPath()
 */
void mbp_fileinfo_set_input_path(CFileInfo *info, const char *path)
{
    CAST(info);
    fi->setInputPath(path);
}

char * mbp_fileinfo_output_path(const CFileInfo *info)
{
    CCAST(info);
    return string_to_cstring(fi->outputPath());
}

void mbp_fileinfo_set_output_path(CFileInfo *info, const char *path)
{
    CAST(info);
    fi->setOutputPath(path);
}

/*!
 * \brief Target device
 *
 * \param info CFileInfo object
 *
 * \return Device
 *
 * \sa FileInfo::device()
 */
struct Device * mbp_fileinfo_device(const CFileInfo *info)
{
    CCAST(info);
    return fi->device();
}

/*!
 * \brief Set target device
 *
 * \param info CFileInfo object
 * \param device Target device
 *
 * \sa FileInfo::setDevice()
 */
void mbp_fileinfo_set_device(CFileInfo *info, struct Device *device)
{
    CAST(info);
    fi->setDevice(device);
}

char * mbp_fileinfo_rom_id(const CFileInfo *info)
{
    CCAST(info);
    return string_to_cstring(fi->romId());
}

void mbp_fileinfo_set_rom_id(CFileInfo *info, const char *id)
{
    CAST(info);
    fi->setRomId(id);
}

}
