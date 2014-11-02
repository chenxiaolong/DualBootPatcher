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

#include "coreramdiskpatcher.h"

#include <boost/format.hpp>

#include "patcherpaths.h"


class CoreRamdiskPatcher::Impl
{
public:
    const PatcherPaths *pp;
    const FileInfo *info;
    CpioFile *cpio;

    PatcherError error;
};


const std::string CoreRamdiskPatcher::FstabRegex
        = "^(#.+)?(/dev/\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)";
const std::string CoreRamdiskPatcher::ExecMount
        = "exec /sbin/busybox-static sh /init.multiboot.mounting.sh";
const std::string CoreRamdiskPatcher::PropPartConfig
        = "ro.patcher.patched=%1%\n";
const std::string CoreRamdiskPatcher::PropVersion
        = "ro.patcher.version=%1%\n";
const std::string CoreRamdiskPatcher::SyncdaemonService
        = "\nservice syncdaemon /sbin/syncdaemon\n"
        "    class main\n"
        "    user root\n";

static const std::string DefaultProp = "default.prop";
static const std::string InitRc = "init.rc";
static const std::string Busybox = "sbin/busybox";

CoreRamdiskPatcher::CoreRamdiskPatcher(const PatcherPaths * const pp,
                                       const FileInfo * const info,
                                       CpioFile * const cpio) :
    m_impl(new Impl())
{
    m_impl->pp = pp;
    m_impl->info = info;
    m_impl->cpio = cpio;
}

CoreRamdiskPatcher::~CoreRamdiskPatcher()
{
}

PatcherError CoreRamdiskPatcher::error() const
{
    return m_impl->error;
}

std::string CoreRamdiskPatcher::id() const
{
    return std::string();
}

bool CoreRamdiskPatcher::patchRamdisk()
{
    if (!modifyDefaultProp()) {
        return false;
    }
    if (!addSyncdaemon()) {
        return false;
    }
    if (!removeAndRelinkBusybox()) {
        return false;
    }
    return true;
}

bool CoreRamdiskPatcher::modifyDefaultProp()
{
    auto defaultProp = m_impl->cpio->contents(DefaultProp);
    if (defaultProp.empty()) {
        m_impl->error = PatcherError::createCpioError(
                PatcherError::CpioFileNotExistError, DefaultProp);
        return false;
    }

    defaultProp.insert(defaultProp.end(), '\n');

    std::string propPartConfig = (boost::format(PropPartConfig)
            % m_impl->info->partConfig()->id()).str();
    defaultProp.insert(defaultProp.end(),
                       propPartConfig.begin(), propPartConfig.end());

    std::string propVersion = (boost::format(PropVersion)
            % m_impl->pp->version()).str();
    defaultProp.insert(defaultProp.end(),
                       propVersion.begin(), propVersion.end());

    m_impl->cpio->setContents(DefaultProp, std::move(defaultProp));

    return true;
}

bool CoreRamdiskPatcher::addSyncdaemon()
{
    auto initRc = m_impl->cpio->contents(InitRc);
    if (initRc.empty()) {
        m_impl->error = PatcherError::createCpioError(
                PatcherError::CpioFileNotExistError, InitRc);
        return false;
    }

    initRc.insert(initRc.end(),
                  SyncdaemonService.begin(), SyncdaemonService.end());

    m_impl->cpio->setContents(InitRc, std::move(initRc));

    return true;
}

bool CoreRamdiskPatcher::removeAndRelinkBusybox()
{
    if (m_impl->cpio->exists(Busybox)) {
        m_impl->cpio->remove(Busybox);
        bool ret = m_impl->cpio->addSymlink("busybox-static", Busybox);

        if (!ret) {
            m_impl->error = m_impl->cpio->error();
        }

        return ret;
    }

    return true;
}
