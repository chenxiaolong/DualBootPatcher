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

#include "mbpatcher/edify/tokenizer.h"

#include <cassert>
#include <cstring>

#include "mbcommon/common.h"
#include "mbcommon/string.h"
#include "mblog/logging.h"

#define LOG_TAG "mbpatcher/edify/tokenizer"


namespace mb::patcher
{

struct EdifyErrorCategory : std::error_category
{
    const char * name() const noexcept override;

    std::string message(int ev) const override;
};

const std::error_category & edify_error_category()
{
    static EdifyErrorCategory c;
    return c;
}

std::error_code make_error_code(EdifyError e)
{
    return {static_cast<int>(e), edify_error_category()};
}

const char * EdifyErrorCategory::name() const noexcept
{
    return "edify";
}

std::string EdifyErrorCategory::message(int ev) const
{
    switch (static_cast<EdifyError>(ev)) {
    case EdifyError::UnterminatedQuote:
        return "unterminated quote";
    case EdifyError::ValueNotQuoted:
        return "value not quoted";
    case EdifyError::InvalidUnquotedCharcter:
        return "invalid unquoted character";
    case EdifyError::UnterminatedEscapeCharacter:
        return "unterminated escape character";
    case EdifyError::IncompleteEscapeSequence:
        return "incomplete escape sequence";
    case EdifyError::InvalidHexEscapeCharacter:
        return "invalid hex escape character";
    case EdifyError::InvalidEscapeCharacter:
        return "invalid escape character";
    default:
        return "(unknown edify error)";
    }
}

oc::result<EdifyTokenString>
EdifyTokenString::from_raw(std::string str, bool quoted)
{
    if (quoted && (str.size() < 2 || str.front() != '"' || str.back() != '"')) {
        return EdifyError::ValueNotQuoted;
    }

    EdifyTokenString token;
    token.m_quoted = quoted;
    if (quoted) {
        token.m_str = str.substr(1, str.size() - 2);
    } else {
        token.m_str = std::move(str);
    }

    return std::move(token);
}

oc::result<EdifyTokenString>
EdifyTokenString::from_string(std::string str, bool make_quoted)
{
    if (!make_quoted) {
        for (char c : str) {
            if (!is_valid_unquoted(c)) {
                return EdifyError::InvalidUnquotedCharcter;
            }
        }
    }

    EdifyTokenString token;
    token.m_quoted = make_quoted;
    if (make_quoted) {
        token.m_str = escape(str);
    } else {
        token.m_str = std::move(str);
    }

    return std::move(token);
}

std::string EdifyTokenString::generate() const
{
    std::string buf;
    buf.reserve(2 + m_str.size());
    if (m_quoted) {
        buf += '"';
    }
    buf += m_str;
    if (m_quoted) {
        buf += '"';
    }
    return buf;
}

oc::result<std::string> EdifyTokenString::unescaped_string() const
{
    if (m_quoted) {
        return unescape(m_str);
    } else {
        return m_str;
    }
}

std::string EdifyTokenString::raw_string() const
{
    return m_str;
}

bool EdifyTokenString::quoted() const
{
    return m_quoted;
}

bool EdifyTokenString::is_valid_unquoted(char c)
{
    return std::isalnum(c)
            || c == '_'
            || c == ':'
            || c == '/'
            || c == '.';
}

std::string EdifyTokenString::escape(std::string_view str)
{
    static constexpr char digits[] = "0123456789abcdef";

    std::string output;
    output.reserve(2 * str.size());

    for (char c : str) {
        if (c == '\a') {
            output += "\\a";
        } else if (c == '\b') {
            output += "\\b";
        } else if (c == '\f') {
            output += "\\f";
        } else if (c == '\n') {
            output += "\\n";
        } else if (c == '\r') {
            output += "\\r";
        } else if (c == '\t') {
            output += "\\t";
        } else if (c == '\v') {
            output += "\\v";
        } else if (std::isprint(c)) {
            output += c;
        } else {
            output += "\\x";
            output += digits[(c >> 4) & 0xf];
            output += digits[c & 0xf];
        }
    }

    return output;
}

static int hex_char_to_int(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'a' && c <= 'f') {
        return c - 'a';
    } else if (c >= 'A' && c <= 'F') {
        return c - 'A';
    } else {
        return -1;
    }
}

