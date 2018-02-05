/*
 * Copyright (C) 2015-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <string>
#include <string_view>
#include <vector>
#include <variant>

#include <cassert>

#include "mbcommon/common.h"
#include "mbcommon/outcome.h"

namespace mb::patcher
{

enum class EdifyError
{
    UnterminatedQuote,
    ValueNotQuoted,
    InvalidUnquotedCharcter,
    UnterminatedEscapeCharacter,
    IncompleteEscapeSequence,
    InvalidHexEscapeCharacter,
    InvalidEscapeCharacter,
};

MB_EXPORT std::error_code make_error_code(EdifyError e);

MB_EXPORT const std::error_category & edify_error_category();

class EdifyTokenIf
{
public:
    std::string generate() const
    {
        return "if";
    }
};

class EdifyTokenThen
{
public:
    std::string generate() const
    {
        return "then";
    }
};

class EdifyTokenElse
{
public:
    std::string generate() const
    {
        return "else";
    }
};

class EdifyTokenEndif
{
public:
    std::string generate() const
    {
        return "endif";
    }
};

class EdifyTokenAnd
{
public:
    std::string generate() const
    {
        return "&&";
    }
};

class EdifyTokenOr
{
public:
    std::string generate() const
    {
        return "||";
    }
};

class EdifyTokenEquals
{
public:
    std::string generate() const
    {
        return "==";
    }
};

class EdifyTokenNotEquals
{
public:
    std::string generate() const
    {
        return "!=";
    }
};

class EdifyTokenNot
{
public:
    std::string generate() const
    {
        return "!";
    }
};

class EdifyTokenLeftParen
{
public:
    std::string generate() const
    {
        return "(";
    }
};

class EdifyTokenRightParen
{
public:
    std::string generate() const
    {
        return ")";
    }
};

class EdifyTokenSemicolon
{
public:
    std::string generate() const
    {
        return ";";
    }
};

class EdifyTokenComma
{
public:
    std::string generate() const
    {
        return ",";
    }
};

class EdifyTokenConcat
{
public:
    std::string generate() const
    {
        return "+";
    }
};

class EdifyTokenNewline
{
public:
    std::string generate() const
    {
        return "\n";
    }
};

class EdifyTokenWhitespace
{
public:
    EdifyTokenWhitespace(std::string str) : m_str(std::move(str))
    {
        assert(!m_str.empty());

        for (char c [[maybe_unused]] : m_str) {
            assert(std::isspace(c));
        }
    }

    std::string generate() const
    {
        return m_str;
    }

private:
    std::string m_str;
};

class EdifyTokenComment
{
public:
    EdifyTokenComment(std::string str) : m_str(std::move(str))
    {
    }

    std::string generate() const
    {
        return '#' + m_str;
    }

private:
    std::string m_str;
};

class EdifyTokenString
{
public:
    static oc::result<EdifyTokenString> from_raw(std::string str, bool quoted);
    static oc::result<EdifyTokenString> from_string(std::string str, bool make_quoted);

    std::string generate() const;

    oc::result<std::string> unescaped_string() const;

    std::string raw_string() const;

    bool quoted() const;

    static bool is_valid_unquoted(char c);

protected:
    std::string m_str;
    bool m_quoted;

    EdifyTokenString() = default;

    static std::string escape(std::string_view str);
    static oc::result<std::string> unescape(std::string_view str);
};

class EdifyTokenUnknown
{
public:
    EdifyTokenUnknown(char c) : m_char(c)
    {
    }

    std::string generate() const
    {
        return std::string(1, m_char);
    }

private:
    char m_char;
};

using EdifyToken = std::variant<
    EdifyTokenIf,
    EdifyTokenThen,
    EdifyTokenElse,
    EdifyTokenEndif,
    EdifyTokenAnd,
    EdifyTokenOr,
    EdifyTokenEquals,
    EdifyTokenNotEquals,
    EdifyTokenNot,
    EdifyTokenLeftParen,
    EdifyTokenRightParen,
    EdifyTokenSemicolon,
    EdifyTokenComma,
    EdifyTokenConcat,
    EdifyTokenNewline,
    EdifyTokenWhitespace,
    EdifyTokenComment,
    EdifyTokenString,
    EdifyTokenUnknown
>;

class EdifyTokenizer
{
public:
    static oc::result<std::vector<EdifyToken>> tokenize(std::string_view str);
    static std::string untokenize(const std::vector<EdifyToken> &tokens);
    static std::string untokenize(std::vector<EdifyToken>::const_iterator begin,
                                  std::vector<EdifyToken>::const_iterator end);

    static void dump(const std::vector<EdifyToken> &tokens);

private:
    static oc::result<EdifyToken> next_token(std::string_view str,
                                             std::size_t &consumed);

    MB_DISABLE_DEFAULT_CONSTRUCTOR(EdifyTokenizer)
    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(EdifyTokenizer)
    MB_DISABLE_MOVE_CONSTRUCT_AND_ASSIGN(EdifyTokenizer)
};

}

namespace std
{
    template<>
    struct MB_EXPORT is_error_code_enum<mb::patcher::EdifyError> : true_type
    {
    };
}
