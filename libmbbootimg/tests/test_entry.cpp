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
    Entry entry;

    ASSERT_FALSE(entry.type());
    ASSERT_FALSE(entry.size());
}

TEST(BootImgEntryTest, CheckGettersSetters)
{
    Entry entry;

    // Type field

    entry.set_type(EntryType::Kernel);
    auto type = entry.type();
    ASSERT_TRUE(type);
    ASSERT_EQ(*type, EntryType::Kernel);

    entry.set_type({});
    ASSERT_FALSE(entry.type());

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
    Entry entry;

    entry.set_type(EntryType::Ramdisk);
    entry.set_size(1024);

    {
        Entry entry2{entry};

        auto type = entry2.type();
        ASSERT_TRUE(type);
        ASSERT_EQ(*type, EntryType::Ramdisk);
        auto size = entry2.size();
        ASSERT_TRUE(size);
        ASSERT_EQ(*size, 1024u);
    }

    {
        Entry entry2;
        entry2 = entry;

        auto type = entry2.type();
        ASSERT_TRUE(type);
        ASSERT_EQ(*type, EntryType::Ramdisk);
        auto size = entry2.size();
        ASSERT_TRUE(size);
        ASSERT_EQ(*size, 1024u);
    }
}

TEST(BootImgEntryTest, CheckMoveConstructAndAssign)
{
    Entry entry;

    {
        entry.set_type(EntryType::Ramdisk);
        entry.set_size(1024);

        Entry entry2{std::move(entry)};

        auto type = entry2.type();
        ASSERT_TRUE(type);
        ASSERT_EQ(*type, EntryType::Ramdisk);
        auto size = entry2.size();
        ASSERT_TRUE(size);
        ASSERT_EQ(*size, 1024u);
    }

    {
        entry.set_type(EntryType::Ramdisk);
        entry.set_size(1024);

        Entry entry2;
        entry2 = std::move(entry);

        auto type = entry2.type();
        ASSERT_TRUE(type);
        ASSERT_EQ(*type, EntryType::Ramdisk);
        auto size = entry2.size();
        ASSERT_TRUE(size);
        ASSERT_EQ(*size, 1024u);
    }
}

TEST(BootImgEntryTest, CheckEquality)
{
    Entry entry;

    entry.set_type(EntryType::Ramdisk);
    entry.set_size(1024);

    Entry entry2;

    entry2.set_type(EntryType::Ramdisk);
    entry2.set_size(1024);

    ASSERT_EQ(entry, entry2);
    entry2.set_type(EntryType::SecondBoot);
    ASSERT_NE(entry, entry2);
}

TEST(BootImgEntryTest, CheckClear)
{
    Entry entry;

    entry.set_type(EntryType::Ramdisk);
    entry.set_size(1024);

    entry.clear();

    ASSERT_FALSE(entry.type());
    ASSERT_FALSE(entry.size());
}
