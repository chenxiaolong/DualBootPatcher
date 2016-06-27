/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbp/ramdiskpatchers/core.h"

#include <algorithm>

#include "mbp/patcherconfig.h"
#include "mbp/private/stringutils.h"


namespace mbp
{

/*! \cond INTERNAL */
class CoreRP::Impl
{
public:
    const PatcherConfig *pc;
    const FileInfo *info;
    CpioFile *cpio;

    ErrorCode error;

    std::vector<std::string> fstabs;
};
/*! \endcond */


CoreRP::CoreRP(const PatcherConfig * const pc,
               const FileInfo * const info,
               CpioFile * const cpio) :
    m_impl(new Impl())
{
    m_impl->pc = pc;
    m_impl->info = info;
    m_impl->cpio = cpio;
}

CoreRP::~CoreRP()
{
}

ErrorCode CoreRP::error() const
{
    return m_impl->error;
}

std::string CoreRP::id() const
{
    return std::string();
}

bool CoreRP::patchRamdisk()
{
    return addMbtool()
            && addExfat()
            && setUpInitWrapper()
            && addBlockDevProps();
}

bool CoreRP::addMbtool()
{
    const std::string mbtool("mbtool");
    const std::string sig("mbtool.sig");
    std::string mbtoolPath(m_impl->pc->dataDirectory());
    mbtoolPath += "/binaries/android/";
    mbtoolPath += m_impl->info->device()->architecture();
    mbtoolPath += "/mbtool";
    std::string sigPath(mbtoolPath);
    sigPath += ".sig";

    m_impl->cpio->remove(mbtool);
    m_impl->cpio->remove(sig);

    if (!m_impl->cpio->addFile(mbtoolPath, mbtool, 0750)
            || !m_impl->cpio->addFile(sigPath, sig, 0640)) {
        m_impl->error = m_impl->cpio->error();
        return false;
    }

    return true;
}

bool CoreRP::addExfat()
{
    const std::string mount("sbin/mount.exfat");
    const std::string fsck("sbin/fsck.exfat");
    const std::string mountSig("sbin/mount.exfat.sig");
    const std::string fsckSig("sbin/fsck.exfat.sig");

    std::string mountPath(m_impl->pc->dataDirectory());
    mountPath += "/binaries/android/";
    mountPath += m_impl->info->device()->architecture();
    mountPath += "/mount.exfat";
    std::string sigPath(mountPath);
    sigPath += ".sig";

    m_impl->cpio->remove(mount);
    m_impl->cpio->remove(fsck);
    m_impl->cpio->remove(mountSig);
    m_impl->cpio->remove(fsckSig);

    if (!m_impl->cpio->addFile(mountPath, mount, 0750)
            || !m_impl->cpio->addFile(sigPath, mountSig, 0640)
            || !m_impl->cpio->addSymlink("mount.exfat", fsck)
            || !m_impl->cpio->addSymlink("mount.exfat.sig", fsckSig)) {
        m_impl->error = m_impl->cpio->error();
        return false;
    }

    return true;
}

bool CoreRP::setUpInitWrapper()
{
    if (m_impl->cpio->exists("init.orig")) {
        // NOTE: If this assumption ever becomes an issue, we'll check the
        //       /init -> /mbtool symlink
        return true;
    }

    if (!m_impl->cpio->rename("init", "init.orig")
            || !m_impl->cpio->addSymlink("mbtool", "init")) {
        m_impl->error = m_impl->cpio->error();
        return false;
    }

    return true;
}

static std::string encode_list(const std::vector<std::string> &list)
{
    // We processing char-by-char, so avoid unnecessary reallocations
    std::size_t size = 0;
    for (const std::string &item : list) {
        size += item.size();
        size += std::count(item.begin(), item.end(), ',');
    }
    if (size == 0) {
        return std::string();
    }
    size += list.size() - 1;

    std::string result;
    result.reserve(size);

    bool first = true;
    for (const std::string &item : list) {
        if (!first) {
            result += ',';
        } else {
            first = false;
        }
        for (char c : item) {
            if (c == ',' || c == '\\') {
                result += '\\';
            }
            result += c;
        }
    }

    return result;
}

bool CoreRP::addBlockDevProps()
{
    if (!m_impl->info->device()) {
        return true;
    }

    std::vector<unsigned char> contents;
    m_impl->cpio->contents("default.prop", &contents);

    std::vector<std::string> lines = StringUtils::splitData(contents, '\n');
    for (auto it = lines.begin(); it != lines.end();) {
        if (StringUtils::starts_with(*it, "ro.patcher.blockdevs.")) {
            it = lines.erase(it);
        } else {
            ++it;
        }
    }

    Device *device = m_impl->info->device();
    std::string encoded;

    encoded = "ro.patcher.blockdevs.base=";
    encoded += encode_list(device->blockDevBaseDirs());
    lines.push_back(encoded);

    encoded = "ro.patcher.blockdevs.system=";
    encoded += encode_list(device->systemBlockDevs());
    lines.push_back(encoded);

    encoded = "ro.patcher.blockdevs.cache=";
    encoded += encode_list(device->cacheBlockDevs());
    lines.push_back(encoded);

    encoded = "ro.patcher.blockdevs.data=";
    encoded += encode_list(device->dataBlockDevs());
    lines.push_back(encoded);

    encoded = "ro.patcher.blockdevs.boot=";
    encoded += encode_list(device->bootBlockDevs());
    lines.push_back(encoded);

    encoded = "ro.patcher.blockdevs.recovery=";
    encoded += encode_list(device->recoveryBlockDevs());
    lines.push_back(encoded);

    encoded = "ro.patcher.blockdevs.extra=";
    encoded += encode_list(device->extraBlockDevs());
    lines.push_back(encoded);

    contents = StringUtils::joinData(lines, '\n');
    m_impl->cpio->setContents("default.prop", std::move(contents));

    return true;
}

}
