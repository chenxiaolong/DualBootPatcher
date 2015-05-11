/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "cwrapper/cfileinfo.h"

#include <cassert>

#include <cwrapper/private/util.h>

#include "fileinfo.h"


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
 * \sa FileInfo::filename()
 */
char * mbp_fileinfo_filename(const CFileInfo *info)
{
    CCAST(info);
    return string_to_cstring(fi->filename());
}

/*!
 * \brief Set file to be patched
 *
 * \param info CFileInfo object
 * \param path File path
 *
 * \sa FileInfo::setFilename()
 */
void mbp_fileinfo_set_filename(CFileInfo *info, const char *path)
{
    CAST(info);
    fi->setFilename(path);
}

/*!
 * \brief PatchInfo used for patching the file
 *
 * \param info CFileInfo object
 *
 * \return PatchInfo object
 *
 * \sa FileInfo::patchInfo()
 */
CPatchInfo * mbp_fileinfo_patchinfo(const CFileInfo *info)
{
    CCAST(info);
    return reinterpret_cast<CPatchInfo *>(fi->patchInfo());
}

/*!
 * \brief Set PatchInfo to be used for patching
 *
 * \param info CFileInfo object
 * \param pInfo PatchInfo object
 *
 * \sa FileInfo::setPatchInfo()
 */
void mbp_fileinfo_set_patchinfo(CFileInfo *info, CPatchInfo *pInfo)
{
    CAST(info);
    fi->setPatchInfo(reinterpret_cast<mbp::PatchInfo *>(pInfo));
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
CDevice * mbp_fileinfo_device(const CFileInfo *info)
{
    CCAST(info);
    return reinterpret_cast<CDevice *>(fi->device());
}

/*!
 * \brief Set target device
 *
 * \param info CFileInfo object
 * \param device Target device
 *
 * \sa FileInfo::setDevice()
 */
void mbp_fileinfo_set_device(CFileInfo *info, CDevice *device)
{
    CAST(info);
    fi->setDevice(reinterpret_cast<mbp::Device *>(device));
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
