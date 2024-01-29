#include "Lexer.h"

#include <cctype>
#include <cstdlib>

namespace cmm::cmc {
    std::string_view TokenTypeToString(const TokenType type) noexcept
    {
        static const char* str[] = { "None",

                                     // Identifier
                                     "Identifier",

                                     // Literals
                                     "NumberLiteral", "StringLiteral",

                                     // Operators
                                     "SemiColon", "Equals", "LeftBrace", "RightBrace", "LeftCurlyBracket",
                                     "RightCurlyBracket", "Plus", "Minus", "Asterisk", "ForwardSlash",

                                     // Keywords
                                     "KeywordIf", "KeywordElse", "KeywordElseIf", "KeywordInt", "KeywordString",
                                     "KeywordWhile", "KeywordReturn", "KeywordTrue", "KeywordFalse",

                                     // Eof
                                     "Eof" };
        return str[(usize)type];
    }

    Lexer::Lexer(const std::string_view source) : m_Source(source)
    {
    }

    std::optional<Token> Lexer::NextToken() noexcept
    {
        static usize token_count{};
        static usize line_count{};

        if (m_CurrentPos == m_Source.size())
        {
            ++m_CurrentPos;
            return Token{ .type = TokenType::Eof };
        }

        auto  c             = CurrentChar();
        usize start         = m_CurrentPos;
        auto  current_token = Token{};

        if (c.has_value())
        {
            if (std::isdigit(*c))
            {
                current_token.num  = ConsumeNumber();
                current_token.type = TokenType::NumberLiteral;
            }
            else if (*c == ' ' || *c == '\t')
            {
                // Skip whitespaces n stuff...
                Consume();
            }
            else if (*c == '\n')
            {
                ++line_count;
            }
            else if (std::isalpha(*c))
            {
                auto ident = ConsumeIdentifier();

                // Check if it's a keyword.
                if (ident == "if")
                    current_token.type = TokenType::KeywordIf;
                else if (ident == "else")
                    current_token.type = TokenType::KeywordElse;
                else if (ident == "int")
                    current_token.type = TokenType::KeywordInt;
                else if (ident == "string")
                    current_token.type = TokenType::KeywordString;
                else
                    current_token.type = TokenType::Identifier;
            }
            else
                current_token.type = ConsumeOperator();
        }

        ++token_count;
        usize end = m_CurrentPos;
        current_token.span =
            TextSpan{ .line = line_count, .cur = token_count, .text = std::string{ m_Source.substr(start, end) } };

        return current_token;
    }

    std::optional<char> Lexer::CurrentChar() const noexcept
    {
        if (m_Source.size() > m_CurrentPos)
            return m_Source[m_CurrentPos];
        else
            return std::nullopt;
    }

    std::optional<char> Lexer::Consume() noexcept
    {
        if (m_CurrentPos >= m_Source.size())
            return std::nullopt;
        auto c = CurrentChar();
        ++m_CurrentPos;
        return c;
    }

    i64 Lexer::ConsumeNumber() noexcept
    {
        i64 num = 0;
        for (auto c = CurrentChar(); c.has_value(); c = CurrentChar())
        {
            if (std::isdigit(*c))
            {
                Consume();
                num = num * 10 + (*c - '0');
            }
            else
                break;
        }
        return num;
    }

    std::string Lexer::ConsumeIdentifier() noexcept
    {
        std::string ident{};
        for (auto c = CurrentChar(); c.has_value(); c = CurrentChar())
        {
            if (std::isalpha(*c))
            {
                Consume();
                ident += *c;
            }
            else
                break;
        }
        return ident;
    }

    TokenType Lexer::ConsumeOperator() noexcept
    {
        auto c = *CurrentChar();
        switch (c)
        {
            case ';': return TokenType::SemiColon;
            case '+': return TokenType::Plus;
            case '-': return TokenType::Minus;
            case '*': return TokenType::Asterisk;
            case '/': return TokenType::ForwardSlash;
            case '=': return TokenType::Equals;
            case '(': return TokenType::LeftBrace;
            case ')': return TokenType::RightBrace;
            case '{': return TokenType::LeftCurlyBracket;
            case '}': return TokenType::RightCurlyBracket;
            default: break;
        }
        return TokenType::None;
    }
} // namespace cmm::cmc

std::ostream& operator<<(std::ostream& stream, const cmm::cmc::TextSpan& span) noexcept
{
    stream << "{ Text: " << span.text << " Line: " << span.line << " Cursor: " << span.cur;
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const cmm::cmc::Token& token) noexcept
{
    stream << "{ Type: " << TokenTypeToString(token.type) << " NumberLiteral: " << token.num << " Span: " << token.span;
    return stream;
}
