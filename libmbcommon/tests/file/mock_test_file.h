/*
 * Copyright (C) 2017-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <vector>

#include <gmock/gmock.h>

#include "mbcommon/file.h"

class MockFile : public mb::File
{
public:
    MockFile() = default;
    virtual ~MockFile() = default;

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(MockFile)

    MOCK_METHOD0(close, mb::oc::result<void>());
    MOCK_METHOD2(read, mb::oc::result<size_t>(void *buf, size_t size));
    MOCK_METHOD2(write, mb::oc::result<size_t>(const void *buf, size_t size));
    MOCK_METHOD2(seek, mb::oc::result<uint64_t>(int64_t offset, int whence));
    MOCK_METHOD1(truncate, mb::oc::result<void>(uint64_t size));
    MOCK_METHOD0(is_open, bool());
};
