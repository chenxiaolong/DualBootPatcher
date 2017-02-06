/*
 * Copyright (C) 2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <gtest/gtest.h>

#include <memory>

#include "mbbootimg/reader.h"
#include "mbbootimg/reader_p.h"

typedef std::unique_ptr<MbBiReader, decltype(mb_bi_reader_free) *> ScopedReader;


TEST(BootImgReaderTest, CheckInitialValues)
{
    ScopedReader bir(mb_bi_reader_new(), mb_bi_reader_free);
    ASSERT_TRUE(!!bir);

    // Should start in NEW state
    ASSERT_EQ(bir->state, ReaderState::NEW);

    // No file opened
    ASSERT_EQ(bir->file, nullptr);
    ASSERT_FALSE(bir->file_owned);

    // No error
    ASSERT_EQ(bir->error_code, 0);
    ASSERT_EQ(bir->error_string, nullptr);

    // No formats registered
    ASSERT_EQ(bir->formats_len, 0);
    ASSERT_EQ(bir->format, nullptr);

    // Header and entry allocated
    ASSERT_NE(bir->header, nullptr);
    ASSERT_NE(bir->entry, nullptr);
}
