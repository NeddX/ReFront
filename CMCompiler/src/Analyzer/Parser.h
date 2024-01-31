#ifndef CMC_ANALYZER_PARSER_H
#define CMC_ANALYZER_PARSER_H

#include <nlohmann/json.hpp>
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
            InitializerList,
            FunctionCallExpression,
            ArgumentListExpression,

            If,
            ElseIf,
            Else,
            While,
            Return,
            Block,

            IdentifierName,
            LiteralExpression
        };

        struct Type
        {
        public:
            std::string       name{};
            FundamentalType   ftype{};
            std::vector<Type> fields{}; // For user defined types.

        public:
            static Type Integer32;
            static Type Integer64;
            static Type String;
            static Type Character;
            static Type Boolean;

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
        std::optional<ast::Statement> ExpectVariableDeclaration();
        std::optional<ast::Statement> ExpectKeyword();
        std::optional<ast::Statement> ExpectExpression();
        std::optional<ast::Statement> ExpectLiteral();
        std::optional<ast::Statement> ExpectIdentifierName();
        std::optional<Token>          Consume() noexcept;
    };
} // namespace cmm::cmc

namespace nlohmann {
    template <>
    struct adl_serializer<cmm::cmc::ast::StatementKind>
    {
        static void to_json(ordered_json& j, const cmm::cmc::ast::StatementKind& e)
        {
            switch (e)
            {
                using enum cmm::cmc::ast::StatementKind;

                case VariableDecleration: j = "VariableDeclaration"; break;
                case FunctionDeclaration: j = "FunctionDeclaration"; break;
                case FunctionParameter: j = "FunctionParameter"; break;
                case FunctionParemeterList: j = "FunctionParameterList"; break;
                case InitializerList: j = "InitializerList"; break;
                case FunctionCallExpression: j = "FunctionCallExpression"; break;
                case ArgumentListExpression: j = "ArgumentListExpression"; break;
                case If: j = "IfStatement"; break;
                case ElseIf: j = "ElseIfStatement"; break;
                case Else: j = "ElseStatement"; break;
                case While: j = "WhileStatement"; break;
                case Return: j = "ReturnStatement"; break;
                case Block: j = "Block"; break;
                case IdentifierName: j = "IdentifierName"; break;
                case LiteralExpression: j = "LiteralExpression"; break;
                default: j = "Unknown"; break;
            }
        }

        // Do not need a from_json() for now.
    };

    template <>
    struct adl_serializer<cmm::cmc::ast::FundamentalType>
    {
        static void to_json(ordered_json& j, const cmm::cmc::ast::FundamentalType& e)
        {
            switch (e)
            {
                using enum cmm::cmc::ast::FundamentalType;

                case Void: j = "Void"; break;
                case Integer32: j = "Integer32"; break;
                case Integer64: j = "Integer64"; break;
                case Boolean: j = "Boolean"; break;
                case Character: j = "Character"; break;
                case String: j = "String"; break;
                case UserDefined: j = "UserDefined"; break;
                default: j = "Unknown"; break;
            }
        }
    };

    template <>
    struct adl_serializer<cmm::cmc::ast::Type>
    {
        static void to_json(ordered_json& j, const cmm::cmc::ast::Type& type)
        {
            j["name"]   = type.name;
            j["ftype"]  = type.ftype;
            j["fields"] = type.fields;
        }
    };

    template <>
    struct adl_serializer<cmm::cmc::ast::Statement>
    {
        static void to_json(ordered_json& j, const cmm::cmc::ast::Statement& s)
        {
            j["name"]     = s.name;
            j["kind"]     = s.kind;
            j["children"] = s.children;
            j["type"]     = s.type;
            j["token"]    = s.token;
        }
    };

} // namespace nlohmann

#endif // CMC_ANALYZER_PARSER_H