oc::result<std::string> EdifyTokenString::unescape(std::string_view str)
{
    std::string output;
    output.reserve(str.size());

    for (auto it = str.begin(); it != str.end();) {
        char c = *it;

        if (c == '\\') {
            if (str.end() - it == 1) {
                // Escape character is last character
                return EdifyError::UnterminatedEscapeCharacter;
            }

            char next = *(it + 1);
            auto new_it = it + 2;

            if (next == 'a') {
                output += '\a';
            } else if (next == 'b') {
                output += '\b';
            } else if (next == 'f') {
                output += '\f';
            } else if (next == 'n') {
                output += '\n';
            } else if (next == 'r') {
                output += '\r';
            } else if (next == 't') {
                output += '\t';
            } else if (next == 'v') {
                output += '\v';
            } else if (next == '\\') {
                output += '\\';
            } else if (next == 'x') {
                if (str.end() - it < 4) {
                    // Need 4 chars: \xYY
                    return EdifyError::IncompleteEscapeSequence;
                }
                int digit1 = hex_char_to_int(*(it + 2));
                int digit2 = hex_char_to_int(*(it + 3));
                if (digit1 < 0 || digit2 < 0) {
                    // One of the chars is not a valid hex character
                    return EdifyError::InvalidHexEscapeCharacter;
                }

                char val = static_cast<char>((digit1 << 4) & digit2);
                output += val;

                new_it += 2;
            } else {
                // Invalid escape char
                return EdifyError::InvalidEscapeCharacter;
            }

            it = new_it;
        } else {
            output += c;
            ++it;
        }
    }

    return std::move(output);
}

static std::string generate_token(const EdifyToken &token)
{
    return std::visit([](auto &&t) {
        return t.generate();
    }, token);
}

oc::result<EdifyToken> EdifyTokenizer::next_token(std::string_view str,
                                                  std::size_t &consumed)
{
    assert(!str.empty());

    if (mb::starts_with(str, {"if", 2})) {
        consumed = 2;
        return EdifyTokenIf();
    } else if (mb::starts_with(str, {"then", 4})) {
        consumed = 4;
        return EdifyTokenThen();
    } else if (mb::starts_with(str, {"else", 4})) {
        consumed = 4;
        return EdifyTokenElse();
    } else if (mb::starts_with(str, {"endif", 4})) {
        consumed = 5;
        return EdifyTokenEndif();
    } else if (mb::starts_with(str, {"&&", 2})) {
        consumed = 2;
        return EdifyTokenAnd();
    } else if (mb::starts_with(str, {"||", 2})) {
        consumed = 2;
        return EdifyTokenOr();
    } else if (mb::starts_with(str, {"==", 2})) {
        consumed = 2;
        return EdifyTokenEquals();
    } else if (mb::starts_with(str, {"!=", 2})) {
        consumed = 2;
        return EdifyTokenNotEquals();
    } else if (str.front() == '!') {
        consumed = 1;
        return EdifyTokenNot();
    } else if (str.front() == '(') {
        consumed = 1;
        return EdifyTokenLeftParen();
    } else if (str.front() == ')') {
        consumed = 1;
        return EdifyTokenRightParen();
    } else if (str.front() == ';') {
        consumed = 1;
        return EdifyTokenSemicolon();
    } else if (str.front() == ',') {
        consumed = 1;
        return EdifyTokenComma();
    } else if (str.front() == '+') {
        consumed = 1;
        return EdifyTokenConcat();
    } else if (str.front() == '\n') {
        consumed = 1;
        return EdifyTokenNewline();
    } else if (char c = str.front(); c != '\n' && std::isspace(c)) {
        std::string buf;
        buf += c;
        consumed = 1;
        for (auto it = str.begin() + 1;
                it != str.end() && *it != '\n' && std::isspace(*it); ++it) {
            buf += *it;
            consumed += 1;
        }
        return EdifyTokenWhitespace(std::move(buf));
    } else if (str.front() == '#') {
        std::string buf;
        // Omit '#' character
        consumed = 1;
        for (auto it = str.begin() + 1; it != str.end() && *it != '\n'; ++it) {
            buf += *it;
            consumed += 1;
        }
        return EdifyTokenComment(std::move(buf));
    } else if (char c = str.front(); EdifyTokenString::is_valid_unquoted(c)) {
        std::string buf;
        buf += c;
        consumed = 1;
        for (auto it = str.begin() + 1;
                it != str.end() && EdifyTokenString::is_valid_unquoted(*it);
                ++it) {
            buf += *it;
            consumed += 1;
        }
        OUTCOME_TRY(r, EdifyTokenString::from_raw(std::move(buf), false));
        return std::move(r);
    } else if (char c = str.front(); c == '"') {
        std::string buf;
        buf += c;
        consumed = 1;
        bool escaped = false;
        bool terminated = false;
        for (auto it = str.begin() + 1; it != str.end(); ++it) {
            if (*it == '\\' || escaped) {
                escaped = !escaped;
            } else if (!escaped && *it == '"') {
                buf += *it;
                consumed += 1;
                terminated = true;
                break;
            }
            buf += *it;
            consumed += 1;
        }
        if (!terminated) {
            return EdifyError::UnterminatedQuote;
        }
        OUTCOME_TRY(r,  EdifyTokenString::from_raw(std::move(buf), true));
        return std::move(r);
    } else {
        consumed = 1;
        return EdifyTokenUnknown(str.front());
    }
}

