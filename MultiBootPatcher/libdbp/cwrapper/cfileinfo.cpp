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

#include "cwrapper/cfileinfo.h"

#include <cstdlib>
#include <cstring>

#include "fileinfo.h"


/*!
 * \file cfileinfo.h
 * \brief C Wrapper for FileInfo
 *
 * Please see the documentation for FileInfo from the C++ API for more details.
 * The C functions directly correspond to the Fileinfo member functions.
 *
 * \sa FileInfo
 */

/*!
 * \typedef CFileInfo
 * \brief C wrapper for FileInfo object
 */

extern "C" {

    /*!
     * \brief Create a new FileInfo object.
     *
     * \note The returned object must be freed with mbp_fileinfo_destroy().
     *
     * \return New CFileInfo
     */
    CFileInfo * mbp_fileinfo_create()
    {
        return reinterpret_cast<CFileInfo *>(new FileInfo());
    }

    /*!
     * \brief Destroys a CFileInfo object.
     *
     * \param info CFileInfo to destroy
     */
    void mbp_fileinfo_destroy(CFileInfo *info)
    {
        delete reinterpret_cast<FileInfo *>(info);
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
        const FileInfo *fi = reinterpret_cast<const FileInfo *>(info);
        return strdup(fi->filename().c_str());
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
        FileInfo *fi = reinterpret_cast<FileInfo *>(info);
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
        const FileInfo *fi = reinterpret_cast<const FileInfo *>(info);
        CPatchInfo *cpi = reinterpret_cast<CPatchInfo *>(fi->patchInfo());
        return cpi;
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
        FileInfo *fi = reinterpret_cast<FileInfo *>(info);
        PatchInfo *pi = reinterpret_cast<PatchInfo *>(pInfo);
        fi->setPatchInfo(pi);
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
        const FileInfo *fi = reinterpret_cast<const FileInfo *>(info);
        CDevice *device = reinterpret_cast<CDevice *>(fi->device());
        return device;
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
        FileInfo *fi = reinterpret_cast<FileInfo *>(info);
        Device *d = reinterpret_cast<Device *>(device);
        fi->setDevice(d);
    }

    /*!
     * \brief Target partition configuration
     *
     * \param info CFileInfo object
     *
     * \return Target PartitionConfig
     *
     * \sa FileInfo::partConfig()
     */
    CPartConfig * mbp_fileinfo_partconfig(const CFileInfo *info)
    {
        const FileInfo *fi = reinterpret_cast<const FileInfo *>(info);
        CPartConfig *cpc = reinterpret_cast<CPartConfig *>(fi->partConfig());
        return cpc;
    }

    /*!
     * \brief Set target partition configuration
     *
     * \param info CFileInfo object
     * \param config Target PartitionConfig
     *
     * \sa FileInfo::setPartConfig()
     */
    void mbp_fileinfo_set_partconfig(CFileInfo *info, CPartConfig *config)
    {
        FileInfo *fi = reinterpret_cast<FileInfo *>(info);
        PartitionConfig *pc = reinterpret_cast<PartitionConfig *>(config);
        fi->setPartConfig(pc);
    }

}
