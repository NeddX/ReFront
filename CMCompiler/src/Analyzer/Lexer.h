#ifndef CMC_ANALYZER_LEXER_H
#define CMC_ANALYZER_LEXER_H

#include <iostream>
#include <optional>
#include <string_view>
#include <vector>

#include <CommonDef.h>

namespace cmm::cmc {
    enum class TokenType : u32
    {
        None,

        // Identifier
        Identifier,

        // Literals
        NumberLiteral,
        StringLiteral,
        CharacterLiteral,

        // Operators
        Colon,
        SemiColon,
        Equals,
        LeftBrace,
        RightBrace,
        LeftCurlyBracket,
        RightCurlyBracket,
        Plus,
        Minus,
        Asterisk,
        ForwardSlash,
        LeftAngleBracket,
        RightAngleBracket,
        LeftSquareBracket,
        RightSquareBracket,
        DoubleQuote,
        Quote,
        Comma,
        Exclamation,

        // Keywords
        KeywordLet,
        KeywordFn,
        KeywordImport,
        KeywordIf,
        KeywordElse,
        KeywordElseIf,
        KeywordI32,
        KeywordI64,
        KeywordString,
        KeywordBool,
        KeywordChar,
        KeywordWhile,
        KeywordReturn,
        KeywordTrue,
        KeywordFalse,

        // Eof
        Eof
    };

    std::string_view TokenTypeToString(const TokenType type) noexcept;

    struct TextSpan
    {
        usize       line{};
        usize       cur{};
        std::string text{};
    };

    struct Token
    {
    public:
        TokenType type = TokenType::None;
        TextSpan  span{};
        i64       num{};

    public:
        // Few handy methods to make parsing easier.
        bool IsValid() const noexcept { return type != TokenType::None && type != TokenType::Eof; }
        bool IsOperator() const noexcept
        {
            switch (type)
            {
                using enum TokenType;

                case Colon:
                case SemiColon:
                case Equals:
                case LeftBrace:
                case RightBrace:
                case LeftCurlyBracket:
                case RightCurlyBracket:
                case Plus:
                case Minus:
                case Asterisk:
                case ForwardSlash:
                case LeftAngleBracket:
                case RightAngleBracket:
                case LeftSquareBracket:
                case RightSquareBracket:
                case DoubleQuote:
                case Quote:
                case Comma:
                case Exclamation: return true;
                default: break;
            }
            return false;
        }
        bool IsKeyword() const noexcept
        {
            switch (type)
            {
                using enum TokenType;

                case KeywordLet:
                case KeywordFn:
                case KeywordImport:
                case KeywordIf:
                case KeywordElse:
                case KeywordElseIf:
                case KeywordI32:
                case KeywordI64:
                case KeywordString:
                case KeywordBool:
                case KeywordChar:
                case KeywordWhile:
                case KeywordReturn:
                case KeywordTrue:
                case KeywordFalse: return true;
                default: break;
            }
            return false;
        }
        std::string_view ToString() const noexcept { return TokenTypeToString(type); }
    };

    using TokenList = std::vector<Token>;

    class Lexer
    {
    private:
        std::string_view m_Source{};
        usize            m_CurrentPos{};

    public:
        Lexer() = default;
        explicit Lexer(const std::string_view source);

    public:
        std::optional<Token> NextToken();

    private:
        std::optional<char>        CurrentChar() const noexcept;
        std::optional<char>        Consume() noexcept;
        i64                        ConsumeNumber() noexcept;
        std::string                ConsumeIdentifier() noexcept;
        TokenType                  ConsumeOperator() noexcept;
        std::optional<std::string> ConsumeString() noexcept;
        bool                       IsIdentifierStart(const char c) const noexcept;
    };

} // namespace cmm::cmc

std::ostream& operator<<(std::ostream& stream, const cmm::cmc::TextSpan& span) noexcept;
std::ostream& operator<<(std::ostream& stream, const cmm::cmc::Token& token) noexcept;

#endif // CMC_ANALYZER_LEXER_H
