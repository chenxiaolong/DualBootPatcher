/*
 * Copyright (C) 2007 The Android Open Source Project
 * Copyright (C) 2018 Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * This is a slight adaptation of TWRP's implementation. Original source code is
 * available at the following link: https://gerrit.omnirom.org/#/c/6019/
 */

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "mbcommon/common.h"

struct LegacyPropArea;
struct LegacyPropInfo;

class LegacyPropertyService
{
public:
    LegacyPropertyService();
    ~LegacyPropertyService();

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(LegacyPropertyService)
    MB_DISABLE_MOVE_CONSTRUCT_AND_ASSIGN(LegacyPropertyService)

    bool initialize();

    bool set(std::string_view name, std::string_view value);

    std::pair<int, size_t> workspace();

private:
    bool m_initialized;
    void *m_data;
    size_t m_size;
    int m_fd;

    LegacyPropArea *m_prop_area;
    LegacyPropInfo *m_prop_infos;

    bool initialize_workspace();

    LegacyPropInfo * find_property(std::string_view name);
    void update_property(LegacyPropInfo *pi, std::string_view value);
};
