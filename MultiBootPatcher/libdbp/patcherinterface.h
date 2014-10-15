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

#ifndef PATCHERINTERFACE_H
#define PATCHERINTERFACE_H

#include "libdbp_global.h"

#include "cpiofile.h"
#include "fileinfo.h"
#include "partitionconfig.h"
#include "patcherpaths.h"
#include "patchinfo.h"

#include <QtCore/QStringList>


/*!
    \class Patcher
    \brief Handles the patching of zip files and boot images

    This is the main class for the patching of files. All of the patching
    operations are performed here and the current patching progress is reported
    from this class as well.
 */
class LIBDBPSHARED_EXPORT Patcher : public QObject
{
    Q_OBJECT

public:
    explicit Patcher(QObject *parent = 0) : QObject(parent) {
    }

    virtual ~Patcher() {}

    /*!
        \brief The error code

        Returns error code from PatcherError::Error. The value is invalid if
        nothing has failed.

        \return PatcherError error code
     */
    virtual PatcherError::Error error() const = 0;

    /*!
        \brief The error message

        Returns a translated string corresponding to the the error code.

        \return Translated error message
     */
    virtual QString errorString() const = 0;

    /*!
        \brief The patcher's identifier
     */
    virtual QString id() const = 0;

    /*!
        \brief The patcher's friendly name
     */
    virtual QString name() const = 0;

    /*!
        \brief Whether or not the patcher uses patchinfo files
     */
    virtual bool usesPatchInfo() const = 0;

    /*!
        \brief List of supported partition configuration IDs
     */
    virtual QStringList supportedPartConfigIds() const = 0;

    /*!
        \brief Sets the FileInfo object corresponding to the file to patch
     */
    virtual void setFileInfo(const FileInfo * const info) = 0;

    /*!
        \brief The path of the newly patched file
     */
    virtual QString newFilePath() = 0;

    /*!
        \brief Start patching the file

        This method starts the patching operations for the current file. The
        maxProgressUpdated() and progressUpdated() signals can be emitted to
        indicate the overall progress of the patching operations and the
        detailsUpdated() signal can be emitted to provide more details on the
        current operation.
     */
    virtual bool patchFile() = 0;

public slots:
    /*!
        \brief Cancel the patching of a file

        This method allows the patching process to be cancelled. This is only
        useful if the patching operation is being done on a thread.
     */
    virtual void cancelPatching() = 0;

signals:
    void maxProgressUpdated(int max);
    void progressUpdated(int max);
    void detailsUpdated(const QString &details);
};


/*!
    \class AutoPatcher
    \brief Handles common patching operations of files within a zip archive

    This is a helper class for the patching of zip files. This class is usually
    called by a Patcher to perform common or repetitive patching tasks for files
    within a zip archive.

    \sa Patcher
 */
class AutoPatcher
{
public:
    virtual ~AutoPatcher() {}

    /*!
        \brief The error code

        Returns error code from PatcherError::Error. The value is invalid if
        nothing has failed.

        \return PatcherError error code
     */
    virtual PatcherError::Error error() const = 0;

    /*!
        \brief The error message

        Returns a translated string corresponding to the the error code.

        \return Translated error message
     */
    virtual QString errorString() const = 0;

    /*!
        \brief The autopatcher's identifier
     */
    virtual QString id() const = 0;

    /*!
        \brief List of new files added by the autopatcher

        \warning Currently unimplemented. A valid list returned by a child class
                 will be ignored.
     */
    virtual QStringList newFiles() const = 0;

    /*!
        \brief List of existing files to be patched in the zip file
     */
    virtual QStringList existingFiles() const = 0;

    /*!
        \brief Start patching the file

        \param file Filename of the file being patched
        \param contents File contents of the file being patched
        \param bootImages List of boot image filenames in the zip file
     */
    virtual bool patchFile(const QString &file,
                           QByteArray * const contents,
                           const QStringList &bootImages) = 0;
};


/*!
    \class RamdiskPatcher
    \brief Handles common patching operations of boot images

    This is a helper class for the patching of boot images. This class is
    usually called by a Patcher to perform common or repetitive patching tasks
    for boot images (either standalone or from a zip file).

    \sa Patcher
 */
class RamdiskPatcher
{
public:
    virtual ~RamdiskPatcher() {}

    /*!
        \brief The error code

        Returns error code from PatcherError::Error. The value is invalid if
        nothing has failed.

        \return PatcherError error code
     */
    virtual PatcherError::Error error() const = 0;

    /*!
        \brief The error message

        Returns a translated string corresponding to the the error code.

        \return Translated error message
     */
    virtual QString errorString() const = 0;

    /*!
        \brief The ramdisk patcher's identifier
     */
    virtual QString id() const = 0;

    /*!
        \brief Start patching the ramdisk
     */
    virtual bool patchRamdisk() = 0;
};

#endif // PATCHERINTERFACE_H
