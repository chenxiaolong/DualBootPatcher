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

#ifndef GALAXYRAMDISKPATCHER_H
#define GALAXYRAMDISKPATCHER_H

#include "libdbp-ramdisk-galaxy_global.h"

#include <libdbp/cpiofile.h>
#include <libdbp/plugininterface.h>

#include <QtCore/QObject>
#include <QtCore/QVariant>

class GalaxyRamdiskPatcherPrivate;

class LIBDBPRAMDISKGALAXYSHARED_EXPORT GalaxyRamdiskPatcher : public RamdiskPatcher
{
public:
    explicit GalaxyRamdiskPatcher(const PatcherPaths * const pp,
                                  const FileInfo * const info,
                                  CpioFile * const cpio,
                                  const QString &version);
    ~GalaxyRamdiskPatcher();

    static const QString JellyBean;
    static const QString KitKat;

    virtual PatcherError::Error error() const override;
    virtual QString errorString() const override;

    virtual QString name() const override;

    virtual bool patchRamdisk() override;

    // Google Edition
    bool geModifyInitRc();

    // TouchWiz
    bool twModifyInitRc();
    bool twModifyInitTargetRc();
    bool twModifyUeventdRc();
    bool twModifyUeventdQcomRc();

    // Google Edition and TouchWiz
    bool getwModifyMsm8960LpmRc();

private:
    const QScopedPointer<GalaxyRamdiskPatcherPrivate> d_ptr;
    Q_DECLARE_PRIVATE(GalaxyRamdiskPatcher)
};

#endif // GALAXYRAMDISKPATCHER_H
