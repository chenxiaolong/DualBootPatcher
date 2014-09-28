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

#ifndef JFLTERAMDISKPATCHER_H
#define JFLTERAMDISKPATCHER_H

#include <libdbp/cpiofile.h>
#include <libdbp/plugininterface.h>

#include <QtCore/QObject>


class JflteRamdiskPatcherPrivate;

class JflteRamdiskPatcher : public RamdiskPatcher
{
public:
    explicit JflteRamdiskPatcher(const PatcherPaths * const pp,
                                 const QString &id,
                                 const FileInfo * const info,
                                 CpioFile * const cpio);
    ~JflteRamdiskPatcher();

    static const QString AOSP;
    static const QString CXL;
    static const QString GoogleEdition;
    static const QString TouchWiz;

    virtual PatcherError::Error error() const;
    virtual QString errorString() const;

    virtual QString name() const;

    virtual bool patchRamdisk();

private:
    bool patchAOSP();
    bool patchCXL();
    bool patchGoogleEdition();
    bool patchTouchWiz();

    // chenxiaolong's noobdev
    bool cxlModifyInitTargetRc();

    // Google Edition
    bool geModifyInitRc();

    // TouchWiz
    bool twModifyInitRc();
    bool twModifyInitTargetRc();
    bool twModifyUeventdRc();
    bool twModifyUeventdQcomRc();

    // Google Edition and TouchWiz
    bool getwModifyMsm8960LpmRc();

    const QScopedPointer<JflteRamdiskPatcherPrivate> d_ptr;
    Q_DECLARE_PRIVATE(JflteRamdiskPatcher)
};
#endif // JFLTERAMDISKPATCHER_H
