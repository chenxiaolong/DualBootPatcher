/*
 * Copyright (C) 2018-2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <vector>

#include "mbcommon/span.h"

using namespace mb;

TEST(SpanTest, CheckConstructors)
{
    {
        span<std::byte> s;
        ASSERT_EQ(s.data(), nullptr);
        ASSERT_EQ(s.size(), 0);
    }
    {
        span<std::byte, 0> s;
        ASSERT_EQ(s.data(), nullptr);
        ASSERT_EQ(s.size(), 0);
    }
    {
        char buf[] = "abc";
        span s(buf, sizeof(buf));
        ASSERT_EQ(s.data(), buf);
        ASSERT_EQ(s.size(), sizeof(buf));
    }
    {
        char buf[] = "abc";
        span s(buf, buf + sizeof(buf));
        ASSERT_EQ(s.data(), buf);
        ASSERT_EQ(s.size(), sizeof(buf));
    }
    {
        char buf[] = "abc";
        span s(buf);
        ASSERT_EQ(s.data(), buf);
        ASSERT_EQ(s.size(), sizeof(buf));
    }
    {
        std::array buf{'a', 'b', 'c'};
        span<char> s(buf);
        ASSERT_EQ(s.data(), buf.data());
        ASSERT_EQ(s.size(), buf.size());
    }
    {
        std::array buf{'a', 'b', 'c'};
        span<char, 3> s(buf);
        ASSERT_EQ(s.data(), buf.data());
        ASSERT_EQ(s.size(), buf.size());
    }
    {
        const std::array buf{'a', 'b', 'c'};
        span<const char> s(buf);
        ASSERT_EQ(s.data(), buf.data());
        ASSERT_EQ(s.size(), buf.size());
    }
    {
        const std::array buf{'a', 'b', 'c'};
        span<const char, 3> s(buf);
        ASSERT_EQ(s.data(), buf.data());
        ASSERT_EQ(s.size(), buf.size());
    }
    {
        std::vector buf{'a', 'b', 'c'};
        span<char> s(buf);
        ASSERT_EQ(s.data(), buf.data());
        ASSERT_EQ(s.size(), buf.size());
    }
    {
        std::vector buf{'a', 'b', 'c'};
        span<char, 3> s(buf);
        ASSERT_EQ(s.data(), buf.data());
        ASSERT_EQ(s.size(), buf.size());
    }
    {
        const std::vector buf{'a', 'b', 'c'};
        span<const char> s(buf);
        ASSERT_EQ(s.data(), buf.data());
        ASSERT_EQ(s.size(), buf.size());
    }
    {
        const std::vector buf{'a', 'b', 'c'};
        span<const char, 3> s(buf);
        ASSERT_EQ(s.data(), buf.data());
        ASSERT_EQ(s.size(), buf.size());
    }
    {
        std::array buf{'a', 'b', 'c'};
        span<char> s(span<char, 3>{buf});
        ASSERT_EQ(s.data(), buf.data());
        ASSERT_EQ(s.size(), buf.size());
    }
    {
        std::array buf{'a', 'b', 'c'};
        span s(buf);
        span s2(s);
        ASSERT_EQ(s2.data(), buf.data());
        ASSERT_EQ(s2.size(), buf.size());
    }
}

TEST(SpanTest, CheckAssignmentOperator)
{
    std::array buf1{'a', 'b', 'c'};
    std::array buf2{'d', 'e', 'f'};

    span s(buf1);
    ASSERT_EQ(s.data(), buf1.data());
    ASSERT_EQ(s.size(), buf1.size());
    s = span(buf2);
    ASSERT_EQ(s.data(), buf2.data());
    ASSERT_EQ(s.size(), buf2.size());
}

TEST(SpanTest, CheckIterators)
{
    std::array buf{'a', 'b', 'c'};
    span s(buf);

    ASSERT_EQ(*buf.begin(), 'a');
    ASSERT_EQ(*buf.cbegin(), 'a');
    ASSERT_EQ(*(buf.end() - 1), 'c');
    ASSERT_EQ(*(buf.cend() - 1), 'c');
    ASSERT_EQ(*buf.rbegin(), 'c');
    ASSERT_EQ(*buf.crbegin(), 'c');
    ASSERT_EQ(*(buf.rend() - 1), 'a');
    ASSERT_EQ(*(buf.crend() - 1), 'a');
}

TEST(SpanTest, CheckFrontBack)
{
    std::array buf{'a', 'b', 'c'};
    span s(buf);

    ASSERT_EQ(s.front(), 'a');
    ASSERT_EQ(s.back(), 'c');
}

TEST(SpanTest, CheckIndexingOperators)
{
    std::array buf{'a', 'b', 'c'};
    span s(buf);

    ASSERT_EQ(s[0], 'a');
    ASSERT_EQ(s[2], 'c');
    ASSERT_EQ(s(0), 'a');
    ASSERT_EQ(s(2), 'c');
}

TEST(SpanTest, CheckSizeEmpty)
{
    std::array buf{1, 2, 3};
    span<int> s(buf);

    ASSERT_EQ(s.size(), buf.size());
    ASSERT_EQ(s.size_bytes(), buf.size() * sizeof(decltype(buf)::value_type));
    ASSERT_FALSE(s.empty());

    s = span<int>();

    ASSERT_EQ(s.size(), 0);
    ASSERT_EQ(s.size_bytes(), 0);
    ASSERT_TRUE(s.empty());
}

TEST(SpanTest, CheckSubViews)
{
    std::array buf{1, 2, 3};

    // Dynamic overload for dynamic spans
    {
        auto s = span<int>(buf).first(2);
        ASSERT_EQ(s.size(), 2);
        ASSERT_EQ(s[0], 1);
        ASSERT_EQ(s[1], 2);
    }
    {
        auto s = span<int>(buf).first(0);
        ASSERT_EQ(s.size(), 0);
    }
    {
        auto s = span<int>(buf).last(2);
        ASSERT_EQ(s.size(), 2);
        ASSERT_EQ(s[0], 2);
        ASSERT_EQ(s[1], 3);
    }
    {
        auto s = span<int>(buf).last(0);
        ASSERT_EQ(s.size(), 0);
    }
    {
        auto s = span<int>(buf).subspan(1);
        ASSERT_EQ(s.size(), 2);
        ASSERT_EQ(s[0], 2);
        ASSERT_EQ(s[1], 3);
    }
    {
        auto s = span<int>(buf).subspan(1, 1);
        ASSERT_EQ(s.size(), 1);
        ASSERT_EQ(s[0], 2);
    }
    {
        auto s = span<int>(buf).subspan(3);
        ASSERT_EQ(s.size(), 0);
    }
    {
        auto s = span<int>(buf).subspan(3, 0);
        ASSERT_EQ(s.size(), 0);
    }

    // Dynamic overload for static spans
    {
        auto s = span<int, 3>(buf).first(2);
        ASSERT_EQ(s.size(), 2);
        ASSERT_EQ(s[0], 1);
        ASSERT_EQ(s[1], 2);
    }
    {
        auto s = span<int, 3>(buf).last(2);
        ASSERT_EQ(s.size(), 2);
        ASSERT_EQ(s[0], 2);
        ASSERT_EQ(s[1], 3);
    }
    {
        auto s = span<int, 3>(buf).subspan(1, 1);
        ASSERT_EQ(s.size(), 1);
        ASSERT_EQ(s[0], 2);
    }

    // Template overload for dynamic span
    {
        auto s = span<int>(buf).first<2>();
        ASSERT_EQ(s.size(), 2);
        ASSERT_EQ(s[0], 1);
        ASSERT_EQ(s[1], 2);
    }
    {
        auto s = span<int>(buf).last<2>();
        ASSERT_EQ(s.size(), 2);
        ASSERT_EQ(s[0], 2);
        ASSERT_EQ(s[1], 3);
    }
    {
        auto s = span<int>(buf).subspan<1, 1>();
        ASSERT_EQ(s.size(), 1);
        ASSERT_EQ(s[0], 2);
    }

    // Template overload for static span
    {
        auto s = span<int, 3>(buf).first<2>();
        ASSERT_EQ(s.size(), 2);
        ASSERT_EQ(s[0], 1);
        ASSERT_EQ(s[1], 2);
    }
    {
        auto s = span<int, 3>(buf).last<2>();
        ASSERT_EQ(s.size(), 2);
        ASSERT_EQ(s[0], 2);
        ASSERT_EQ(s[1], 3);
    }
    {
        auto s = span<int, 3>(buf).subspan<1, 1>();
        ASSERT_EQ(s.size(), 1);
        ASSERT_EQ(s[0], 2);
    }
}

TEST(SpanTest, CheckAsBytes)
{
    std::array buf{0x11111111, 0x22222222, 0x33333333};

    auto s_dynamic_bytes = as_bytes(span<int>(buf));
    ASSERT_TRUE((std::is_same_v<decltype(s_dynamic_bytes),
                 span<const std::byte>>));
    ASSERT_EQ(s_dynamic_bytes.size(), 12);
    ASSERT_EQ(s_dynamic_bytes[0], std::byte(0x11));
    ASSERT_EQ(s_dynamic_bytes[11], std::byte(0x33));

    auto s_static_bytes = as_bytes(span<int, 3>(buf));
    ASSERT_TRUE((std::is_same_v<decltype(s_static_bytes),
                 span<const std::byte, 12>>));
    ASSERT_EQ(s_static_bytes.size(), 12);
    ASSERT_EQ(s_static_bytes[0], std::byte(0x11));
    ASSERT_EQ(s_static_bytes[11], std::byte(0x33));

    auto mutable_bytes = as_writable_bytes(span(buf));
    for (auto &byte : mutable_bytes) {
        byte = ~byte;
    }

    ASSERT_EQ(buf[0], 0xeeeeeeee);
    ASSERT_EQ(buf[1], 0xdddddddd);
    ASSERT_EQ(buf[2], 0xcccccccc);
}

TEST(SpanTest, CheckAsBytesExtension)
{
    std::array buf{1, 2, 3};

    auto s = as_bytes(buf);
    ASSERT_EQ(static_cast<const void *>(s.data()), static_cast<void *>(&buf));
    ASSERT_EQ(s.size(), sizeof(buf));

    auto s2 = as_writable_bytes(buf);
    ASSERT_EQ(static_cast<void *>(s2.data()), static_cast<void *>(&buf));
    ASSERT_EQ(s2.size(), sizeof(buf));

    auto s3 = as_bytes(&buf, sizeof(buf));
    ASSERT_EQ(static_cast<const void *>(s3.data()), static_cast<void *>(&buf));
    ASSERT_EQ(s3.size(), sizeof(buf));

    auto s4 = as_writable_bytes(&buf, sizeof(buf));
    ASSERT_EQ(static_cast<void *>(s4.data()), static_cast<void *>(&buf));
    ASSERT_EQ(s4.size(), sizeof(buf));
}
