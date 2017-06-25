/*
 * Copyright (C) 2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#pragma once

#include <string>
#include <vector>

#include "ramdisk_patcher.h"

struct MbBiReader;
struct MbBiWriter;
struct MbFile;

namespace mb
{

class InstallerUtil
{
public:
    static bool unpack_ramdisk(const std::string &input_file,
                               const std::string &output_dir,
                               int &format_out,
                               std::vector<int> &filters_out);
    static bool pack_ramdisk(const std::string &input_dir,
                             const std::string &output_file,
                             int format,
                             const std::vector<int> &filters);

    static bool patch_boot_image(const std::string &input_file,
                                 const std::string &output_file,
                                 std::vector<std::function<RamdiskPatcherFn>> &rps);
    static bool patch_ramdisk(const std::string &input_file,
                              const std::string &output_file,
                              unsigned int depth,
                              std::vector<std::function<RamdiskPatcherFn>> &rps);
    static bool patch_ramdisk_dir(const std::string &ramdisk_dir,
                                  std::vector<std::function<RamdiskPatcherFn>> &rps);
    static bool patch_kernel_rkp(const std::string &input_file,
                                 const std::string &output_file);

    static bool replace_file(const std::string &replace,
                             const std::string &with);

private:
    static bool copy_file_to_file(MbFile *fin, MbFile *fout, uint64_t to_copy);
    static bool copy_file_to_file_eof(MbFile *fin, MbFile *fout);
};

}
