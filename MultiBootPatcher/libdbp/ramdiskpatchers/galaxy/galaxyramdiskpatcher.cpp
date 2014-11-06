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

#include "ramdiskpatchers/galaxy/galaxyramdiskpatcher.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/regex.hpp>

#include "ramdiskpatchers/common/coreramdiskpatcher.h"


class GalaxyRamdiskPatcher::Impl
{
public:
    const PatcherPaths *pp;
    const FileInfo *info;
    CpioFile *cpio;

    std::string version;

    PatcherError error;
};


/*! \brief Android Jelly Bean */
const std::string GalaxyRamdiskPatcher::JellyBean("jb");
/*! \brief Android Kit Kat */
const std::string GalaxyRamdiskPatcher::KitKat("kk");

static const std::string InitRc("init.rc");
static const std::string InitTargetRc("init.target.rc");
static const std::string UeventdRc("ueventd.rc");
static const std::string UeventdQcomRc("ueventd.qcom.rc");
static const std::string Msm8960LpmRc("MSM8960_lpm.rc");


/*!
    \class GalaxyRamdiskPatcher
    \brief Handles common ramdisk patching operations for Samsung Galaxy devices.
 */

/*!
    Constructs a ramdisk patcher associated with the ramdisk of a particular
    kernel image.

    The \a cpio is a pointer to a CpioFile on which all of the operations will
    be applied. If more than one ramdisk needs to be patched, create a new
    instance for each one. The \a version parameter specifies which Android
    version the ramdisk is from (GalaxyRamdiskPatcher::JellyBean,
    GalaxyRamdiskPatcher::KitKat, etc.).
 */
GalaxyRamdiskPatcher::GalaxyRamdiskPatcher(const PatcherPaths * const pp,
                                           const FileInfo * const info,
                                           CpioFile * const cpio,
                                           const std::string &version) :
    m_impl(new Impl())
{
    m_impl->pp = pp;
    m_impl->info = info;
    m_impl->cpio = cpio;
    m_impl->version = version;
}

GalaxyRamdiskPatcher::~GalaxyRamdiskPatcher()
{
}

PatcherError GalaxyRamdiskPatcher::error() const
{
    return m_impl->error;
}

std::string GalaxyRamdiskPatcher::id() const
{
    return std::string();
}

bool GalaxyRamdiskPatcher::patchRamdisk()
{
    return false;
}

static std::string whitespace(const std::string &str) {
    auto nonSpace = std::find_if(str.begin(), str.end(),
                                 std::not1(std::ptr_fun<int, int>(isspace)));
    int count = std::distance(str.begin(), nonSpace);

    return str.substr(0, count);
}

/*!
    \brief Patches init.rc in Google Edition ramdisks

    This method performs the following modifications on the init.rc file:

    1. Finds the /system mounting line for the charger mode:

           mount /system

       and replaces it with lines to mount the multiboot paths:

           mount_all fstab.jgedlte
           exec /sbin/busybox-static sh /init.multiboot.mounting.sh

    \return Succeeded or not
 */
bool GalaxyRamdiskPatcher::geModifyInitRc()
{
    auto contents = m_impl->cpio->contents(InitRc);
    if (contents.empty()) {
        m_impl->error = PatcherError::createCpioError(
                MBP::ErrorCode::CpioFileNotExistError, InitRc);
        return false;
    }

    std::string previousLine;

    std::string strContents(contents.begin(), contents.end());
    std::vector<std::string> lines;
    boost::split(lines, strContents, boost::is_any_of("\n"));

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        if (boost::regex_search(*it, boost::regex("mount.*/system"))
                && boost::regex_search(
                        previousLine, boost::regex("on\\s+charger"))) {
            it = lines.erase(it);
            it = lines.insert(it, "    mount_all fstab.jgedlte");
            ++it;
            it = lines.insert(it, "    " + CoreRamdiskPatcher::ExecMount);
        }

        previousLine = *it;
    }

    strContents = boost::join(lines, "\n");
    contents.assign(strContents.begin(), strContents.end());
    m_impl->cpio->setContents(InitRc, std::move(contents));

    return true;
}

/*!
    \brief Patches init.rc in TouchWiz ramdisks

    This method performs the following modifications on the init.rc file:

    1. Comments out lines that attempt to reload the SELinux policy (which fails
       in multi-booted TouchWiz ROMs.

           setprop selinux.reload_policy 1

    3. Removes calls to \c check_icd, which does not exist in the custom init
       implementation.

    4. If the version was set to GalaxyRamdiskPatcher::KitKat, then the
       \c /system/bin/mediaserver service will be modified such that it runs
       under the root user.

    \return Succeeded or not
 */
