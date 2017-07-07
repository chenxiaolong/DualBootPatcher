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

#include "mbpatcher/cwrapper/cpatcherinterface.h"

#include <cassert>

#include "mbpatcher/cwrapper/private/util.h"

#include "mbpatcher/patcherinterface.h"


#define CASTP(x) \
    assert(x != nullptr); \
    auto *p = reinterpret_cast<mb::patcher::Patcher *>(x);
#define CCASTP(x) \
    assert(x != nullptr); \
    auto const *p = reinterpret_cast<const mb::patcher::Patcher *>(x);
#define CASTAP(x) \
    assert(x != nullptr); \
    auto *ap = reinterpret_cast<mb::patcher::AutoPatcher *>(x);
#define CCASTAP(x) \
    assert(x != nullptr); \
    auto const *ap = reinterpret_cast<const mb::patcher::AutoPatcher *>(x);


/*!
 * \file cpatcherinterface.h
 * \brief C Wrapper for Patcher and AutoPatcher
 *
 * Please see the documentation for the patchers from the C++ API for more
 * details. The C functions directly correspond to the member functions of the
 * patcher classes.
 *
 * \sa Patcher, AutoPatcher
 */

extern "C" {

struct CallbackWrapper {
    ProgressUpdatedCallback progressCb;
    FilesUpdatedCallback filesCb;
    DetailsUpdatedCallback detailsCb;
    void *userData;
};
typedef struct CallbackWrapper CallbackWrapper;

void progressCbWrapper(uint64_t bytes, uint64_t maxBytes, void *userData)
{
    CallbackWrapper *wrapper = reinterpret_cast<CallbackWrapper *>(userData);
    if (wrapper->progressCb != nullptr) {
        wrapper->progressCb(bytes, maxBytes, wrapper->userData);
    }
}

void filesCbWrapper(uint64_t files, uint64_t maxFiles, void *userData)
{
    CallbackWrapper *wrapper = reinterpret_cast<CallbackWrapper *>(userData);
    if (wrapper->filesCb) {
        wrapper->filesCb(files, maxFiles, userData);
    }
}

void detailsCbWrapper(const std::string &text, void *userData)
{
    CallbackWrapper *wrapper = reinterpret_cast<CallbackWrapper *>(userData);
    if (wrapper->detailsCb) {
        wrapper->detailsCb(text.c_str(), wrapper->userData);
    }
}

/*!
 * \brief Get the error information
 *
 * \note The returned ErrorCode is filled with valid data only if a
 *       CPatcher operation has failed.
 *
 * \note The returned ErrorCode should be freed with mbpatcher_error_destroy()
 *       when it is no longer needed.
 *
 * \param patcher CPatcher object
 *
 * \return ErrorCode
 *
 * \sa Patcher::error()
 */
/* enum ErrorCode */ int mbpatcher_patcher_error(const CPatcher *patcher)
{
    CCASTP(patcher);
    return static_cast<int>(p->error());
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
char * mbpatcher_patcher_id(const CPatcher *patcher)
{
    CCASTP(patcher);
    return string_to_cstring(p->id());
}

/*!
 * \brief Sets the FileInfo object corresponding to the file to patch
 *
 * \param patcher CPatcher object
 * \param info CFileInfo
 *
 * \sa Patcher::setFileInfo()
 */
void mbpatcher_patcher_set_fileinfo(CPatcher *patcher, const CFileInfo *info)
{
    CASTP(patcher);
    p->setFileInfo(reinterpret_cast<const mb::patcher::FileInfo *>(info));
}

/*!
 * \brief Start patching the file
 *
 * \param patcher CPatcher object
 * \param progressCb Callback for receiving current progress value
 * \param detailsCb Callback for receiving detailed progress text
 * \param userData Pointer to pass to callback functions
 * \return true on success, otherwise false (and error set appropriately)
 *
 * \sa Patcher::patchFile()
 */
bool mbpatcher_patcher_patch_file(CPatcher *patcher,
                                  ProgressUpdatedCallback progressCb,
                                  FilesUpdatedCallback filesCb,
                                  DetailsUpdatedCallback detailsCb,
                                  void *userData)
{
    CASTP(patcher);

    CallbackWrapper wrapper;
    wrapper.progressCb = progressCb;
    wrapper.filesCb = filesCb;
    wrapper.detailsCb = detailsCb;
    wrapper.userData = userData;

    return p->patchFile(&progressCbWrapper, &filesCbWrapper, &detailsCbWrapper,
                        reinterpret_cast<void *>(&wrapper));
}

/*!
 * \brief Cancel the patching of a file
 *
 * \param patcher CPatcher object
 *
 * \sa Patcher::cancelPatching()
 */
void mbpatcher_patcher_cancel_patching(CPatcher *patcher)
{
    CASTP(patcher);
    p->cancelPatching();
}

/*!
 * \brief Get the error information
 *
 * \note The returned ErrorCode is filled with valid data only if a
 *       CAutoPatcher operation has failed.
 *
 * \note The returned ErrorCode should be freed with mbpatcher_error_destroy()
 *       when it is no longer needed.
 *
 * \param patcher CAutoPatcher object
 *
 * \return ErrorCode
 *
 * \sa AutoPatcher::error()
 */
/* enum ErrorCode */ int mbpatcher_autopatcher_error(const CAutoPatcher *patcher)
{
    CCASTAP(patcher);
    return static_cast<int>(ap->error());
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
char * mbpatcher_autopatcher_id(const CAutoPatcher *patcher)
{
    CCASTAP(patcher);
    return string_to_cstring(ap->id());
}

/*!
 * \brief List of new files added by the autopatcher
 *
 * \note The returned array should be freed with `mbpatcher_free_array()` when
 *       it is no longer needed.
 *
 * \param patcher CAutoPatcher object
 * \return A NULL-terminated array containing the files
 *
 * \sa AutoPatcher::newFiles()
 */
char ** mbpatcher_autopatcher_new_files(const CAutoPatcher *patcher)
{
    CCASTAP(patcher);
    return vector_to_cstring_array(ap->newFiles());
}

/*!
 * \brief List of existing files to be patched in the zip file
 *
 * \note The returned array should be freed with `mbpatcher_free_array()` when
 *       it is no longer needed.
 *
 * \param patcher CAutoPatcher object
 * \return A NULL-terminated array containing the files
 *
 * \sa AutoPatcher::existingFiles()
 */
char ** mbpatcher_autopatcher_existing_files(const CAutoPatcher *patcher)
{
    CCASTAP(patcher);
    return vector_to_cstring_array(ap->existingFiles());
}

/*!
 * \brief Start patching the file
 *
 * \param patcher CAutoPatcher object
 * \param directory Directory containing the files to be patched
 * \return true on success, otherwise false (and error set appropriately)
 *
 * \sa AutoPatcher::patchFiles()
 */
bool mbpatcher_autopatcher_patch_files(CAutoPatcher *patcher,
                                       const char *directory)
{
    CASTAP(patcher);
    return ap->patchFiles(directory);
}

}
