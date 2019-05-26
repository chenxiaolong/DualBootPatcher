/*
 * Copyright (C) 2014-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/capi/util.h"

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

extern "C"
{

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
    return mb::capi_str_to_cstr(p->id());
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
    p->set_file_info(reinterpret_cast<const mb::patcher::FileInfo *>(info));
}

/*!
 * \brief Start patching the file
 *
 * \param patcher CPatcher object
 * \param progress_cb Callback for receiving current progress value
 * \param files_cb Callback for receiving current files count
 * \param details_cb Callback for receiving detailed progress text
 * \param userdata Pointer to pass to callback functions
 * \return true on success, otherwise false (and error set appropriately)
 *
 * \sa Patcher::patchFile()
 */
bool mbpatcher_patcher_patch_file(CPatcher *patcher,
                                  ProgressUpdatedCallback progress_cb,
                                  FilesUpdatedCallback files_cb,
                                  DetailsUpdatedCallback details_cb,
                                  void *userdata)
{
    CASTP(patcher);

    return p->patch_file(
        [&](uint64_t bytes, uint64_t max_bytes) {
            if (progress_cb) {
                progress_cb(bytes, max_bytes, userdata);
            }
        },
        [&](uint64_t files, uint64_t max_files) {
            if (files_cb) {
                files_cb(files, max_files, userdata);
            }
        },
        [&](const std::string &text) {
            if (details_cb) {
                details_cb(text.c_str(), userdata);
            }
        }
    );
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
    p->cancel_patching();
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
    return mb::capi_str_to_cstr(ap->id());
}

/*!
 * \brief List of new files added by the autopatcher
 *
 * \note The returned array and its contents should be freed with `free()` when
 *       they are no longer needed.
 *
 * \param patcher CAutoPatcher object
 * \return A NULL-terminated array containing the files
 *
 * \sa AutoPatcher::newFiles()
 */
char ** mbpatcher_autopatcher_new_files(const CAutoPatcher *patcher)
{
    CCASTAP(patcher);
    return mb::capi_vector_to_cstr_array(ap->new_files());
}

/*!
 * \brief List of existing files to be patched in the zip file
 *
 * \note The returned array and its contents should be freed with `free()` when
 *       they are no longer needed.
 *
 * \param patcher CAutoPatcher object
 * \return A NULL-terminated array containing the files
 *
 * \sa AutoPatcher::existingFiles()
 */
char ** mbpatcher_autopatcher_existing_files(const CAutoPatcher *patcher)
{
    CCASTAP(patcher);
    return mb::capi_vector_to_cstr_array(ap->existing_files());
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
    return ap->patch_files(directory);
}

}
