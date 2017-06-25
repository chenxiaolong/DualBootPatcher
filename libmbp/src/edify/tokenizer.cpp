/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbp/edify/tokenizer.h"

#include <cassert>
#include <cstring>

#include "mbcommon/common.h"
#include "mbcommon/string.h"
#include "mblog/logging.h"

#include "mbp/private/stringutils.h"

namespace mbp
{

EdifyToken::~EdifyToken()
{
}

EdifyToken::EdifyToken(EdifyTokenType type) : m_type(type)
{
}

EdifyTokenType EdifyToken::type() const
{
    return m_type;
}

////////////////////////////////////////////////////////////////////////////////

EdifyKeywordToken::EdifyKeywordToken(EdifyTokenType type, std::string keyword)
    : EdifyToken(type), m_keyword(std::move(keyword))
{
}

std::string EdifyKeywordToken::generate()
{
    return m_keyword;
}

////////////////////////////////////////////////////////////////////////////////

EdifyTokenIf::EdifyTokenIf()
    : EdifyKeywordToken(EdifyTokenType::If, "if")
{
}

////////////////////////////////////////////////////////////////////////////////

EdifyTokenThen::EdifyTokenThen()
    : EdifyKeywordToken(EdifyTokenType::Then, "then")
{
}

////////////////////////////////////////////////////////////////////////////////

EdifyTokenElse::EdifyTokenElse()
    : EdifyKeywordToken(EdifyTokenType::Else, "else")
{
}

////////////////////////////////////////////////////////////////////////////////

EdifyTokenEndif::EdifyTokenEndif()
    : EdifyKeywordToken(EdifyTokenType::Endif, "endif")
{
}

////////////////////////////////////////////////////////////////////////////////

EdifyTokenAnd::EdifyTokenAnd()
    : EdifyKeywordToken(EdifyTokenType::And, "&&")
{
}

////////////////////////////////////////////////////////////////////////////////

EdifyTokenOr::EdifyTokenOr()
    : EdifyKeywordToken(EdifyTokenType::Or, "||")
{
}

////////////////////////////////////////////////////////////////////////////////

EdifyTokenEquals::EdifyTokenEquals()
    : EdifyKeywordToken(EdifyTokenType::Equals, "==")
{
}

////////////////////////////////////////////////////////////////////////////////

EdifyTokenNotEquals::EdifyTokenNotEquals()
    : EdifyKeywordToken(EdifyTokenType::NotEquals, "!=")
{
}

////////////////////////////////////////////////////////////////////////////////

EdifyTokenNot::EdifyTokenNot()
    : EdifyKeywordToken(EdifyTokenType::Not, "!")
{
}

////////////////////////////////////////////////////////////////////////////////

EdifyTokenLeftParen::EdifyTokenLeftParen()
    : EdifyKeywordToken(EdifyTokenType::LeftParen, "(")
{
}

////////////////////////////////////////////////////////////////////////////////

EdifyTokenRightParen::EdifyTokenRightParen()
    : EdifyKeywordToken(EdifyTokenType::RightParen, ")")
{
}

////////////////////////////////////////////////////////////////////////////////

EdifyTokenSemicolon::EdifyTokenSemicolon()
    : EdifyKeywordToken(EdifyTokenType::Semicolon, ";")
{
}

////////////////////////////////////////////////////////////////////////////////

EdifyTokenComma::EdifyTokenComma()
    : EdifyKeywordToken(EdifyTokenType::Comma, ",")
{
}

////////////////////////////////////////////////////////////////////////////////

EdifyTokenConcat::EdifyTokenConcat()
    : EdifyKeywordToken(EdifyTokenType::Concat, "+")
{
}

////////////////////////////////////////////////////////////////////////////////

EdifyTokenNewline::EdifyTokenNewline()
    : EdifyKeywordToken(EdifyTokenType::Newline, "\n")
{
}

////////////////////////////////////////////////////////////////////////////////

EdifyTokenWhitespace::EdifyTokenWhitespace(std::string str)
    : EdifyToken(EdifyTokenType::Whitespace)
{
    assert(!str.empty());

    for (char c MB_UNUSED : str) {
        // Should never fail
        assert(std::isspace(c));
    }

    m_str = std::move(str);
}

std::string EdifyTokenWhitespace::generate()
{
    return m_str;
}

////////////////////////////////////////////////////////////////////////////////

EdifyTokenComment::EdifyTokenComment(std::string str)
    : EdifyToken(EdifyTokenType::Comment), m_str(std::move(str))
{
}

std::string EdifyTokenComment::generate()
{
    return '#' + m_str;
}

////////////////////////////////////////////////////////////////////////////////

EdifyTokenString::EdifyTokenString(std::string str, Type type)
    : EdifyToken(EdifyTokenType::String), m_type(type)
{
    switch (type) {
    case AlreadyQuoted:
        m_quoted = true;
        assert(str.size() >= 2);
        assert(str.front() == '"');
        assert(str.back() == '"');
        m_str = std::move(str);
        break;
    case NotQuoted:
        m_quoted = false;
        m_str = std::move(str);
        break;
    case MakeQuoted:
        m_quoted = true;
        std::string temp;
        escape(str, &temp);
        temp.insert(temp.begin(), '"');
        temp.push_back('"');
        m_str = std::move(temp);
        break;
    }
}

std::string EdifyTokenString::generate()
{
    return m_str;
}

std::string EdifyTokenString::unescapedString()
{
    std::string out;
    // TODO: Check return value
    unescape(m_str, &out);
    if (m_quoted && out.size() >= 2) {
        out.pop_back();
        out.erase(out.begin());
    }
    return out;
}

std::string EdifyTokenString::string()
{
    return m_str;
}

void EdifyTokenString::escape(const std::string &str, std::string *out)
{
    static const char digits[] = "0123456789abcdef";

    std::string output;

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

    out->swap(output);
}

static int hexCharToInt(char c)
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

bool EdifyTokenString::unescape(const std::string &str, std::string *out)
{
    std::string output;

    for (std::size_t i = 0; i < str.size();) {
        char c = str[i];

        if (c == '\\') {
            if (i == str.size() - 1) {
                // Escape character is last character
                return false;
            }

            std::size_t new_i = i + 2;

            if (str[i + 1] == 'a') {
                output += '\a';
            } else if (str[i + 1] == 'b') {
                output += '\b';
            } else if (str[i + 1] == 'f') {
                output += '\f';
            } else if (str[i + 1] == 'n') {
                output += '\n';
            } else if (str[i + 1] == 'r') {
                output += '\r';
            } else if (str[i + 1] == 't') {
                output += '\t';
            } else if (str[i + 1] == 'v') {
                output += '\v';
            } else if (str[i + 1] == '\\') {
                output += '\\';
            } else if (str[i + 1] == 'x') {
                if (str.size() - i < 4) {
                    // Need 4 chars: \xYY
                    return false;
                }
                int digit1 = hexCharToInt(str[i + 2]);
                int digit2 = hexCharToInt(str[i + 3]);
                if (digit1 < 0 || digit2 < 0) {
                    // One of the chars is not a valid hex character
                    return false;
                }

                char val = (digit1 << 4) & digit2;
                output += val;

                new_i += 2;
            } else {
                // Invalid escape char
                return false;
            }

            i = new_i;
        } else {
            output += c;
            i += 1;
        }
    }

    out->swap(output);

    return true;
}

////////////////////////////////////////////////////////////////////////////////

EdifyTokenUnknown::EdifyTokenUnknown(char c) : EdifyToken(EdifyTokenType::Unknown), m_char(c)
{
}

std::string EdifyTokenUnknown::generate()
{
    return std::string(1, m_char);
}

////////////////////////////////////////////////////////////////////////////////

bool EdifyTokenizer::isValidUnquoted(char c)
{
    return std::isalnum(c)
            || c == '_'
            || c == ':'
            || c == '/'
            || c == '.';
}

bool EdifyTokenizer::nextToken(const char *data, std::size_t size,
                               std::size_t *pos, EdifyToken **token)
{
    std::size_t p = *pos;
    assert(p < size);

    if (size - p >= 2 && std::memcmp(data + p, "if", 2) == 0) {
        *token = new EdifyTokenIf();
        p += 2;
    } else if (size - p >= 4 && std::memcmp(data + p, "then", 4) == 0) {
        *token = new EdifyTokenThen();
        p += 4;
    } else if (size - p >= 4 && std::memcmp(data + p, "else", 4) == 0) {
        *token = new EdifyTokenElse();
        p += 4;
    } else if (size - p >= 5 && std::memcmp(data + p, "endif", 5) == 0) {
        *token = new EdifyTokenEndif();
        p += 5;
    } else if (size - p >= 2 && std::memcmp(data + p, "&&", 2) == 0) {
        *token = new EdifyTokenAnd();
        p += 2;
    } else if (size - p >= 2 && std::memcmp(data + p, "||", 2) == 0) {
        *token = new EdifyTokenOr();
        p += 2;
    } else if (size - p >= 2 && std::memcmp(data + p, "==", 2) == 0) {
        *token = new EdifyTokenEquals();
        p += 2;
    } else if (size - p >= 2 && std::memcmp(data + p, "!=", 2) == 0) {
        *token = new EdifyTokenNotEquals();
        p += 2;
    } else if (data[p] == '!') {
        *token = new EdifyTokenNot();
        p += 1;
    } else if (data[p] == '(') {
        *token = new EdifyTokenLeftParen();
        p += 1;
    } else if (data[p] == ')') {
        *token = new EdifyTokenRightParen();
        p += 1;
    } else if (data[p] == ';') {
        *token = new EdifyTokenSemicolon();
        p += 1;
    } else if (data[p] == ',') {
        *token = new EdifyTokenComma();
        p += 1;
    } else if (data[p] == '+') {
        *token = new EdifyTokenConcat();
        p += 1;
    } else if (data[p] == '\n') {
        *token = new EdifyTokenNewline();
        p += 1;
    } else if (data[p] != '\n' && std::isspace(data[p])) {
        std::string buf;
        buf += data[p];
        p += 1;
        while (size - p >= 1 && data[p] != '\n' && std::isspace(data[p])) {
            buf += data[p];
            p += 1;
        }
        *token = new EdifyTokenWhitespace(std::move(buf));
    } else if (data[p] == '#') {
        std::string buf;
        // Omit '#' character
        p += 1;
        while (size - p >= 1 && data[p] != '\n') {
            buf += data[p];
            p += 1;
        }
        *token = new EdifyTokenComment(std::move(buf));
    } else if (isValidUnquoted(data[p])) {
        std::string buf;
        buf += data[p];
        p += 1;
        while (size - p >= 1 && isValidUnquoted(data[p])) {
            buf += data[p];
            p += 1;
        }
        *token = new EdifyTokenString(std::move(buf), EdifyTokenString::NotQuoted);
    } else if (data[p] == '"') {
        std::size_t curPos = p;
        std::string buf;
        buf += data[p];
        p += 1;
        bool escaped = false;
        bool terminated = false;
        while (size - p >= 1) {
            if (data[p] == '\\' || escaped) {
                escaped = !escaped;
            } else if (!escaped && data[p] == '"') {
                buf += data[p];
                p += 1;
                terminated = true;
                break;
            }
            buf += data[p];
            p += 1;
        }
        if (!terminated) {
            LOGE("Unterminated quote at position %" MB_PRIzu, curPos);
            return false;
        }
        *token = new EdifyTokenString(std::move(buf), EdifyTokenString::AlreadyQuoted);
    } else {
        *token = new EdifyTokenUnknown(data[p]);
        p += 1;
    }

    *pos = p;

    return true;
}

bool EdifyTokenizer::tokenize(const char *data, std::size_t size,
                              std::vector<EdifyToken *> *tokens)
{
    std::vector<EdifyToken *> temp;
    EdifyToken *token;
    std::size_t pos = 0;
    bool fail = false;

    while (true) {
        if (pos > size) {
            LOGE("Tokenizer position exceeded data size!");
            fail = true;
            break;
        } else if (pos == size) {
            break;
        } else if (!nextToken(data, size, &pos, &token)) {
            fail = true;
            break;
        }
        temp.push_back(token);
    }

    if (fail) {
        for (EdifyToken *t : temp) {
            delete t;
        }
        temp.clear();
        return false;
    }

    tokens->swap(temp);
    return true;
}

std::string EdifyTokenizer::untokenize(const std::vector<EdifyToken *> &tokens)
{
    std::string output;
    for (EdifyToken *token : tokens) {
        output += token->generate();
    }
    return output;
}

std::string EdifyTokenizer::untokenize(const std::vector<EdifyToken *>::iterator &begin,
                                       const std::vector<EdifyToken *>::iterator &end)
{
    std::string output;
    for (auto it = begin; it != end; ++it) {
        output += (*it)->generate();
    }
    return output;
}

void EdifyTokenizer::dump(const std::vector<EdifyToken *> &tokens)
{
    const char *tokenName = nullptr;

    for (std::size_t i = 0; i < tokens.size(); ++i) {
        EdifyToken *t = tokens[i];

        switch (t->type()) {
        case EdifyTokenType::If:         tokenName = "If";         break;
        case EdifyTokenType::Then:       tokenName = "Then";       break;
        case EdifyTokenType::Else:       tokenName = "Else";       break;
        case EdifyTokenType::Endif:      tokenName = "Endif";      break;
        case EdifyTokenType::And:        tokenName = "And";        break;
        case EdifyTokenType::Or:         tokenName = "Or";         break;
        case EdifyTokenType::Equals:     tokenName = "Equals";     break;
        case EdifyTokenType::NotEquals:  tokenName = "NotEquals";  break;
        case EdifyTokenType::Not:        tokenName = "Not";        break;
        case EdifyTokenType::LeftParen:  tokenName = "LeftParen";  break;
        case EdifyTokenType::RightParen: tokenName = "RightParen"; break;
        case EdifyTokenType::Semicolon:  tokenName = "Semicolon";  break;
        case EdifyTokenType::Comma:      tokenName = "Comma";      break;
        case EdifyTokenType::Concat:     tokenName = "Concat";     break;
        case EdifyTokenType::Newline:    tokenName = "Newline";    break;
        case EdifyTokenType::Whitespace: tokenName = "Whitespace"; break;
        case EdifyTokenType::Comment:    tokenName = "Comment";    break;
        case EdifyTokenType::String:     tokenName = "String";     break;
        case EdifyTokenType::Unknown:    tokenName = "Unknown";    break;
        }

        LOGD("%" MB_PRIzu ": %-20s: %s", i, tokenName, t->generate().c_str());
    }
}

}
