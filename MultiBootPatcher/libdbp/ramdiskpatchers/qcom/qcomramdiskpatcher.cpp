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

#include "ramdiskpatchers/qcom/qcomramdiskpatcher.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/format.hpp>

#include "ramdiskpatchers/common/coreramdiskpatcher.h"
#include "private/logging.h"
#include "private/regex.h"


/*! \cond INTERNAL */
class QcomRamdiskPatcher::Impl
{
public:
    const PatcherConfig *pc;
    const FileInfo *info;
    CpioFile *cpio;

    PatcherError error;
};
/*! \endcond */


static const std::string CachePartition =
        "/dev/block/platform/msm_sdcc.1/by-name/cache";

static const std::string RawSystem = "/raw-system";
static const std::string RawCache = "/raw-cache";
static const std::string RawData = "/raw-data";
static const std::string System = "/system";
static const std::string Cache = "/cache";
static const std::string Data = "/data";


QcomRamdiskPatcher::QcomRamdiskPatcher(const PatcherConfig * const pc,
                                       const FileInfo * const info,
                                       CpioFile * const cpio) :
    m_impl(new Impl())
{
    m_impl->pc = pc;
    m_impl->info = info;
    m_impl->cpio = cpio;
}

QcomRamdiskPatcher::~QcomRamdiskPatcher()
{
}

PatcherError QcomRamdiskPatcher::error() const
{
    return m_impl->error;
}

std::string QcomRamdiskPatcher::id() const
{
    return std::string();
}

bool QcomRamdiskPatcher::patchRamdisk()
{
    return false;
}

bool QcomRamdiskPatcher::addMissingCacheInFstab(const std::vector<std::string> &additionalFstabs)
{
    std::vector<std::string> fstabs;
    for (auto const &file : m_impl->cpio->filenames()) {
        if (boost::starts_with(file, "fstab.")) {
            fstabs.push_back(file);
        }
    }

    fstabs.insert(fstabs.end(),
                  additionalFstabs.begin(), additionalFstabs.end());

    for (auto const &fstab : fstabs) {
        auto contents = m_impl->cpio->contents(fstab);
        if (contents.empty()) {
            m_impl->error = PatcherError::createCpioError(
                    MBP::ErrorCode::CpioFileNotExistError, fstab);
            return false;
        }

        std::string strContents(contents.begin(), contents.end());
        std::vector<std::string> lines;
        boost::split(lines, strContents, boost::is_any_of("\n"));

        static const std::string mountLine = "%1%%2% %3% %4% %5% %6%";

        // Some Android 4.2 ROMs mount the cache partition in the init
        // scripts, so the fstab has no cache line
        bool hasCacheLine = false;

        for (auto it = lines.begin(); it != lines.end(); ++it) {
            auto &line = *it;

            MBP_smatch what;
            bool hasMatch = MBP_regex_search(
                    line, what, MBP_regex(CoreRamdiskPatcher::FstabRegex));

            if (hasMatch && what[3] == Cache) {
                hasCacheLine = true;
            }
        }

        if (!hasCacheLine) {
            std::string cacheLine = "%1% /cache ext4 %2% %3%";
            std::string mountArgs = "nosuid,nodev,barrier=1";
            std::string voldArgs = "wait,check";

            lines.push_back((boost::format(cacheLine) % CachePartition
                             % mountArgs % voldArgs).str());
        }

        strContents = boost::join(lines, "\n");
        contents.assign(strContents.begin(), strContents.end());
        m_impl->cpio->setContents(fstab, std::move(contents));
    }

    return true;
}

static std::string whitespace(const std::string &str) {
    auto nonSpace = std::find_if(str.begin(), str.end(),
                                 std::not1(std::ptr_fun<int, int>(isspace)));
    int count = std::distance(str.begin(), nonSpace);

    return str.substr(0, count);
}

bool QcomRamdiskPatcher::stripManualCacheMounts(const std::string &filename)
{
    auto contents = m_impl->cpio->contents(filename);
    if (contents.empty()) {
        m_impl->error = PatcherError::createCpioError(
                MBP::ErrorCode::CpioFileNotExistError, filename);
        return false;
    }

    std::string strContents(contents.begin(), contents.end());
    std::vector<std::string> lines;
    boost::split(lines, strContents, boost::is_any_of("\n"));

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        if (MBP_regex_search(*it,
                MBP_regex("^\\s+wait\\s+/dev/.*/cache.*$"))
                || MBP_regex_search(*it,
                MBP_regex("^\\s+check_fs\\s+/dev/.*/cache.*$"))
                || MBP_regex_search(*it,
                MBP_regex("^\\s+mount\\s+ext4\\s+/dev/.*/cache.*$"))) {
            // Remove lines that mount /cache
            it->insert(it->begin(), '#');
        }
    }

    strContents = boost::join(lines, "\n");
    contents.assign(strContents.begin(), strContents.end());
    m_impl->cpio->setContents(filename, std::move(contents));

    return true;
}

bool QcomRamdiskPatcher::useGeneratedFstab(const std::string &filename)
{
    auto contents = m_impl->cpio->contents(filename);
    if (contents.empty()) {
        m_impl->error = PatcherError::createCpioError(
                MBP::ErrorCode::CpioFileNotExistError, filename);
        return false;
    }

    std::string strContents(contents.begin(), contents.end());
    std::vector<std::string> lines;
    boost::split(lines, strContents, boost::is_any_of("\n"));

    std::vector<std::string> fstabs;

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        MBP_smatch what;

        if (MBP_regex_search(*it, what,
                MBP_regex("^\\s+mount_all\\s+([^\\s]+)\\s*(#.*)?$"))) {
            // Use fstab generated by mbtool
            std::string spaces = whitespace(*it);

            std::string fstab = what[1];
            std::string completed = "/." + fstab + ".completed";
            std::string generated = "/." + fstab + ".gen";

            // Debugging this: "- exec '/system/bin/sh' failed: No such file or directory (2) -"
            // sure was fun... Turns out service names > 16 characters are rejected
            // See valid_name() in https://android.googlesource.com/platform/system/core/+/master/init/init_parser.c

            // Keep track of fstabs, so we can create services for them
            int index;
            auto it2 = std::find(fstabs.begin(), fstabs.end(), fstab);
            if (it2 == fstabs.end()) {
                index = fstabs.size();
                fstabs.push_back(fstab);
            } else {
                index = it2 - fstabs.begin();
            }

            std::string serviceName =
                    (boost::format("mbtool-mount-%03d") % index).str();

            // Start mounting service
            it = lines.insert(it, spaces + "start " + serviceName);
            // Wait for mount to complete (this sucks, but only custom ROMs
            // implement the 'exec' init script command)
            it = lines.insert(++it, spaces + "wait " + completed + " 15");
            // Mount generated fstab
            ++it;
            *it = spaces + "mount_all " + generated;
        }
    }

    for (unsigned int i = 0; i < fstabs.size(); ++i) {
        std::string serviceName =
                (boost::format("mbtool-mount-%03d") % i).str();

        lines.push_back((boost::format("service %s /mbtool mount_fstab %s")
                % serviceName % fstabs[i]).str());
        lines.push_back("    class core");
        lines.push_back("    critical");
        lines.push_back("    oneshot");
    }

    strContents = boost::join(lines, "\n");
    contents.assign(strContents.begin(), strContents.end());
    m_impl->cpio->setContents(filename, std::move(contents));

    return true;
}
