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

#pragma once

#include <vector>
#include <cstdint>


#define BUMP_MAGIC \
        "\x41\xa9\xe4\x67\x74\x4d\x1d\x1b\xa4\x29\xf2\xec\xea\x65\x52\x79"
#define BUMP_MAGIC_SIZE 16


class BumpPatcher
{
public:
    static bool patchImage(std::vector<unsigned char> *data);
};