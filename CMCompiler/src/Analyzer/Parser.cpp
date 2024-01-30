#include "Parser.h"

namespace cmm::cmc {
    using namespace ast;

    constexpr void CompileError(const Token& token, const char* msg)
    {
        std::cerr << std::format("Compile Error @ line ({}, {}): {}", token.span.line, token.span.cur, msg)
                  << std::endl;
    }

    Parser::Parser(const std::string_view source) noexcept : m_Source(source)
    {
    }

    std::vector<Statement> Parser::Parse() noexcept
    {
        m_Lexer = Lexer(m_Source);
        for (m_CurrentToken = m_Lexer.NextToken(); m_CurrentToken.has_value(); m_CurrentToken = m_Lexer.NextToken())
        {
            if (auto c = ExpectFunctionDecl(); c.has_value())
                m_GlobalStatements.push_back(std::move(*c));
        }
        return m_GlobalStatements;
    }

    std::optional<Token> Parser::ConsumeToken() noexcept
    {
        auto current   = m_CurrentToken;
        m_CurrentToken = m_Lexer.NextToken();
        return current;
    }

    std::optional<Statement> Parser::ExpectFunctionDecl() noexcept
    {
        if (m_CurrentToken.value().type == TokenType::KeywordFn)
        {
            auto prev_token = ConsumeToken();
            if (m_CurrentToken.has_value()) {}
            else
            {
                // Then nothing followed the fn keyword so it must be a compile error.
                CompileError(*prev_token, "Missing an identifier after the fn keyword.");
            }
        }
        return std::nullopt;
    }
} // namespace cmm::cmc
