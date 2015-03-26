/*
 * Copyright (C) 2014  Anthony King
 * Copyright (C) 2014  CyboLabs
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * This is a C++ adaptation of open_bump.py and is licensed under the same
 * terms as the original source code. It can be used standalone (outside of
 * MultiBootPatcher) under the terms above.
 */

#include "bootimage/bumppatcher.h"

#include <string.h>


// BootImage creates a properly padded boot image file, so can directly append
// the magic.
bool BumpPatcher::patchImage(std::vector<unsigned char> *data)
{
    data->insert(data->end(), BUMP_MAGIC, BUMP_MAGIC + BUMP_MAGIC_SIZE);
    return true;
}