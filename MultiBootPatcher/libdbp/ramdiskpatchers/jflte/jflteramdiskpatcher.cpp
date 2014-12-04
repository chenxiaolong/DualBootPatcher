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

#include "ramdiskpatchers/jflte/jflteramdiskpatcher.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>

#include "patcherconfig.h"
#include "ramdiskpatchers/common/coreramdiskpatcher.h"
#include "ramdiskpatchers/galaxy/galaxyramdiskpatcher.h"
#include "ramdiskpatchers/qcom/qcomramdiskpatcher.h"


/*! \cond INTERNAL */
class JflteBaseRamdiskPatcher::Impl
{
public:
    const PatcherConfig *pc;
    const FileInfo *info;
    CpioFile *cpio;

    std::string getwVersion;

    PatcherError error;
};
/*! \endcond */


static const std::string InitRc = "init.rc";
static const std::string InitTargetRc = "init.target.rc";
static const std::string UeventdRc = "ueventd.rc";
static const std::string UeventdQcomRc = "ueventd.qcom.rc";
static const std::string Msm8960LpmRc = "MSM8960_lpm.rc";


/*!
    \class JflteRamdiskPatcher
    \brief Handles common ramdisk patching operations for the Samsung Galaxy S 4

    This patcher handles the patching of ramdisks for the Samsung Galaxy S 4.
    The currently supported ramdisk types are:

    1. AOSP or AOSP-derived ramdisks
    2. Google Edition (Google Play Edition) ramdisks
    3. TouchWiz (Android 4.2-4.4) ramdisks
 */


JflteBaseRamdiskPatcher::JflteBaseRamdiskPatcher(const PatcherConfig * const pc,
                                                 const FileInfo * const info,
                                                 CpioFile * const cpio)
    : m_impl(new Impl())
{
    m_impl->pc = pc;
    m_impl->info = info;
    m_impl->cpio = cpio;
}

JflteBaseRamdiskPatcher::~JflteBaseRamdiskPatcher()
{
}

PatcherError JflteBaseRamdiskPatcher::error() const
{
    return m_impl->error;
}

////////////////////////////////////////////////////////////////////////////////

const std::string JflteAOSPRamdiskPatcher::Id = "jflte/AOSP/AOSP";

JflteAOSPRamdiskPatcher::JflteAOSPRamdiskPatcher(const PatcherConfig * const pc,
                                                 const FileInfo *const info,
                                                 CpioFile *const cpio)
    : JflteBaseRamdiskPatcher(pc, info, cpio)
{
}

std::string JflteAOSPRamdiskPatcher::id() const
{
    return Id;
}

bool JflteAOSPRamdiskPatcher::patchRamdisk()
{
    CoreRamdiskPatcher corePatcher(m_impl->pc, m_impl->info, m_impl->cpio);
    QcomRamdiskPatcher qcomPatcher(m_impl->pc, m_impl->info, m_impl->cpio);

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

    if (!qcomPatcher.modifyInitTargetRc()) {
        m_impl->error = qcomPatcher.error();
        return false;
    }

    std::string mountScript(m_impl->pc->scriptsDirectory()
            + "/jflte/mount.modem.sh");
    m_impl->cpio->addFile(mountScript, "init.additional.sh", 0755);

    return true;
}

////////////////////////////////////////////////////////////////////////////////

const std::string JflteGoogleEditionRamdiskPatcher::Id
        = "jflte/GoogleEdition/GoogleEdition";

JflteGoogleEditionRamdiskPatcher::JflteGoogleEditionRamdiskPatcher(const PatcherConfig * const pc,
                                                                   const FileInfo *const info,
                                                                   CpioFile *const cpio)
    : JflteBaseRamdiskPatcher(pc, info, cpio)
{
    if (m_impl->cpio->exists(Msm8960LpmRc)) {
        m_impl->getwVersion = GalaxyRamdiskPatcher::JellyBean;
    } else {
        m_impl->getwVersion = GalaxyRamdiskPatcher::KitKat;
    }
}

std::string JflteGoogleEditionRamdiskPatcher::id() const
{
    return Id;
}

bool JflteGoogleEditionRamdiskPatcher::patchRamdisk()
{
    CoreRamdiskPatcher corePatcher(m_impl->pc, m_impl->info, m_impl->cpio);
    QcomRamdiskPatcher qcomPatcher(m_impl->pc, m_impl->info, m_impl->cpio);
    GalaxyRamdiskPatcher galaxyPatcher(m_impl->pc, m_impl->info, m_impl->cpio,
                                       m_impl->getwVersion);

    if (!corePatcher.patchRamdisk()) {
        m_impl->error = corePatcher.error();
        return false;
    }

    if (!qcomPatcher.modifyInitRc()) {
        m_impl->error = qcomPatcher.error();
        return false;
    }

    if (!galaxyPatcher.geModifyInitRc()) {
        m_impl->error = galaxyPatcher.error();
        return false;
    }

    std::vector<std::string> qcomRcFiles;
    qcomRcFiles.push_back("init.jgedlte.rc");

    if (!qcomPatcher.modifyInitQcomRc(qcomRcFiles)) {
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

    if (!galaxyPatcher.getwModifyMsm8960LpmRc()) {
        m_impl->error = galaxyPatcher.error();
        return false;
    }

    // Samsung's init binary is pretty screwed up
    if (m_impl->getwVersion == GalaxyRamdiskPatcher::KitKat) {
        m_impl->cpio->remove("init");

        std::string newInit(m_impl->pc->initsDirectory() + "/init-kk44");
        m_impl->cpio->addFile(newInit, "init", 0755);
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////

const std::string JflteTouchWizRamdiskPatcher::Id = "jflte/TouchWiz/TouchWiz";

JflteTouchWizRamdiskPatcher::JflteTouchWizRamdiskPatcher(const PatcherConfig * const pc,
                                                         const FileInfo *const info,
                                                         CpioFile *const cpio)
    : JflteBaseRamdiskPatcher(pc, info, cpio)
{
    if (m_impl->cpio->exists(Msm8960LpmRc)) {
        m_impl->getwVersion = GalaxyRamdiskPatcher::JellyBean;
    } else {
        m_impl->getwVersion = GalaxyRamdiskPatcher::KitKat;
    }
}

std::string JflteTouchWizRamdiskPatcher::id() const
{
    return Id;
}

bool JflteTouchWizRamdiskPatcher::patchRamdisk()
{
    CoreRamdiskPatcher corePatcher(m_impl->pc, m_impl->info, m_impl->cpio);
    QcomRamdiskPatcher qcomPatcher(m_impl->pc, m_impl->info, m_impl->cpio);
    GalaxyRamdiskPatcher galaxyPatcher(m_impl->pc, m_impl->info, m_impl->cpio,
                                       m_impl->getwVersion);

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

    if (!galaxyPatcher.getwModifyMsm8960LpmRc()) {
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

    std::string mountScript(m_impl->pc->scriptsDirectory()
            + "/jflte/mount.modem.sh");
    m_impl->cpio->addFile(mountScript, "init.additional.sh", 0755);

    // Samsung's init binary is pretty screwed up
    if (m_impl->getwVersion == GalaxyRamdiskPatcher::KitKat) {
        m_impl->cpio->remove("init");

        std::string newInit(m_impl->pc->initsDirectory() + "/jflte/tw44-init");
        m_impl->cpio->addFile(newInit, "init", 0755);

        std::string newAdbd(m_impl->pc->initsDirectory() + "/jflte/tw44-adbd");
        m_impl->cpio->addFile(newAdbd, "sbin/adbd", 0755);
    }

    return true;
}
