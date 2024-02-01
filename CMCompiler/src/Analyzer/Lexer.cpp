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
                                     "NumberLiteral", "StringLiteral", "CharacterLiteral",

                                     // Operators
                                     "Colon", "SemiColon", "Equals", "LeftBrace", "RightBrace", "LeftCurlyBracket",
                                     "RightCurlyBracket", "Plus", "Minus", "Asterisk", "ForwardSlash",
                                     "LeftAngleBracket", "RightAngleBracket", "LeftSquareBracket", "RightSquareBracket",
                                     "DoubleQuote", "Quote", "Comma", "Exclamation",

                                     // Keywords
                                     "KeywordLet", "KeywordFn", "KeywordImport", "KeywordIf", "KeywordElse",
                                     "KeywordElseIf", "KeywordI32", "KeywordI64", "KeywordString", "KeywordBool",
                                     "KeywordChar", "KeywordWhile", "KeywordReturn", "KeywordTrue", "KeywordFalse",

                                     // Eof
                                     "Eof" };
        return str[(usize)type];
    }

    Lexer::Lexer(const std::string_view source) : m_Source(source)
    {
    }

    std::optional<Token> Lexer::PeekToken()
    {
        const auto current_state = *this;
        auto       token         = NextToken();
        *this                    = std::move(current_state);
        return token;
    }

    std::optional<Token> Lexer::NextToken()
    {
        // If we've reached the end.
        if (m_CurrentPos == m_Source.size())
        {
            ++m_CurrentPos;
            return Token{ .type = TokenType::Eof };
        }
        else if (m_CurrentPos > m_Source.size())
            // Now we're past the end.
            return std::nullopt;

        auto  c             = CurrentChar();
        usize start         = m_CurrentPos;
        auto  current_token = Token{};

        // Skip whitespace and friends
        while (c.has_value() && (*c == ' ' || *c == '\t' || *c == '\n' || *c == '\r'))
        {
            // Consume the whitespace.
            Consume();

            if (*c == '\n')
                // If it's a line then increment the line count.
                ++m_LineCount;

            // Progress to the next character and update start.
            c     = CurrentChar();
            start = m_CurrentPos;
        }

        // If our current character is valid.
        if (c.has_value())
        {
            // If it's a number start.
            if (std::isdigit(*c))
            {
                // Consume the number and set the type.
                current_token.num  = ConsumeNumber();
                current_token.type = TokenType::NumberLiteral;
            }
            else if (IsIdentifierStart(*c)) // Else if it's a possible identifier.
            {
                auto ident = ConsumeIdentifier();

                // Check if it's a keyword.
                if (ident == "let")
                    current_token.type = TokenType::KeywordLet;
                else if (ident == "fn")
                    current_token.type = TokenType::KeywordFn;
                else if (ident == "import")
                    current_token.type = TokenType::KeywordImport;
                else if (ident == "if")
                    current_token.type = TokenType::KeywordIf;
                else if (ident == "else")
                    current_token.type = TokenType::KeywordElse;
                else if (ident == "while")
                    current_token.type = TokenType::KeywordWhile;
                else if (ident == "i32")
                    current_token.type = TokenType::KeywordI32;
                else if (ident == "i64")
                    current_token.type = TokenType::KeywordI64;
                else if (ident == "string")
                    current_token.type = TokenType::KeywordString;
                else if (ident == "bool")
                    current_token.type = TokenType::KeywordBool;
                else if (ident == "char")
                    current_token.type = TokenType::KeywordChar;
                else if (ident == "return")
                    current_token.type = TokenType::KeywordReturn;
                else if (ident == "false")
                    current_token.type = TokenType::KeywordFalse;
                else if (ident == "true")
                    current_token.type = TokenType::KeywordTrue;
                else
                    current_token.type = TokenType::Identifier;
            }
            else // Else it must be an operator.
            {
                current_token.type = ConsumeOperator();

                // Possible string or character literal.
                switch (current_token.type)
                {
                    case TokenType::DoubleQuote: {
                        // Consume the string.
                        auto str = ConsumeString();

                        // This is always going to be true but whatever I guess.
                        if (str.has_value())
                        {
                            // Since we know it's a string we can just handle it right away and move str into our token
                            // span.
                            ++m_TokenCount;
                            current_token.type = TokenType::StringLiteral;
                            current_token.span =
                                TextSpan{ .line = m_LineCount, .cur = m_TokenCount, .text = std::move(*str) };

                            return current_token;
                        }
                        break;
                    }
                    case TokenType::Quote: {
                        // Consume the possible character inside the single quotes.
                        auto c = Consume();
                        if (c.has_value())
                        {
                            // Consume the possible second quote.
                            auto ec = Consume();
                            if (ec == '\'')
                            {
                                // Since we know it's a character we can just handle it right away.
                                ++m_TokenCount;
                                current_token.type = TokenType::CharacterLiteral;
                                current_token.num  = *ec;
                                current_token.span = TextSpan{ .line = m_LineCount, .cur = m_TokenCount };
                                current_token.span.text += *c;

                                return current_token;
                            }
                            else
                                throw std::runtime_error("Compile Error: Expected a closing single quote.");
                        }
                        break;
                    }
                    default: break;
                }
            }
        }
        else
        {
            // If our current character is invalid then we've reached the end.
            ++m_CurrentPos;
            return Token{ .type = TokenType::Eof };
        }

        // Increment the token count and create our text span which will hold the line number, cursor number and the
        // text itself.
        ++m_TokenCount;
        usize end          = m_CurrentPos;
        current_token.span = TextSpan{ .line = m_LineCount,
                                       .cur  = m_TokenCount,
                                       .text = std::string{ m_Source.substr(start, end - start) } };

        return current_token;
    }

    std::optional<char> Lexer::CurrentChar() const noexcept
    {
        // Bounds checking.
        if (m_Source.size() > m_CurrentPos)
            return m_Source[m_CurrentPos];
        else
            return std::nullopt;
    }

    std::optional<char> Lexer::Consume() noexcept
    {
        // Progress to the next character and return the previous or nothing if we've reached the end.
        if (m_CurrentPos >= m_Source.size())
            return std::nullopt;
        auto c = CurrentChar();
        ++m_CurrentPos;
        return c;
    }

    i64 Lexer::ConsumeNumber() noexcept
    {
        // Consume the number digit by digit until we hit a non-digit character.
        i64 num = 0;
        for (auto c = CurrentChar(); c.has_value(); c = CurrentChar())
        {
            if (std::isdigit(*c))
            {
                // If c was valid then Consume the current character then store c into num.
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
            if ((ident.empty() && std::isalpha(*c)) || (!ident.empty() && std::isalnum(*c)) || *c == '_')
            {
                // If our identifier starts with a letter or contains a possible letter or a number, or it's just an
                // underscore then Consume the current character, append the previous to ident, then continue.
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
        // We know that this is guaranteed to be an operator so don't even bother with null checking and just unwrap
        // straight away.
        auto c = *Consume();
        switch (c)
        {
            using enum TokenType;

            case ':': return Colon;
            case ';': return SemiColon;
            case '+': return Plus;
            case '-': return Minus;
            case '*': return Asterisk;
            case '/': return ForwardSlash;
            case '=': return Equals;
            case '(': return LeftBrace;
            case ')': return RightBrace;
            case '{': return LeftCurlyBrace;
            case '}': return RightCurlyBrace;
            case '<': return LeftAngleBracket;
            case '>': return RightAngleBracket;
            case '[': return LeftSquareBracket;
            case ']': return RightSquareBracket;
            case '"': return DoubleQuote;
            case '\'': return Quote;
            case ',': return Comma;
            case '!': return Exclamation;
            default: break;
        }
        return TokenType::None;
    }

    std::optional<std::string> Lexer::ConsumeString() noexcept
    {
        std::string str{};
        for (auto c = CurrentChar(); c.has_value(); c = CurrentChar())
        {
            // Consume the string until we hit a double quote.
            Consume();
            if (*c != '"')
                str += *c;
            else
                break;
        }

        // If the string was invalid then return nothing, or str otherwise.
        if (str.empty())
            return std::nullopt;
        else
            return str;
    }

    bool Lexer::IsIdentifierStart(const char c) const noexcept
    {
        // Identifiers start with a letter or an underscore followed by more letters, underscores and numbers.
        return (std::isalpha(c) || c == '_');
    }
} // namespace cmm::cmc

std::ostream& operator<<(std::ostream& stream, const cmm::cmc::TextSpan& span) noexcept
{
    stream << "{ Text: '" << span.text << "', Line: " << span.line << ", Cursor: " << span.cur << " }";
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const cmm::cmc::Token& token) noexcept
{
    stream << "{ Type: " << TokenTypeToString(token.type) << ", NumberLiteral: " << token.num
           << ", Span: " << token.span << " }";
    return stream;
}