bool GalaxyRamdiskPatcher::twModifyInitRc()
{
    auto contents = m_impl->cpio->contents(InitRc);
    if (contents.empty()) {
        m_impl->error = PatcherError::createCpioError(
                MBP::ErrorCode::CpioFileNotExistError, InitRc);
        return false;
    }

    bool inMediaserver = false;

    std::string strContents(contents.begin(), contents.end());
    std::vector<std::string> lines;
    boost::split(lines, strContents, boost::is_any_of("\n"));

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        if (boost::regex_search(*it, boost::regex(
                "^.*setprop.*selinux.reload_policy.*$"))) {
            it->insert(it->begin(), '#');
        } else if (it->find("check_icd") != std::string::npos) {
            it->insert(it->begin(), '#');
        } else if (m_impl->version == KitKat
                && boost::starts_with(*it, "service")) {
            inMediaserver = it->find("/system/bin/mediaserver") != std::string::npos;
        } else if (inMediaserver
                && boost::regex_search(*it, boost::regex("^\\s*user"))) {
            *it = whitespace(*it) + "user root";
        }
    }

    strContents = boost::join(lines, "\n");
    contents.assign(strContents.begin(), strContents.end());
    m_impl->cpio->setContents(InitRc, std::move(contents));

    return true;
}

/*!
    \brief Patches init.target.rc in TouchWiz ramdisks

    This method performs the following modifications on the init.target.rc file:

    1. If the version is Kit Kat, the following multiboot mounting lines:

           mount_all fstab.qcom
           exec /sbin/busybox-static sh /init.multiboot.mounting.sh

       will be added to the:

           on fs_selinux

       section.

    2. Comments out lines that attempt to reload the SELinux policy (which fails
       in multi-booted TouchWiz ROMs.

           setprop selinux.reload_policy 1

    3. If the version was set to GalaxyRamdiskPatcher::KitKat, then the
       \c qcamerasvr service will be modified such that it runs under the root
       user and group.

    \return Succeeded or not
 */
bool GalaxyRamdiskPatcher::twModifyInitTargetRc()
{
    auto contents = m_impl->cpio->contents(InitTargetRc);
    if (contents.empty()) {
        m_impl->error = PatcherError::createCpioError(
                MBP::ErrorCode::CpioFileNotExistError, InitTargetRc);
        return false;
    }

    bool inQcam = false;

    std::string strContents(contents.begin(), contents.end());
    std::vector<std::string> lines;
    boost::split(lines, strContents, boost::is_any_of("\n"));

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        if (m_impl->version == KitKat
                && boost::regex_search(*it, boost::regex("^on\\s+fs_selinux\\s*$"))) {
            ++it;
            it = lines.insert(it, "    mount_all fstab.qcom");
            ++it;
            it = lines.insert(it, "    " + CoreRamdiskPatcher::ExecMount);
        } else if (boost::regex_search(*it,
                boost::regex("^.*setprop.*selinux.reload_policy.*$"))) {
            it->insert(it->begin(), '#');
        } else if (m_impl->version == KitKat
                && boost::starts_with(*it, "service")) {
            inQcam = it->find("qcamerasvr") != std::string::npos;
        }

        // This is not exactly safe, but it's the best we can do. TouchWiz is
        // doing some funny business where a process running under a certain
        // user or group (confirmed by /proc/<pid>/status) cannot access files
        // by that group. Not only that, mm-qcamera-daemon doesn't work if the
        // process has multiple groups and root is not the primary group. Oh
        // well, I'm done debugging proprietary binaries.

        else if (inQcam && boost::regex_search(*it, boost::regex("^\\s*user"))) {
            *it = whitespace(*it) + "user root";
        } else if (inQcam && boost::regex_search(*it, boost::regex("^\\s*group"))) {
            *it = whitespace(*it) + "group root";
        }
    }

    strContents = boost::join(lines, "\n");
    contents.assign(strContents.begin(), strContents.end());
    m_impl->cpio->setContents(InitTargetRc, std::move(contents));

    return true;
}

