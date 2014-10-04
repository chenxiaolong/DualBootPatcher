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

#ifndef HLTERAMDISKPATCHER_H
#define HLTERAMDISKPATCHER_H

#include <libdbp/cpiofile.h>
#include <libdbp/plugininterface.h>

#include <QtCore/QObject>


class HlteRamdiskPatcherPrivate;

class HlteRamdiskPatcher : public RamdiskPatcher
{
public:
    explicit HlteRamdiskPatcher(const PatcherPaths * const pp,
                                const QString &id,
                                const FileInfo * const info,
                                CpioFile * const cpio);
    ~HlteRamdiskPatcher();

    static const QString AOSP;
    static const QString TouchWiz;

    virtual PatcherError::Error error() const;
    virtual QString errorString() const;

    virtual QString name() const;

    virtual bool patchRamdisk();

private:
    bool patchAOSP();
    bool patchTouchWiz();

    const QScopedPointer<HlteRamdiskPatcherPrivate> d_ptr;
    Q_DECLARE_PRIVATE(HlteRamdiskPatcher)
};
#endif // HLTERAMDISKPATCHER_H
