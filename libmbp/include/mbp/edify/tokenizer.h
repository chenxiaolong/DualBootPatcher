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

#pragma once

#include <string>
#include <vector>

namespace mbp
{

enum class EdifyTokenType
{
    // Keyword tokens
    If,
    Then,
    Else,
    Endif,
    And,
    Or,
    Equals,
    NotEquals,
    Not,
    LeftParen,
    RightParen,
    Semicolon,
    Comma,
    Concat,
    Newline,
    // String-based tokens
    Whitespace,
    Comment,
    String,
    // Unknown
    Unknown
};

class EdifyToken
{
public:
    virtual ~EdifyToken();

    EdifyTokenType type() const;
    virtual std::string generate() = 0;

protected:
    EdifyToken(EdifyTokenType type);

private:
    EdifyTokenType m_type;
};

////////////////////////////////////////////////////////////////////////////////

// Keyword token base class
class EdifyKeywordToken : public EdifyToken
{
public:
    virtual std::string generate() override;

protected:
    EdifyKeywordToken(EdifyTokenType type, std::string keyword);

private:
    std::string m_keyword;
};

////////////////////////////////////////////////////////////////////////////////

class EdifyTokenIf : public EdifyKeywordToken
{
public:
    EdifyTokenIf();
};

////////////////////////////////////////////////////////////////////////////////

class EdifyTokenThen : public EdifyKeywordToken
{
public:
    EdifyTokenThen();
};

////////////////////////////////////////////////////////////////////////////////

class EdifyTokenElse : public EdifyKeywordToken
{
public:
    EdifyTokenElse();
};

////////////////////////////////////////////////////////////////////////////////

class EdifyTokenEndif : public EdifyKeywordToken
{
public:
    EdifyTokenEndif();
};

////////////////////////////////////////////////////////////////////////////////

class EdifyTokenAnd : public EdifyKeywordToken
{
public:
    EdifyTokenAnd();
};

////////////////////////////////////////////////////////////////////////////////

class EdifyTokenOr : public EdifyKeywordToken
{
public:
    EdifyTokenOr();
};

////////////////////////////////////////////////////////////////////////////////

class EdifyTokenEquals : public EdifyKeywordToken
{
public:
    EdifyTokenEquals();
};

////////////////////////////////////////////////////////////////////////////////

class EdifyTokenNotEquals : public EdifyKeywordToken
{
public:
    EdifyTokenNotEquals();
};

////////////////////////////////////////////////////////////////////////////////

class EdifyTokenNot : public EdifyKeywordToken
{
public:
    EdifyTokenNot();
};

////////////////////////////////////////////////////////////////////////////////

class EdifyTokenLeftParen : public EdifyKeywordToken
{
public:
    EdifyTokenLeftParen();
};

////////////////////////////////////////////////////////////////////////////////

class EdifyTokenRightParen : public EdifyKeywordToken
{
public:
    EdifyTokenRightParen();
};

////////////////////////////////////////////////////////////////////////////////

class EdifyTokenSemicolon : public EdifyKeywordToken
{
public:
    EdifyTokenSemicolon();
};

////////////////////////////////////////////////////////////////////////////////

class EdifyTokenComma : public EdifyKeywordToken
{
public:
    EdifyTokenComma();
};

////////////////////////////////////////////////////////////////////////////////

class EdifyTokenConcat : public EdifyKeywordToken
{
public:
    EdifyTokenConcat();
};

////////////////////////////////////////////////////////////////////////////////

class EdifyTokenNewline : public EdifyKeywordToken
{
public:
    EdifyTokenNewline();
};

////////////////////////////////////////////////////////////////////////////////

class EdifyTokenWhitespace : public EdifyToken
{
public:
    EdifyTokenWhitespace(std::string str);
    virtual std::string generate() override;

private:
    std::string m_str;
};

////////////////////////////////////////////////////////////////////////////////

class EdifyTokenComment : public EdifyToken
{
public:
    EdifyTokenComment(std::string str);
    virtual std::string generate() override;

private:
    std::string m_str;
};

////////////////////////////////////////////////////////////////////////////////

class EdifyTokenString : public EdifyToken
{
public:
    enum Type {
        NotQuoted,
        MakeQuoted,
        AlreadyQuoted
    };

    EdifyTokenString(std::string str, Type type);
    virtual std::string generate() override;

    std::string unescapedString();
    std::string string();

protected:
    std::string m_str;
    Type m_type;
    bool m_quoted;

    static void escape(const std::string &str, std::string *out);
    static bool unescape(const std::string &str, std::string *out);
};

////////////////////////////////////////////////////////////////////////////////

class EdifyTokenUnknown : public EdifyToken
{
public:
    EdifyTokenUnknown(char c);
    virtual std::string generate() override;

private:
    char m_char;
};

////////////////////////////////////////////////////////////////////////////////

class EdifyTokenizer
{
public:
    static bool tokenize(const char *data, std::size_t size,
                         std::vector<EdifyToken *> *tokens);
    static std::string untokenize(const std::vector<EdifyToken *> &tokens);
    static std::string untokenize(const std::vector<EdifyToken *>::iterator &begin,
                                  const std::vector<EdifyToken *>::iterator &end);

    static void dump(const std::vector<EdifyToken *> &tokens);

private:
    static bool isValidUnquoted(char c);

    static bool nextToken(const char *data, std::size_t size, std::size_t *pos,
                          EdifyToken **token);

    EdifyTokenizer() = delete;
    EdifyTokenizer(const EdifyTokenizer &) = delete;
    EdifyTokenizer(EdifyTokenizer &&) = delete;
};

}
