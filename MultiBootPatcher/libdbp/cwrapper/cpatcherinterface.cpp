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

#include "cwrapper/cpatcherinterface.h"

#include <cstdlib>
#include <cstring>

#include "patcherinterface.h"


/*!
 * \file cpatcherinterface.h
 * \brief C Wrapper for Patcher, AutoPatcher, and RamdiskPatcher
 *
 * Please see the documentation for the patchers from the C++ API for more
 * details. The C functions directly correspond to the member functions of the
 * patcher classes.
 *
 * \sa Patcher, AutoPatcher, RamdiskPatcher
 */

extern "C" {

    struct CallbackWrapper {
        MaxProgressUpdatedCallback maxProgressCb;
        ProgressUpdatedCallback progressCb;
        DetailsUpdatedCallback detailsCb;
        void *userData;
    };
    typedef struct CallbackWrapper CallbackWrapper;

    void maxProgressCbWrapper(int value, void *userData) {
        CallbackWrapper *wrapper = reinterpret_cast<CallbackWrapper *>(userData);
        if (wrapper->maxProgressCb != nullptr) {
            wrapper->maxProgressCb(value, wrapper->userData);
        }
    }

    void progressCbWrapper(int value, void *userData) {
        CallbackWrapper *wrapper = reinterpret_cast<CallbackWrapper *>(userData);
        if (wrapper->progressCb != nullptr) {
            wrapper->progressCb(value, wrapper->userData);
        }
    }

    void detailsCbWrapper(const std::string &text, void *userData) {
        CallbackWrapper *wrapper = reinterpret_cast<CallbackWrapper *>(userData);
        if (wrapper->detailsCb != nullptr) {
            wrapper->detailsCb(text.c_str(), wrapper->userData);
        }
    }

    /*!
     * \brief Get the error information
     *
     * \note The returned CPatcherError is filled with valid data only if a
     *       CPatcher operation has failed.
     *
     * \note The returned CPatcherError should be freed with mbp_error_destroy()
     *       when it is no longer needed.
     *
     * \param patcher CPatcher object
     *
     * \return CPatcherError
     *
     * \sa Patcher::error()
     */
    CPatcherError * mbp_patcher_error(const CPatcher *patcher)
    {
        const Patcher *p = reinterpret_cast<const Patcher *>(patcher);
        PatcherError *pe = new PatcherError(p->error());
        return reinterpret_cast<CPatcherError *>(pe);
    }

    /*!
     * \brief The patcher's identifier
     *
     * \note The returned string is dynamically allocated. It should be free()'d
     *       when it is no longer needed.
     *
     * \param patcher CPatcher object
     * \return Patcher ID
     *
     * \sa Patcher::id()
     */
    char * mbp_patcher_id(const CPatcher *patcher)
    {
        const Patcher *p = reinterpret_cast<const Patcher *>(patcher);
        return strdup(p->id().c_str());
    }

    /*!
     * \brief The patcher's friendly name
     *
     * \note The returned string is dynamically allocated. It should be free()'d
     *       when it is no longer needed.
     *
     * \param patcher CPatcher object
     * \return Patcher name
     *
     * \sa Patcher::name()
     */
    char * mbp_patcher_name(const CPatcher *patcher)
    {
        const Patcher *p = reinterpret_cast<const Patcher *>(patcher);
        return strdup(p->name().c_str());
    }

    /*!
     * \brief Whether or not the patcher uses patchinfo files
     *
     * \param patcher CPatcher object
     * \return Whether the patcher uses patchinfo files
     *
     * \sa Patcher::usesPatchInfo()
     */
    bool mbp_patcher_uses_patchinfo(const CPatcher *patcher)
    {
        const Patcher *p = reinterpret_cast<const Patcher *>(patcher);
        return p->usesPatchInfo();
    }

    /*!
     * \brief List of supported partition configuration IDs
     *
     * \note The returned array should be freed with `mbp_free_array()` when it
     *       is no longer needed.
     *
     * \param patcher CPatcher object
     * \return A NULL-terminated array containing the IDs
     *
     * \sa Patcher::supportedPartConfigIds()
     */
    char ** mbp_patcher_supported_partconfig_ids(const CPatcher *patcher)
    {
        const Patcher *p = reinterpret_cast<const Patcher *>(patcher);
        auto const ids = p->supportedPartConfigIds();

        char **cIds = (char **) std::malloc(
                sizeof(char *) * (ids.size() + 1));
        for (unsigned int i = 0; i < ids.size(); ++i) {
            cIds[i] = strdup(ids[i].c_str());
        }
        cIds[ids.size()] = nullptr;

        return cIds;
    }

    /*!
     * \brief Sets the FileInfo object corresponding to the file to patch
     *
     * \param patcher CPatcher object
     * \param info CFileInfo
     *
     * \sa Patcher::setFileInfo()
     */
    void mbp_patcher_set_fileinfo(CPatcher *patcher, const CFileInfo *info)
    {
        Patcher *p = reinterpret_cast<Patcher *>(patcher);
        const FileInfo *fi = reinterpret_cast<const FileInfo *>(info);
        p->setFileInfo(fi);
    }

    /*!
     * \brief The path of the newly patched file
     *
     * \param patcher CPatcher object
     * \return Path to new file
     *
     * \sa Patcher::newFilePath()
     */
    char * mbp_patcher_new_file_path(CPatcher *patcher)
    {
        Patcher *p = reinterpret_cast<Patcher *>(patcher);
        return strdup(p->newFilePath().c_str());
    }

    /*!
     * \brief Start patching the file
     *
     * \param patcher CPatcher object
     * \param maxProgressCb Callback for receiving maximum progress value
     * \param progressCb Callback for receiving current progress value
     * \param detailsCb Callback for receiving detailed progress text
     * \param userData Pointer to pass to callback functions
     * \return 0 on success, otherwise -1 (and error set appropriately)
     *
     * \sa Patcher::patchFile()
     */
    int mbp_patcher_patch_file(CPatcher *patcher,
                               MaxProgressUpdatedCallback maxProgressCb,
                               ProgressUpdatedCallback progressCb,
                               DetailsUpdatedCallback detailsCb,
                               void *userData)
    {
        Patcher *p = reinterpret_cast<Patcher *>(patcher);

        CallbackWrapper wrapper;
        wrapper.maxProgressCb = maxProgressCb;
        wrapper.progressCb = progressCb;
        wrapper.detailsCb = detailsCb;
        wrapper.userData = userData;

        bool ret = p->patchFile(&maxProgressCbWrapper, &progressCbWrapper,
                                &detailsCbWrapper,
                                reinterpret_cast<void *>(&wrapper));
        return ret ? 0 : -1;
    }

    /*!
     * \brief Cancel the patching of a file
     *
     * \param patcher CPatcher object
     *
     * \sa Patcher::cancelPatching()
     */
    void mbp_patcher_cancel_patching(CPatcher *patcher)
    {
        Patcher *p = reinterpret_cast<Patcher *>(patcher);
        p->cancelPatching();
    }

    /*!
     * \brief Get the error information
     *
     * \note The returned CPatcherError is filled with valid data only if a
     *       CAutoPatcher operation has failed.
     *
     * \note The returned CPatcherError should be freed with mbp_error_destroy()
     *       when it is no longer needed.
     *
     * \param patcher CAutoPatcher object
     *
     * \return CPatcherError
     *
     * \sa AutoPatcher::error()
     */
    CPatcherError * mbp_autopatcher_error(const CAutoPatcher *patcher)
    {
        const AutoPatcher *p = reinterpret_cast<const AutoPatcher *>(patcher);
        PatcherError *pe = new PatcherError(p->error());
        return reinterpret_cast<CPatcherError *>(pe);
    }

    /*!
     * \brief The autopatcher's identifier
     *
     * \note The returned string is dynamically allocated. It should be free()'d
     *       when it is no longer needed.
     *
     * \param patcher CAutoPatcher object
     * \return AutoPatcher ID
     *
     * \sa AutoPatcher::id()
     */
    char * mbp_autopatcher_id(const CAutoPatcher *patcher)
    {
        const AutoPatcher *p = reinterpret_cast<const AutoPatcher *>(patcher);
        return strdup(p->id().c_str());
    }

    /*!
     * \brief List of new files added by the autopatcher
     *
     * \note The returned array should be freed with `mbp_free_array()` when it
     *       is no longer needed.
     *
     * \param patcher CAutoPatcher object
     * \return A NULL-terminated array containing the files
     *
     * \sa AutoPatcher::newFiles()
     */
    char ** mbp_autopatcher_new_files(const CAutoPatcher *patcher)
    {
        const AutoPatcher *p = reinterpret_cast<const AutoPatcher *>(patcher);
        auto const files = p->newFiles();

        char ** cFiles = (char **) std::malloc(
                sizeof(char *) * (files.size() + 1));
        for (unsigned int i = 0; i < files.size(); ++i) {
            cFiles[i] = strdup(files[i].c_str());
        }
        cFiles[files.size()] = nullptr;

        return cFiles;
    }

    /*!
     * \brief List of existing files to be patched in the zip file
     *
     * \note The returned array should be freed with `mbp_free_array()` when it
     *       is no longer needed.
     *
     * \param patcher CAutoPatcher object
     * \return A NULL-terminated array containing the files
     *
     * \sa AutoPatcher::existingFiles()
     */
    char ** mbp_autopatcher_existing_files(const CAutoPatcher *patcher)
    {
        const AutoPatcher *p = reinterpret_cast<const AutoPatcher *>(patcher);
        auto const files = p->existingFiles();

        char ** cFiles = (char **) std::malloc(
                sizeof(char *) * (files.size() + 1));
        for (unsigned int i = 0; i < files.size(); ++i) {
            cFiles[i] = strdup(files[i].c_str());
        }
        cFiles[files.size()] = nullptr;

        return cFiles;
    }

    /*!
     * \brief Start patching the file
     *
     * \param patcher CAutoPatcher object
     * \param directory Directory containing the files to be patched
     * \param bootImages List of boot image filenames in the zip file
     * \return 0 on success, otherwise -1 (and error set appropriately)
     *
     * \sa AutoPatcher::patchFiles()
     */
    int mbp_autopatcher_patch_files(CAutoPatcher *patcher, const char *directory,
                                    char **bootImages)
    {
        AutoPatcher *p = reinterpret_cast<AutoPatcher *>(patcher);

        std::vector<std::string> list;
        while (*bootImages != nullptr) {
            list.push_back(*bootImages);
            ++bootImages;
        }

        bool ret = p->patchFiles(directory, list);
        return ret ? 0 : -1;
    }

    /*!
     * \brief Get the error information
     *
     * \note The returned CPatcherError is filled with valid data only if a
     *       CRamdiskPatcher operation has failed.
     *
     * \note The returned CPatcherError should be freed with mbp_error_destroy()
     *       when it is no longer needed.
     *
     * \param patcher CRamdiskPatcher object
     *
     * \return CPatcherError
     *
     * \sa RamdiskPatcher::error()
     */
    CPatcherError * mbp_ramdiskpatcher_error(const CRamdiskPatcher *patcher)
    {
        const RamdiskPatcher *p = reinterpret_cast<const RamdiskPatcher *>(patcher);
        PatcherError *pe = new PatcherError(p->error());
        return reinterpret_cast<CPatcherError *>(pe);
    }

    /*!
     * \brief The ramdisk patchers's identifier
     *
     * \note The returned string is dynamically allocated. It should be free()'d
     *       when it is no longer needed.
     *
     * \param patcher CRamdiskPatcher object
     * \return RamdiskPatcher ID
     *
     * \sa RamdiskPatcher::id()
     */
    char * mbp_ramdiskpatcher_id(const CRamdiskPatcher *patcher)
    {
        const RamdiskPatcher *p = reinterpret_cast<const RamdiskPatcher *>(patcher);
        return strdup(p->id().c_str());
    }

    /*!
     * \brief Start patching the ramdisk
     *
     * \param patcher CRamdiskPatcher object
     * \return 0 on success, otherwise -1 (and error set appropriately)
     *
     * \sa RamdiskPatcher::patchRamdisk()
     */
    int mbp_ramdiskpatcher_patch_ramdisk(CRamdiskPatcher *patcher)
    {
        RamdiskPatcher *p = reinterpret_cast<RamdiskPatcher *>(patcher);
        bool ret = p->patchRamdisk();
        return ret ? 0 : -1;
    }

}
