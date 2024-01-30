#ifndef CMC_ANALYZER_PARSER_H
#define CMC_ANALYZER_PARSER_H

#include <format>
#include <unordered_map>
#include <vector>

#include "Lexer.h"

namespace cmm::cmc {
    namespace ast {
        enum class FundamentalType : u8
        {
            Void,
            Integer32,
            Integer64,
            Boolean,
            Character,
            String,
            UserDefined
        };

        enum class StatementType : u8
        {
            VariableDecleration,
            FunctionDeclaration,
            FunctionParameter,
            FundamentalType,
            Initializer,
            Expression,
            FunctionCallExpression,
            ArgumentListExpression,
            Block
        };

        struct Type
        {
            std::string       name{};
            FundamentalType   ftype{};
            std::vector<Type> fields{}; // For user defined types.
        };

        struct Statement
        {
            std::string            name{};
            StatementType          kind{};
            std::vector<Statement> params{};
            Type                   type{};
            Token                  token{};
        };
    } // namespace ast

    class Parser
    {
    private:
        std::string_view            m_Source{};
        Lexer                       m_Lexer{};
        std::optional<Token>        m_CurrentToken{};
        std::vector<ast::Statement> m_GlobalStatements{};

    public:
        explicit Parser(const std::string_view source) noexcept;

    public:
        std::vector<ast::Statement> Parse() noexcept;

    private:
        std::optional<ast::Statement> ExpectFunctionDecl() noexcept;
        std::optional<Token>          ConsumeToken() noexcept;
    };
} // namespace cmm::cmc

#endif // CMC_ANALYZER_PARSER_H
