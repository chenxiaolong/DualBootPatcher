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

#include "mbbootimg/entry.h"
#include "mbbootimg/entry_p.h"

typedef std::unique_ptr<MbBiEntry, decltype(mb_bi_entry_free) *> ScopedEntry;


TEST(BootImgEntryTest, CheckDefaultValues)
{
    ScopedEntry entry(mb_bi_entry_new(), mb_bi_entry_free);
    ASSERT_TRUE(!!entry);

    // Check private fields
    ASSERT_EQ(entry->fields_set, 0);
    ASSERT_EQ(entry->field.type, 0);
    ASSERT_EQ(entry->field.name, nullptr);
    ASSERT_EQ(entry->field.size, 0);

    // Check public API
    ASSERT_EQ(mb_bi_entry_type(entry.get()), 0);
    ASSERT_FALSE(mb_bi_entry_type_is_set(entry.get()));
    ASSERT_EQ(mb_bi_entry_name(entry.get()), nullptr);
    ASSERT_EQ(mb_bi_entry_size(entry.get()), 0);
    ASSERT_FALSE(mb_bi_entry_size_is_set(entry.get()));
}

TEST(BootImgEntryTest, CheckGettersSetters)
{
    ScopedEntry entry(mb_bi_entry_new(), mb_bi_entry_free);
    ASSERT_TRUE(!!entry);

    // Type field

    mb_bi_entry_set_type(entry.get(), 1234);
    ASSERT_TRUE(mb_bi_entry_type_is_set(entry.get()));
    ASSERT_TRUE(entry->fields_set & MB_BI_ENTRY_FIELD_TYPE);
    ASSERT_EQ(entry->field.type, 1234);
    ASSERT_EQ(mb_bi_entry_type(entry.get()), 1234);

    mb_bi_entry_unset_type(entry.get());
    ASSERT_FALSE(mb_bi_entry_type_is_set(entry.get()));
    ASSERT_FALSE(entry->fields_set & MB_BI_ENTRY_FIELD_TYPE);
    ASSERT_EQ(entry->field.type, 0);
    ASSERT_EQ(mb_bi_entry_type(entry.get()), 0);

    // Name field

    mb_bi_entry_set_name(entry.get(), "Hello, world!");
    ASSERT_TRUE(entry->fields_set & MB_BI_ENTRY_FIELD_NAME);
    ASSERT_STREQ(entry->field.name, "Hello, world!");
    ASSERT_STREQ(mb_bi_entry_name(entry.get()), "Hello, world!");

    mb_bi_entry_set_name(entry.get(), nullptr);
    ASSERT_FALSE(entry->fields_set & MB_BI_ENTRY_FIELD_NAME);
    ASSERT_EQ(entry->field.name, nullptr);
    ASSERT_EQ(mb_bi_entry_name(entry.get()), nullptr);

    // Size field

    mb_bi_entry_set_size(entry.get(), 1234);
    ASSERT_TRUE(mb_bi_entry_size_is_set(entry.get()));
    ASSERT_TRUE(entry->fields_set & MB_BI_ENTRY_FIELD_SIZE);
    ASSERT_EQ(entry->field.size, 1234);
    ASSERT_EQ(mb_bi_entry_size(entry.get()), 1234);

    mb_bi_entry_unset_size(entry.get());
    ASSERT_FALSE(mb_bi_entry_size_is_set(entry.get()));
    ASSERT_FALSE(entry->fields_set & MB_BI_ENTRY_FIELD_SIZE);
    ASSERT_EQ(entry->field.size, 0);
    ASSERT_EQ(mb_bi_entry_size(entry.get()), 0);
}

TEST(BootImgEntryTest, CheckClone)
{
    ScopedEntry entry(mb_bi_entry_new(), mb_bi_entry_free);
    ASSERT_TRUE(!!entry);

    // Set fields
    entry->fields_set =
            MB_BI_ENTRY_FIELD_TYPE
            | MB_BI_ENTRY_FIELD_NAME
            | MB_BI_ENTRY_FIELD_SIZE;
    entry->field.type = 1;
    entry->field.name = strdup("test");
    ASSERT_NE(entry->field.name, nullptr);
    entry->field.size = 1024;

    ScopedEntry dup(mb_bi_entry_clone(entry.get()), mb_bi_entry_free);
    ASSERT_TRUE(!!dup);

    // Compare values
    ASSERT_EQ(entry->fields_set, dup->fields_set);
    ASSERT_EQ(entry->field.type, dup->field.type);
    ASSERT_NE(entry->field.name, dup->field.name);
    ASSERT_STREQ(entry->field.name, dup->field.name);
    ASSERT_EQ(entry->field.size, dup->field.size);
}

TEST(BootImgEntryTest, CheckClear)
{
    ScopedEntry entry(mb_bi_entry_new(), mb_bi_entry_free);
    ASSERT_TRUE(!!entry);

    // Set fields
    entry->fields_set =
            MB_BI_ENTRY_FIELD_TYPE
            | MB_BI_ENTRY_FIELD_NAME
            | MB_BI_ENTRY_FIELD_SIZE;
    entry->field.type = 1;
    entry->field.name = strdup("test");
    ASSERT_NE(entry->field.name, nullptr);
    entry->field.size = 1024;

    mb_bi_entry_clear(entry.get());

    ASSERT_EQ(entry->fields_set, 0);
    ASSERT_EQ(entry->field.type, 0);
    ASSERT_EQ(entry->field.name, nullptr);
    ASSERT_EQ(entry->field.size, 0);
}
