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

#include "autopatchers/jflte/jfltepatcher.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>

#include "autopatchers/standard/standardpatcher.h"
#include "private/fileutils.h"
#include "private/regex.h"


/*! \cond INTERNAL */
class JflteBasePatcher::Impl
{
public:
    const PatcherConfig *pc;
    std::string id;
    const FileInfo *info;
};
/*! \endcond */


static const std::string AromaScript =
        "META-INF/com/google/android/aroma-config";
static const std::string BuildProp =
        "system/build.prop";
static const std::string QcomAudioScript =
        "system/etc/init.qcom.audio.sh";

static const std::string System = "/system";
static const std::string Cache = "/cache";
static const std::string Data = "/data";


JflteBasePatcher::JflteBasePatcher(const PatcherConfig * const pc,
                                   const FileInfo * const info)
    : m_impl(new Impl())
{
    m_impl->pc = pc;
    m_impl->info = info;
}

JflteBasePatcher::~JflteBasePatcher()
{
}

PatcherError JflteBasePatcher::error() const
{
    return PatcherError();
}

////////////////////////////////////////////////////////////////////////////////

const std::string JflteDalvikCachePatcher::Id = "DalvikCachePatcher";

JflteDalvikCachePatcher::JflteDalvikCachePatcher(const PatcherConfig * const pc,
                                                 const FileInfo* const info)
    : JflteBasePatcher(pc, info)
{
}

std::string JflteDalvikCachePatcher::id() const
{
    return Id;
}

std::vector<std::string> JflteDalvikCachePatcher::newFiles() const
{
    return std::vector<std::string>();
}

std::vector<std::string> JflteDalvikCachePatcher::existingFiles() const
{
    std::vector<std::string> files;
    files.push_back(BuildProp);
    return files;
}

bool JflteDalvikCachePatcher::patchFiles(const std::string &directory,
                                         const std::vector<std::string> &bootImages)
{
    (void) bootImages;

    std::vector<unsigned char> contents;

    // BuildProp begin
    FileUtils::readToMemory(directory + "/" + BuildProp, &contents);
    std::string strContents(contents.begin(), contents.end());
    std::vector<std::string> lines;
    boost::split(lines, strContents, boost::is_any_of("\n"));

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        if (it->find("dalvik.vm.dexopt-data-only") != std::string::npos) {
            lines.erase(it);
            break;
        }
    }

    strContents = boost::join(lines, "\n");
    contents.assign(strContents.begin(), strContents.end());
    FileUtils::writeFromMemory(directory + "/" + BuildProp, contents);
    // BuildProp end

    return true;
}

////////////////////////////////////////////////////////////////////////////////

const std::string JflteGoogleEditionPatcher::Id = "GoogleEditionPatcher";

JflteGoogleEditionPatcher::JflteGoogleEditionPatcher(const PatcherConfig * const pc,
                                                     const FileInfo* const info)
    : JflteBasePatcher(pc, info)
{
}

std::string JflteGoogleEditionPatcher::id() const
{
    return Id;
}

std::vector<std::string> JflteGoogleEditionPatcher::newFiles() const
{
    return std::vector<std::string>();
}

std::vector<std::string> JflteGoogleEditionPatcher::existingFiles() const
{
    std::vector<std::string> files;
    files.push_back(QcomAudioScript);
    return files;
}

bool JflteGoogleEditionPatcher::patchFiles(const std::string &directory,
                                           const std::vector<std::string> &bootImages)
{
    (void) bootImages;

    std::vector<unsigned char> contents;

    // QcomAudioScript begin
    FileUtils::readToMemory(directory + "/" + QcomAudioScript, &contents);
    std::string strContents(contents.begin(), contents.end());
    std::vector<std::string> lines;
    boost::split(lines, strContents, boost::is_any_of("\n"));

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        if (it->find("snd_soc_msm_2x_Fusion3_auxpcm") != std::string::npos) {
            it = lines.erase(it);
        }
    }

    strContents = boost::join(lines, "\n");
    contents.assign(strContents.begin(), strContents.end());
    FileUtils::writeFromMemory(directory + "/" + QcomAudioScript, contents);
    // QcomAudioScript end

    return true;
}

////////////////////////////////////////////////////////////////////////////////

const std::string JflteSlimAromaBundledMount::Id = "SlimAromaBundledMount";

JflteSlimAromaBundledMount::JflteSlimAromaBundledMount(const PatcherConfig * const pc,
                                                       const FileInfo* const info)
    : JflteBasePatcher(pc, info)
{
}

std::string JflteSlimAromaBundledMount::id() const
{
    return Id;
}

std::vector<std::string> JflteSlimAromaBundledMount::newFiles() const
{
    return std::vector<std::string>();
}

