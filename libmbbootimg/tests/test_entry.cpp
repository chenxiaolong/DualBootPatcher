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

#include <gtest/gtest.h>

#include <memory>

#include "mbbootimg/entry.h"

using namespace mb::bootimg;


TEST(BootImgEntryTest, CheckDefaultValues)
{
    Entry entry(EntryType::Kernel);

    ASSERT_FALSE(entry.size());
}

TEST(BootImgEntryTest, CheckGettersSetters)
{
    Entry entry(EntryType::Kernel);

    // Type field

    ASSERT_EQ(entry.type(), EntryType::Kernel);

    // Size field

    entry.set_size(1234);
    auto size = entry.size();
    ASSERT_TRUE(size);
    ASSERT_EQ(*size, 1234u);

    entry.set_size({});
    ASSERT_FALSE(entry.size());
}

TEST(BootImgEntryTest, CheckCopyConstructAndAssign)
{
    Entry entry(EntryType::Ramdisk);

    entry.set_size(1024);

    {
        Entry entry2{entry};

        ASSERT_EQ(entry2.type(), EntryType::Ramdisk);
        auto size = entry2.size();
        ASSERT_TRUE(size);
        ASSERT_EQ(*size, 1024u);
    }

    {
        Entry entry2(EntryType::Kernel);
        entry2 = entry;

        ASSERT_EQ(entry2.type(), EntryType::Ramdisk);
        auto size = entry2.size();
        ASSERT_TRUE(size);
        ASSERT_EQ(*size, 1024u);
    }
}

TEST(BootImgEntryTest, CheckMoveConstructAndAssign)
{
    Entry entry(EntryType::Ramdisk);

    {
        entry.set_size(1024);

        Entry entry2{std::move(entry)};

        ASSERT_EQ(entry2.type(), EntryType::Ramdisk);
        auto size = entry2.size();
        ASSERT_TRUE(size);
        ASSERT_EQ(*size, 1024u);
    }

    {
        entry = Entry(EntryType::Ramdisk);
        entry.set_size(1024);

        Entry entry2(EntryType::Kernel);
        entry2 = std::move(entry);

        ASSERT_EQ(entry2.type(), EntryType::Ramdisk);
        auto size = entry2.size();
        ASSERT_TRUE(size);
        ASSERT_EQ(*size, 1024u);
    }
}

TEST(BootImgEntryTest, CheckEquality)
{
    Entry entry(EntryType::Ramdisk);

    entry.set_size(1024);

    Entry entry2(EntryType::Ramdisk);

    entry2.set_size(1024);

    ASSERT_EQ(entry, entry2);
    entry2.set_size(2048);
    ASSERT_NE(entry, entry2);
}
