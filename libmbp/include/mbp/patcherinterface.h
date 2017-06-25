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

#pragma once

#include "mbcommon/common.h"

#include "errors.h"
#include "fileinfo.h"


namespace mbp
{

/*!
 * \class Patcher
 * \brief Handles the patching of zip files and boot images
 *
 * This is the main class for the patching of files. All of the patching
 * operations are performed here and the current patching progress is reported
 * from this class as well.
 */
class MB_EXPORT Patcher
{
public:
    typedef void (*ProgressUpdatedCallback) (uint64_t, uint64_t, void *);
    typedef void (*FilesUpdatedCallback) (uint64_t, uint64_t, void *);
    typedef void (*DetailsUpdatedCallback) (const std::string &, void *);

    virtual ~Patcher() {}

    /*!
     * \brief The error
     *
     * Returns a ErrorCode. The value is invalid if nothing has failed.
     *
     * \return ErrorCode error
     */
    virtual ErrorCode error() const = 0;

    /*!
     * \brief The patcher's identifier
     */
    virtual std::string id() const = 0;

    /*!
     * \brief Sets the FileInfo object corresponding to the file to patch
     */
    virtual void setFileInfo(const FileInfo * const info) = 0;

    /*!
     * \brief Start patching the file
     *
     * This method starts the patching operations for the current file. The
     * callback parameters can be passed nullptr if they are not needed.
     *
     * \param progressCb Callback for receiving current progress values
     * \param filesCb Callback for receiving current files count
     * \param detailsCb Callback for receiving detailed progress text
     * \param userData Pointer to pass to callback functions
     */
    virtual bool patchFile(ProgressUpdatedCallback progressCb,
                           FilesUpdatedCallback filesCb,
                           DetailsUpdatedCallback detailsCb,
                           void *userData) = 0;

    /*!
     * \brief Cancel the patching of a file
     *
     * This method allows the patching process to be cancelled. This is only
     * useful if the patching operation is being done on a thread.
     */
    virtual void cancelPatching() = 0;
};


/*!
 * \class AutoPatcher
 * \brief Handles common patching operations of files within a zip archive
 *
 * This is a helper class for the patching of zip files. This class is usually
 * called by a Patcher to perform common or repetitive patching tasks for files
 * within a zip archive.
 *
 * \sa Patcher
 */
class AutoPatcher
{
public:
    virtual ~AutoPatcher() {}

    /*!
     * \brief The error
     *
     * Returns a ErrorCode. The value is invalid if nothing has failed.
     *
     * \return ErrorCode error
     */
    virtual ErrorCode error() const = 0;

    /*!
     * \brief The autopatcher's identifier
     */
    virtual std::string id() const = 0;

    /*!
     * \brief List of new files added by the autopatcher
     *
     * \warning Currently unimplemented. A valid list returned by a child class
     *          will be ignored.
     */
    virtual std::vector<std::string> newFiles() const = 0;

    /*!
     * \brief List of existing files to be patched in the zip file
     */
    virtual std::vector<std::string> existingFiles() const = 0;

    /*!
     * \brief Start patching the file
     *
     * \param directory Directory containing the files to be patched
     */
    virtual bool patchFiles(const std::string &directory) = 0;
};

}
