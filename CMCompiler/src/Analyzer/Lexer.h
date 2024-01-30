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
        KeywordFn,
        KeywordImport,
        KeywordIf,
        KeywordElse,
        KeywordElseIf,
        KeywordInt,
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
        TokenType type = TokenType::None;
        TextSpan  span{};
        i64       num{};
    };

    using TokenList = std::vector<Token>;

    class Lexer
    {
    private:
        std::string_view m_Source{};
        usize            m_CurrentPos{};

    public:
        Lexer(const std::string_view source);

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
