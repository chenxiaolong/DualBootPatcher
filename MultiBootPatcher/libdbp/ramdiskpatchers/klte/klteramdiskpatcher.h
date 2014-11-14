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

#ifndef KLTERAMDISKPATCHER_H
#define KLTERAMDISKPATCHER_H

#include <memory>

#include "cpiofile.h"
#include "patcherconfig.h"
#include "patcherinterface.h"


class KlteBaseRamdiskPatcher : public RamdiskPatcher
{
public:
    explicit KlteBaseRamdiskPatcher(const PatcherConfig * const pc,
                                    const FileInfo * const info,
                                    CpioFile * const cpio);
    virtual ~KlteBaseRamdiskPatcher();

    virtual PatcherError error() const override;

    virtual std::string id() const override = 0;

    virtual bool patchRamdisk() override = 0;

protected:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};


class KlteAOSPRamdiskPatcher : public KlteBaseRamdiskPatcher
{
public:
    explicit KlteAOSPRamdiskPatcher(const PatcherConfig * const pc,
                                    const FileInfo * const info,
                                    CpioFile * const cpio);

    static const std::string Id;

    virtual std::string id() const override;

    virtual bool patchRamdisk() override;
};


class KlteTouchWizRamdiskPatcher : public KlteBaseRamdiskPatcher
{
public:
    explicit KlteTouchWizRamdiskPatcher(const PatcherConfig * const pc,
                                        const FileInfo * const info,
                                        CpioFile * const cpio);

    static const std::string Id;

    virtual std::string id() const override;

    virtual bool patchRamdisk() override;
};

#endif // KLTERAMDISKPATCHER_H
