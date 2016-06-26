/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <string>
#include <unordered_map>
#include <vector>

class BootImageFormat;

struct BootImageIntermediate
{
    // Used in:                                  | Android | Loki | Bump | Mtk | Sony |
    uint32_t kernelAddr = 0;                  // | X       | X    | X    | X   | X    |
    uint32_t ramdiskAddr = 0;                 // | X       | X    | X    | X   | X    |
    uint32_t secondAddr = 0;                  // | X       | X    | X    | X   |      |
    uint32_t tagsAddr = 0;                    // | X       | X    | X    | X   |      |
    uint32_t iplAddr = 0;                     // |         |      |      |     | X    |
    uint32_t rpmAddr = 0;                     // |         |      |      |     | X    |
    uint32_t appsblAddr = 0;                  // |         |      |      |     | X    |
    uint32_t pageSize = 0;                    // | X       | X    | X    | X   |      |
    std::string boardName;                    // | X       | X    | X    | X   |      |
    std::string cmdline;                      // | X       | X    | X    | X   |      |
    std::vector<unsigned char> kernelImage;   // | X       | X    | X    | X   | X    |
    std::vector<unsigned char> ramdiskImage;  // | X       | X    | X    | X   | X    |
    std::vector<unsigned char> secondImage;   // | X       | X    | X    | X   |      |
    std::vector<unsigned char> dtImage;       // | X       | X    | X    | X   |      |
    std::vector<unsigned char> abootImage;    // |         | X    |      |     |      |
    std::vector<unsigned char> mtkKernelHdr;  // |         |      |      | X   |      |
    std::vector<unsigned char> mtkRamdiskHdr; // |         |      |      | X   |      |
    std::vector<unsigned char> iplImage;      // |         |      |      |     | X    |
    std::vector<unsigned char> rpmImage;      // |         |      |      |     | X    |
    std::vector<unsigned char> appsblImage;   // |         |      |      |     | X    |
    std::vector<unsigned char> sonySinImage;  // |         |      |      |     | X    |
    std::vector<unsigned char> sonySinHdr;    // |         |      |      |     | X    |
    // Raw header values                         |---------|------|------|-----|------|
    uint32_t hdrKernelSize = 0;               // | X       | X    | X    | X   |      |
    uint32_t hdrRamdiskSize = 0;              // | X       | X    | X    | X   |      |
    uint32_t hdrSecondSize = 0;               // | X       | X    | X    | X   |      |
    uint32_t hdrDtSize = 0;                   // | X       | X    | X    | X   |      |
    uint32_t hdrUnused = 0;                   // | X       | X    | X    | X   |      |
    uint32_t hdrId[8] = { 0 };                // | X       | X    | X    | X   |      |
    uint32_t hdrEntrypoint = 0;               // |         |      |      |     | X    |
};
