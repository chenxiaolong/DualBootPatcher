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
#include <libdbp/patcherinterface.h>

#include <QtCore/QObject>


class JflteBaseRamdiskPatcherPrivate;

class JflteBaseRamdiskPatcher : public RamdiskPatcher
{
public:
    explicit JflteBaseRamdiskPatcher(const PatcherPaths * const pp,
                                     const FileInfo * const info,
                                     CpioFile * const cpio);
    virtual ~JflteBaseRamdiskPatcher();

    virtual PatcherError::Error error() const override;
    virtual QString errorString() const override;

    virtual QString id() const override = 0;

    virtual bool patchRamdisk() override = 0;

protected:
    const QScopedPointer<JflteBaseRamdiskPatcherPrivate> d_ptr;
    Q_DECLARE_PRIVATE(JflteBaseRamdiskPatcher)
};


class JflteAOSPRamdiskPatcher : public JflteBaseRamdiskPatcher
{
public:
    explicit JflteAOSPRamdiskPatcher(const PatcherPaths * const pp,
                                     const FileInfo * const info,
                                     CpioFile * const cpio);

    static const QString Id;

    virtual QString id() const override;

    virtual bool patchRamdisk() override;
};


class JflteGoogleEditionRamdiskPatcher : public JflteBaseRamdiskPatcher
{
public:
    explicit JflteGoogleEditionRamdiskPatcher(const PatcherPaths * const pp,
                                              const FileInfo * const info,
                                              CpioFile * const cpio);

    static const QString Id;

    virtual QString id() const override;

    virtual bool patchRamdisk() override;
};


class JflteNoobdevRamdiskPatcher : public JflteBaseRamdiskPatcher
{
public:
    explicit JflteNoobdevRamdiskPatcher(const PatcherPaths * const pp,
                                        const FileInfo * const info,
                                        CpioFile * const cpio);

    static const QString Id;

    virtual QString id() const override;

    virtual bool patchRamdisk() override;

private:
    bool cxlModifyInitTargetRc();
};


class JflteTouchWizRamdiskPatcher : public JflteBaseRamdiskPatcher
{
public:
    explicit JflteTouchWizRamdiskPatcher(const PatcherPaths * const pp,
                                         const FileInfo * const info,
                                         CpioFile * const cpio);

    static const QString Id;

    virtual QString id() const override;

    virtual bool patchRamdisk() override;
};

#endif // JFLTERAMDISKPATCHER_H
