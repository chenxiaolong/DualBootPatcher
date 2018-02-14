/*
 * Copyright (C) 2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of DualBootPatcher
 *
 * DualBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DualBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DualBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mbpatcher/autopatchers/magiskpatcher.h"

#include <cstring>

#include "mbcommon/string.h"

#include "mbpatcher/autopatchers/standardpatcher.h"
#include "mbpatcher/private/fileutils.h"


namespace mb::patcher
{

const std::string MagiskPatcher::Id = "MagiskPatcher";

static const std::string AddonDScript = "addon.d/99-magisk.sh";
static const std::string UtilFunctions = "common/util_functions.sh";


MagiskPatcher::MagiskPatcher(const PatcherConfig &pc, const FileInfo &info)
{
    (void) pc;
    (void) info;
}

MagiskPatcher::~MagiskPatcher() = default;

ErrorCode MagiskPatcher::error() const
{
    return {};
}

std::string MagiskPatcher::id() const
{
    return Id;
}

std::vector<std::string> MagiskPatcher::new_files() const
{
    return {};
}

std::vector<std::string> MagiskPatcher::existing_files() const
{
    return { StandardPatcher::UpdaterScript, AddonDScript, UtilFunctions };
}

static void replace_all(std::string &source, std::string_view from,
                        std::string_view to)
{
    if (from.empty()) {
        return;
    }

    std::size_t pos = 0;
    while ((pos = source.find(from, pos)) != std::string::npos) {
        source.replace(pos, from.size(), to);
        pos += to.size();
    }
}

static bool patch_file(const std::string &path, bool is_updater)
{
    std::string contents;

    ErrorCode ret = FileUtils::read_to_string(path, &contents);
    if (ret != ErrorCode::NoError) {
        return false;
    }

    if (is_updater && !starts_with(contents, "#MAGISK")) {
        return true;
    }

    replace_all(contents, "mount /data", "/update-binary-tool mount /data");
    replace_all(contents, "mount /cache", "/update-binary-tool mount /cache");
    replace_all(contents, "mount -o ro /system", "/update-binary-tool mount /system");

    // Not a dual boot bug, but the installer uses a Java program to determine
    // if a boot image is signed and that program fails to run properly on
    // certain devices. /system/bin/dalvikvm would always exit with status 0,
    // but nothing would be executed.
    replace_all(contents, "BOOTSIGNED=true", "BOOTSIGNED=false");

    // Also not a dual boot bug, but on AOSP-style custom ROMs, Magisk installs
    // an addon.d script to repatch the boot image during the installation.
    // However, because the post-install hook executes before the boot image is
    // flashed, the script forks a background process and waits 5 seconds before
    // continuing. This race condition leads to a corrupted boot image on
    // devices with slow internal storage like the Galaxy S4.
    replace_all(contents, "sleep 5", "sleep 10");

    FileUtils::write_from_string(path, contents);

    return true;
}

bool MagiskPatcher::patch_files(const std::string &directory)
{
    patch_file(directory + "/" + StandardPatcher::UpdaterScript, true);
    patch_file(directory + "/" + AddonDScript, false);
    patch_file(directory + "/" + UtilFunctions, false);

    // Don't fail if an error occurs
    return true;
}

}
