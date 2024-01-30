#ifndef CMC_ANALYZER_LEXER_H
#define CMC_ANALYZER_LEXER_H

#include <iostream>
#include <optional>
#include <string_view>
#include <vector>

#include <CommonDef.h>

namespace cmm::cmc {
    enum class TokenType
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
        bool IsValid() const noexcept { return type == TokenType::None; }
        bool IsOperator() const noexcept
        {
            switch (type)
            {
                case TokenType::Colon:
                case TokenType::SemiColon:
                case TokenType::Equals:
                case TokenType::LeftBrace:
                case TokenType::RightBrace:
                case TokenType::LeftCurlyBracket:
                case TokenType::RightCurlyBracket:
                case TokenType::Plus:
                case TokenType::Minus:
                case TokenType::Asterisk:
                case TokenType::ForwardSlash:
                case TokenType::LeftAngleBracket:
                case TokenType::RightAngleBracket:
                case TokenType::LeftSquareBracket:
                case TokenType::RightSquareBracket:
                case TokenType::DoubleQuote:
                case TokenType::Quote:
                case TokenType::Comma:
                case TokenType::Exclamation: return true;
                default: break;
            }
            return false;
        }
        bool IsKeyword() const noexcept
        {
            switch (type)
            {
                case TokenType::KeywordLet:
                case TokenType::KeywordFn:
                case TokenType::KeywordImport:
                case TokenType::KeywordIf:
                case TokenType::KeywordElse:
                case TokenType::KeywordElseIf:
                case TokenType::KeywordI32:
                case TokenType::KeywordI64:
                case TokenType::KeywordString:
                case TokenType::KeywordBool:
                case TokenType::KeywordChar:
                case TokenType::KeywordWhile:
                case TokenType::KeywordReturn:
                case TokenType::KeywordTrue:
                case TokenType::KeywordFalse: return true;
                default: break;
            }
            return false;
        }
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
