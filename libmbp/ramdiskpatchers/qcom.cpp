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

#include "ramdiskpatchers/qcom.h"

#include <regex>

#include "private/logging.h"


namespace mbp
{

/*! \cond INTERNAL */
class QcomRP::Impl
{
public:
    const PatcherConfig *pc;
    const FileInfo *info;
    CpioFile *cpio;

    PatcherError error;
};
/*! \endcond */


static const std::string FstabRegex =
        "^(#.+)?(/dev/\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)";

static const std::string CachePartition =
        "/dev/block/platform/msm_sdcc.1/by-name/cache";

static const std::string System = "/system";
static const std::string Cache = "/cache";
static const std::string Data = "/data";


QcomRP::QcomRP(const PatcherConfig * const pc,
               const FileInfo * const info,
               CpioFile * const cpio) :
    m_impl(new Impl())
{
    m_impl->pc = pc;
    m_impl->info = info;
    m_impl->cpio = cpio;
}

QcomRP::~QcomRP()
{
}

PatcherError QcomRP::error() const
{
    return m_impl->error;
}

std::string QcomRP::id() const
{
    return std::string();
}

bool QcomRP::patchRamdisk()
{
    return false;
}

bool QcomRP::addMissingCacheInFstab(const std::vector<std::string> &additionalFstabs)
{
    std::vector<std::string> fstabs;
    for (auto const &file : m_impl->cpio->filenames()) {
        if (StringUtils::starts_with(file, "fstab.")) {
            fstabs.push_back(file);
        }
    }

    fstabs.insert(fstabs.end(),
                  additionalFstabs.begin(), additionalFstabs.end());

    for (auto const &fstab : fstabs) {
        std::vector<unsigned char> contents;
        if (!m_impl->cpio->contents(fstab, &contents)) {
            m_impl->error = m_impl->cpio->error();
            return false;
        }

        std::vector<std::string> lines = StringUtils::splitData(contents, '\n');

        // Some Android 4.2 ROMs mount the cache partition in the init
        // scripts, so the fstab has no cache line
        bool hasCacheLine = false;

        for (auto it = lines.begin(); it != lines.end(); ++it) {
            auto &line = *it;

            std::smatch what;
            bool hasMatch = std::regex_search(
                    line, what, std::regex(FstabRegex));

            if (hasMatch && what[3] == Cache) {
                hasCacheLine = true;
            }
        }

        if (!hasCacheLine) {
            lines.push_back(StringUtils::format(
                    "%s /cache ext4 nosuid,nodev,barrier=1 wait,check",
                    CachePartition.c_str()));
        }

        contents = StringUtils::joinData(lines, '\n');
        m_impl->cpio->setContents(fstab, std::move(contents));
    }

    return true;
}

}