oc::result<std::vector<EdifyToken>>
EdifyTokenizer::tokenize(std::string_view str)
{
    std::vector<EdifyToken> temp;

    while (true) {
        if (str.empty()) {
            break;
        }

        size_t consumed;
        OUTCOME_TRY(token, next_token(str, consumed));

        str = str.substr(consumed);
        temp.push_back(std::move(token));
    }

    return std::move(temp);
}

std::string EdifyTokenizer::untokenize(const std::vector<EdifyToken> &tokens)
{
    return untokenize(tokens.begin(), tokens.end());
}

std::string EdifyTokenizer::untokenize(std::vector<EdifyToken>::const_iterator begin,
                                       std::vector<EdifyToken>::const_iterator end)
{
    std::string output;
    for (auto it = begin; it != end; ++it) {
        output += generate_token(*it);
    }
    return output;
}

void EdifyTokenizer::dump(const std::vector<EdifyToken> &tokens)
{
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        auto const &t = tokens[i];

        const char *token_name = std::visit(
            [](auto &&arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, EdifyTokenIf>) {
                    return "If";
                } else if constexpr (std::is_same_v<T, EdifyTokenThen>) {
                    return "Then";
                } else if constexpr (std::is_same_v<T, EdifyTokenElse>) {
                    return "Else";
                } else if constexpr (std::is_same_v<T, EdifyTokenEndif>) {
                    return "Endif";
                } else if constexpr (std::is_same_v<T, EdifyTokenAnd>) {
                    return "And";
                } else if constexpr (std::is_same_v<T, EdifyTokenOr>) {
                    return "Or";
                } else if constexpr (std::is_same_v<T, EdifyTokenEquals>) {
                    return "Equals";
                } else if constexpr (std::is_same_v<T, EdifyTokenNotEquals>) {
                    return "NotEquals";
                } else if constexpr (std::is_same_v<T, EdifyTokenNot>) {
                    return "Not";
                } else if constexpr (std::is_same_v<T, EdifyTokenLeftParen>) {
                    return "LeftParen";
                } else if constexpr (std::is_same_v<T, EdifyTokenRightParen>) {
                    return "RightParen";
                } else if constexpr (std::is_same_v<T, EdifyTokenSemicolon>) {
                    return "Semicolon";
                } else if constexpr (std::is_same_v<T, EdifyTokenComma>) {
                    return "Comma";
                } else if constexpr (std::is_same_v<T, EdifyTokenConcat>) {
                    return "Concat";
                } else if constexpr (std::is_same_v<T, EdifyTokenNewline>) {
                    return "Newline";
                } else if constexpr (std::is_same_v<T, EdifyTokenWhitespace>) {
                    return "Whitespace";
                } else if constexpr (std::is_same_v<T, EdifyTokenComment>) {
                    return "Comment";
                } else if constexpr (std::is_same_v<T, EdifyTokenString>) {
                    return "String";
                } else if constexpr (std::is_same_v<T, EdifyTokenUnknown>) {
                    return "Unknown";
                }
            },
            t
        );

        LOGD("%" MB_PRIzu ": %-20s: %s",
             i, token_name, generate_token(t).c_str());
    }
}

}