/*!
    \brief Patches ueventd.rc in TouchWiz Kit Kat ramdisks

    \note The method does not do anything for non-Kit Kat ramdisks.

    This method changed the permissions of the \c /dev/snd/* entry in ueventd.rc
    from 0660 to 0666.

    \return Succeeded or not
 */
bool GalaxyRamdiskPatcher::twModifyUeventdRc()
{
    // Only needs to be patched for Kit Kat
    if (m_impl->version != KitKat) {
        return true;
    }

    auto contents = m_impl->cpio->contents(UeventdRc);
    if (contents.empty()) {
        m_impl->error = PatcherError::createCpioError(
                MBP::ErrorCode::CpioFileNotExistError, UeventdRc);
        return false;
    }

    std::string strContents(contents.begin(), contents.end());
    std::vector<std::string> lines;
    boost::split(lines, strContents, boost::is_any_of("\n"));

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        if (it->find("/dev/snd/*") != std::string::npos) {
            boost::replace_all(*it, "0660", "0666");
        }
    }

    strContents = boost::join(lines, "\n");
    contents.assign(strContents.begin(), strContents.end());
    m_impl->cpio->setContents(UeventdRc, std::move(contents));

    return true;
}

/*!
    \brief Patches ueventd.qcom.rc in TouchWiz Kit Kat ramdisks

    \note The method does not do anything for non-Kit Kat ramdisks.

    This method changed the permissions from 0660 to 0666 in the following
    entries in \c ueventd.qcom.rc.

    - /dev/video*
    - /dev/media*
    - /dev/v4l-subdev*
    - /dev/msm_camera/*
    - /dev/msm_vidc_enc

    \return Succeeded or not
 */
bool GalaxyRamdiskPatcher::twModifyUeventdQcomRc()
{
    // Only needs to be patched for Kit Kat
    if (m_impl->version != KitKat) {
        return true;
    }

    auto contents = m_impl->cpio->contents(UeventdQcomRc);
    if (contents.empty()) {
        m_impl->error = PatcherError::createCpioError(
                MBP::ErrorCode::CpioFileNotExistError, UeventdQcomRc);
        return false;
    }

    std::string strContents(contents.begin(), contents.end());
    std::vector<std::string> lines;
    boost::split(lines, strContents, boost::is_any_of("\n"));

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        // More funny business: even with mm-qcamera-daemon running as root,
        // the daemon and the default camera application are unable access the
        // camera hardware unless it's writable to everyone. Baffles me...
        //
        // More, more funny business: chmod'ing the /dev/video* devices to
        // anything while mm-qcamera-daemon is running causes a kernel panic.
        // **Wonderful** /s
        if (it->find("/dev/video*") != std::string::npos
                || it->find("/dev/media*") != std::string::npos
                || it->find("/dev/v4l-subdev*") != std::string::npos
                || it->find("/dev/msm_camera/*") != std::string::npos
                || it->find("/dev/msm_vidc_enc") != std::string::npos) {
            boost::replace_all(*it, "0660", "0666");
        }
    }

    lines.push_back("/dev/ion 0666 system system\n");

    strContents = boost::join(lines, "\n");
    contents.assign(strContents.begin(), strContents.end());
    m_impl->cpio->setContents(UeventdQcomRc, std::move(contents));

    return true;
}

/*!
    \brief Patches MSM8960_lpm.rc in TouchWiz and Google Edition ramdisks

    \note The method does not do anything for Kit Kat ramdisks.

    This method comments out the line that mounts \c /cache in
    \c MSM8960_lpm.rc.

    \return Succeeded or not
 */
bool GalaxyRamdiskPatcher::getwModifyMsm8960LpmRc()
{
    // This file does not exist on Kit Kat ramdisks
    if (m_impl->version == KitKat) {
        return true;
    }

    auto contents = m_impl->cpio->contents(Msm8960LpmRc);
    if (contents.empty()) {
        m_impl->error = PatcherError::createCpioError(
                MBP::ErrorCode::CpioFileNotExistError, Msm8960LpmRc);
        return false;
    }

    std::string strContents(contents.begin(), contents.end());
    std::vector<std::string> lines;
    boost::split(lines, strContents, boost::is_any_of("\n"));

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        if (boost::regex_search(*it, boost::regex("^\\s+mount.*/cache.*$"))) {
            it->insert(it->begin(), '#');
        }
    }

    strContents = boost::join(lines, "\n");
    contents.assign(strContents.begin(), strContents.end());
    m_impl->cpio->setContents(Msm8960LpmRc, std::move(contents));

    return true;
}
