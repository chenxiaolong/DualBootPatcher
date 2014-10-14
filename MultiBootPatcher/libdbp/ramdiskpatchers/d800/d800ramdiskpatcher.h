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

#ifndef D800RAMDISKPATCHER_H
#define D800RAMDISKPATCHER_H

#include <libdbp/cpiofile.h>
#include <libdbp/patcherinterface.h>

#include <QtCore/QObject>


class D800RamdiskPatcherPrivate;

class D800RamdiskPatcher : public RamdiskPatcher
{
public:
    explicit D800RamdiskPatcher(const PatcherPaths * const pp,
                                const FileInfo * const info,
                                CpioFile * const cpio);
    ~D800RamdiskPatcher();

    static const QString Id;

    virtual PatcherError::Error error() const override;
    virtual QString errorString() const override;

    virtual QString id() const override;

    virtual bool patchRamdisk() override;

private:
    const QScopedPointer<D800RamdiskPatcherPrivate> d_ptr;
    Q_DECLARE_PRIVATE(D800RamdiskPatcher)
};

#endif // D800RAMDISKPATCHER_H
