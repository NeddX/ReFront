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

        enum class StatementKind : u8
        {
            VariableDecleration,
            FunctionDeclaration,
            FunctionParameter,
            FunctionParemeterList,
            FundamentalType,
            Initializer,
            Expression,
            FunctionCallExpression,
            ArgumentListExpression,
            Block
        };

        struct Type
        {
        public:
            std::string       name{};
            FundamentalType   ftype{};
            std::vector<Type> fields{}; // For user defined types.

        public:
            static std::optional<Type> FromToken(const Token& token) noexcept;
        };

        struct Statement
        {
            std::string            name{};
            StatementKind          kind{};
            std::vector<Statement> children{};
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
        std::vector<ast::Statement> Parse();

    private:
        std::optional<ast::Statement> ExpectFunctionDecl();
        ast::Statement                ExpectFunctionParameterList();
        std::vector<ast::Statement>   ParseFunctionBody();
        std::optional<ast::Statement> ExpectLocalFunctionStatement();
        std::optional<Token>          Consume() noexcept;
    };
} // namespace cmm::cmc

#endif // CMC_ANALYZER_PARSER_H
