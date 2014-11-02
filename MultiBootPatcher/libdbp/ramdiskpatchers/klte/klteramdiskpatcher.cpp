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

#include "ramdiskpatchers/klte/klteramdiskpatcher.h"

#include "patcherpaths.h"
#include "ramdiskpatchers/common/coreramdiskpatcher.h"
#include "ramdiskpatchers/galaxy/galaxyramdiskpatcher.h"
#include "ramdiskpatchers/qcom/qcomramdiskpatcher.h"


class KlteBaseRamdiskPatcher::Impl
{
public:
    const PatcherPaths *pp;
    const FileInfo *info;
    CpioFile *cpio;

    PatcherError error;
};


/*!
    \class KlteRamdiskPatcher
    \brief Handles common ramdisk patching operations for the Samsung Galaxy S 5

    This patcher handles the patching of ramdisks for the Samsung Galaxy S 5.
    The currently supported ramdisk types are:

    1. AOSP or AOSP-derived ramdisks
    2. TouchWiz ramdisks
 */


KlteBaseRamdiskPatcher::KlteBaseRamdiskPatcher(const PatcherPaths * const pp,
                                               const FileInfo * const info,
                                               CpioFile * const cpio) :
    m_impl(new Impl())
{
    m_impl->pp = pp;
    m_impl->info = info;
    m_impl->cpio = cpio;
}

KlteBaseRamdiskPatcher::~KlteBaseRamdiskPatcher()
{
}

PatcherError KlteBaseRamdiskPatcher::error() const
{
    return m_impl->error;
}

////////////////////////////////////////////////////////////////////////////////

const std::string KlteAOSPRamdiskPatcher::Id = "klte/AOSP/AOSP";

KlteAOSPRamdiskPatcher::KlteAOSPRamdiskPatcher(const PatcherPaths *const pp,
                                               const FileInfo *const info,
                                               CpioFile *const cpio)
    : KlteBaseRamdiskPatcher(pp, info, cpio)
{
}

std::string KlteAOSPRamdiskPatcher::id() const
{
    return Id;
}

bool KlteAOSPRamdiskPatcher::patchRamdisk()
{
    CoreRamdiskPatcher corePatcher(m_impl->pp, m_impl->info, m_impl->cpio);
    QcomRamdiskPatcher qcomPatcher(m_impl->pp, m_impl->info, m_impl->cpio);

    if (!corePatcher.patchRamdisk()) {
        m_impl->error = corePatcher.error();
        return false;
    }

    if (!qcomPatcher.modifyInitRc()) {
        m_impl->error = qcomPatcher.error();
        return false;
    }

    if (!qcomPatcher.modifyInitQcomRc()) {
        m_impl->error = qcomPatcher.error();
        return false;
    }

    if (!qcomPatcher.modifyFstab(true)) {
        m_impl->error = qcomPatcher.error();
        return false;
    }

    if (m_impl->cpio->exists("init.target.rc")) {
        if (!qcomPatcher.modifyInitTargetRc()) {
            m_impl->error = qcomPatcher.error();
            return false;
        }
    } else {
        if (!qcomPatcher.modifyInitTargetRc("init.qcom.rc")) {
            m_impl->error = qcomPatcher.error();
            return false;
        }
    }

    std::string mountScript(m_impl->pp->scriptsDirectory()
            + "/klte/mount.modem.aosp.sh");
    m_impl->cpio->addFile(mountScript, "init.additional.sh", 0755);

    return true;
}

////////////////////////////////////////////////////////////////////////////////

const std::string KlteTouchWizRamdiskPatcher::Id = "klte/TouchWiz/TouchWiz";

KlteTouchWizRamdiskPatcher::KlteTouchWizRamdiskPatcher(const PatcherPaths *const pp,
                                                       const FileInfo *const info,
                                                       CpioFile *const cpio)
    : KlteBaseRamdiskPatcher(pp, info, cpio)
{
}

std::string KlteTouchWizRamdiskPatcher::id() const
{
    return Id;
}

bool KlteTouchWizRamdiskPatcher::patchRamdisk()
{
    CoreRamdiskPatcher corePatcher(m_impl->pp, m_impl->info, m_impl->cpio);
    QcomRamdiskPatcher qcomPatcher(m_impl->pp, m_impl->info, m_impl->cpio);
    GalaxyRamdiskPatcher galaxyPatcher(m_impl->pp, m_impl->info, m_impl->cpio,
                                       GalaxyRamdiskPatcher::KitKat);

    if (!corePatcher.patchRamdisk()) {
        m_impl->error = corePatcher.error();
        return false;
    }

    if (!qcomPatcher.modifyInitRc()) {
        m_impl->error = qcomPatcher.error();
        return false;
    }

    if (!galaxyPatcher.twModifyInitRc()) {
        m_impl->error = galaxyPatcher.error();
        return false;
    }

    if (!qcomPatcher.modifyInitQcomRc()) {
        m_impl->error = qcomPatcher.error();
        return false;
    }

    if (!qcomPatcher.modifyFstab()) {
        m_impl->error = qcomPatcher.error();
        return false;
    }

    if (!qcomPatcher.modifyInitTargetRc()) {
        m_impl->error = qcomPatcher.error();
        return false;
    }

    if (!galaxyPatcher.twModifyInitTargetRc()) {
        m_impl->error = galaxyPatcher.error();
        return false;
    }

    if (!galaxyPatcher.twModifyUeventdRc()) {
        m_impl->error = galaxyPatcher.error();
        return false;
    }

    if (!galaxyPatcher.twModifyUeventdQcomRc()) {
        m_impl->error = galaxyPatcher.error();
        return false;
    }

    std::string mountScript(m_impl->pp->scriptsDirectory()
            + "/klte/mount.modem.touchwiz.sh");
    m_impl->cpio->addFile(mountScript, "init.additional.sh", 0755);

    // Samsung's init binary is pretty screwed up
    m_impl->cpio->remove("init");

    std::string newInit = m_impl->pp->initsDirectory() + "/jflte/tw44-init";
    m_impl->cpio->addFile(newInit, "init", 0755);

    std::string newAdbd = m_impl->pp->initsDirectory() + "/jflte/tw44-adbd";
    m_impl->cpio->addFile(newAdbd, "sbin/adbd", 0755);

    return true;
}