std::vector<std::string> JflteSlimAromaBundledMount::existingFiles() const
{
    std::vector<std::string> files;
    files.push_back(StandardPatcher::UpdaterScript);
    return files;
}

bool JflteSlimAromaBundledMount::patchFiles(const std::string &directory,
                                            const std::vector<std::string> &bootImages)
{
    (void) bootImages;

    std::vector<unsigned char> contents;

    // StandardPatcher::UpdaterScript begin
    FileUtils::readToMemory(directory + "/" + StandardPatcher::UpdaterScript, &contents);
    std::string strContents(contents.begin(), contents.end());
    std::vector<std::string> lines;
    boost::split(lines, strContents, boost::is_any_of("\n"));

    for (auto it = lines.begin(); it != lines.end();) {
        if (MBP_regex_search(*it, MBP_regex("/tmp/mount.*/system"))) {
            it = lines.erase(it);
            it = StandardPatcher::insertMountSystem(it, &lines);
        } else if (MBP_regex_search(*it, MBP_regex("/tmp/mount.*/cache"))) {
            it = lines.erase(it);
            it = StandardPatcher::insertMountCache(it, &lines);
        } else if (MBP_regex_search(*it, MBP_regex("/tmp/mount.*/data"))) {
            it = lines.erase(it);
            it = StandardPatcher::insertMountData(it, &lines);
        } else {
            ++it;
        }
    }

    strContents = boost::join(lines, "\n");
    contents.assign(strContents.begin(), strContents.end());
    FileUtils::writeFromMemory(directory + "/" + StandardPatcher::UpdaterScript, contents);
    // StandardPatcher::UpdaterScript end

    return true;
}

////////////////////////////////////////////////////////////////////////////////

const std::string JflteImperiumPatcher::Id = "ImperiumPatcher";

JflteImperiumPatcher::JflteImperiumPatcher(const PatcherConfig * const pc,
                                           const FileInfo* const info)
    : JflteBasePatcher(pc, info)
{
}

std::string JflteImperiumPatcher::id() const
{
    return Id;
}

std::vector<std::string> JflteImperiumPatcher::newFiles() const
{
    return std::vector<std::string>();
}

std::vector<std::string> JflteImperiumPatcher::existingFiles() const
{
    std::vector<std::string> files;
    files.push_back(StandardPatcher::UpdaterScript);
    return files;
}

bool JflteImperiumPatcher::patchFiles(const std::string &directory,
                                      const std::vector<std::string> &bootImages)
{
    (void) bootImages;

    std::vector<unsigned char> contents;

    // StandardPatcher::UpdaterScript begin
    FileUtils::readToMemory(directory + "/" + StandardPatcher::UpdaterScript, &contents);
    std::string strContents(contents.begin(), contents.end());
    std::vector<std::string> lines;
    boost::split(lines, strContents, boost::is_any_of("\n"));

    StandardPatcher::insertDualBootSh(&lines, true);
    StandardPatcher::replaceMountLines(&lines, m_impl->info->device());
    StandardPatcher::replaceUnmountLines(&lines, m_impl->info->device());
    StandardPatcher::replaceFormatLines(&lines, m_impl->info->device());
    StandardPatcher::insertUnmountEverything(lines.end(), &lines);

    // Insert set kernel line
    const std::string setKernelLine =
            "run_program(\"/tmp/dualboot.sh\", \"set-multi-kernel\");";
    for (auto it = lines.rbegin(); it != lines.rend(); ++it) {
        if (it->find("Umounting Partitions") != std::string::npos) {
            auto fwdIt = (++it).base();
            lines.insert(++fwdIt, setKernelLine);
            break;
        }
    }

    strContents = boost::join(lines, "\n");
    contents.assign(strContents.begin(), strContents.end());
    FileUtils::writeFromMemory(directory + "/" + StandardPatcher::UpdaterScript, contents);
    // StandardPatcher::UpdaterScript end

    return true;
}

////////////////////////////////////////////////////////////////////////////////

const std::string JflteNegaliteNoWipeData::Id = "NegaliteNoWipeData";

JflteNegaliteNoWipeData::JflteNegaliteNoWipeData(const PatcherConfig * const pc,
                                                 const FileInfo* const info)
    : JflteBasePatcher(pc, info)
{
}

std::string JflteNegaliteNoWipeData::id() const
{
    return Id;
}

std::vector<std::string> JflteNegaliteNoWipeData::newFiles() const
{
    return std::vector<std::string>();
}

std::vector<std::string> JflteNegaliteNoWipeData::existingFiles() const
{
    std::vector<std::string> files;
    files.push_back(StandardPatcher::UpdaterScript);
    return files;
}

bool JflteNegaliteNoWipeData::patchFiles(const std::string &directory,
                                         const std::vector<std::string> &bootImages)
{
    (void) bootImages;

    std::vector<unsigned char> contents;

    // StandardPatcher::UpdaterScript begin
    FileUtils::readToMemory(directory + "/" + StandardPatcher::UpdaterScript, &contents);
    std::string strContents(contents.begin(), contents.end());
    std::vector<std::string> lines;
    boost::split(lines, strContents, boost::is_any_of("\n"));

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        if (MBP_regex_search(*it, MBP_regex("run_program.*/tmp/wipedata.sh"))) {
            it = lines.erase(it);
            StandardPatcher::insertFormatData(it, &lines);
            break;
        }
    }

    strContents = boost::join(lines, "\n");
    contents.assign(strContents.begin(), strContents.end());
    FileUtils::writeFromMemory(directory + "/" + StandardPatcher::UpdaterScript, contents);
    // StandardPatcher::UpdaterScript end

    return true;
}

////////////////////////////////////////////////////////////////////////////////

const std::string JflteTriForceFixAroma::Id = "TriForceFixAroma";

JflteTriForceFixAroma::JflteTriForceFixAroma(const PatcherConfig * const pc,
                                             const FileInfo* const info)
    : JflteBasePatcher(pc, info)
{
}

std::string JflteTriForceFixAroma::id() const
{
    return Id;
}

std::vector<std::string> JflteTriForceFixAroma::newFiles() const
{
    return std::vector<std::string>();
}

std::vector<std::string> JflteTriForceFixAroma::existingFiles() const
{
    std::vector<std::string> files;
    files.push_back(AromaScript);
    return files;
}

bool JflteTriForceFixAroma::patchFiles(const std::string &directory,
                                       const std::vector<std::string> &bootImages)
{
    (void) bootImages;

    std::vector<unsigned char> contents;

    // AromaScript begin
    FileUtils::readToMemory(directory + "/" + AromaScript, &contents);
    std::string strContents(contents.begin(), contents.end());
    std::vector<std::string> lines;
    boost::split(lines, strContents, boost::is_any_of("\n"));

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        if (it->find(BuildProp) != std::string::npos) {
            // Remove 'raw-' since aroma mounts the partitions directly
            std::string target = m_impl->info->partConfig()->targetSystem();
            boost::replace_all(target, "raw-", "");
            boost::replace_all(*it, System, target);
        } else if (MBP_regex_search(*it, MBP_regex("/sbin/mount.+/system"))) {
            it = lines.insert(it, boost::replace_all_copy(*it, System, Cache));
            ++it;
            it = lines.insert(it, boost::replace_all_copy(*it, System, Data));
            ++it;
        }
    }

    strContents = boost::join(lines, "\n");
    contents.assign(strContents.begin(), strContents.end());
    FileUtils::writeFromMemory(directory + "/" + AromaScript, contents);
    // AromaScript end

    return true;
}

////////////////////////////////////////////////////////////////////////////////

const std::string JflteTriForceFixUpdate::Id = "TriForceFixUpdate";

JflteTriForceFixUpdate::JflteTriForceFixUpdate(const PatcherConfig * const pc,
                                               const FileInfo* const info)
    : JflteBasePatcher(pc, info)
{
}

std::string JflteTriForceFixUpdate::id() const
{
    return Id;
}

std::vector<std::string> JflteTriForceFixUpdate::newFiles() const
{
    return std::vector<std::string>();
}

std::vector<std::string> JflteTriForceFixUpdate::existingFiles() const
{
    std::vector<std::string> files;
    files.push_back(StandardPatcher::UpdaterScript);
    return files;
}

bool JflteTriForceFixUpdate::patchFiles(const std::string &directory,
                                        const std::vector<std::string> &bootImages)
{
    (void) bootImages;

    std::vector<unsigned char> contents;

    // StandardPatcher::UpdaterScript begin
    FileUtils::readToMemory(directory + "/" + StandardPatcher::UpdaterScript, &contents);
    std::string strContents(contents.begin(), contents.end());
    std::vector<std::string> lines;
    boost::split(lines, strContents, boost::is_any_of("\n"));

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        if (MBP_regex_search(*it, MBP_regex("getprop.+/system/build.prop"))) {
            it = StandardPatcher::insertMountSystem(it, &lines);
            it = StandardPatcher::insertMountCache(it, &lines);
            it = StandardPatcher::insertMountData(it, &lines);
            boost::replace_all(*it, System,
                               m_impl->info->partConfig()->targetSystem());
        }
    }

    strContents = boost::join(lines, "\n");
    contents.assign(strContents.begin(), strContents.end());
    FileUtils::writeFromMemory(directory + "/" + StandardPatcher::UpdaterScript, contents);
    // StandardPatcher::UpdaterScript end

    return true;
}
