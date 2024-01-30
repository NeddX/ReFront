#ifndef CMC_ANALYZER_PARSER_H
#define CMC_ANALYZER_PARSER_H

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

        enum class Operation : u8
        {
            LocalFunction,
            VariableDecleration
        };

        enum class StatementType : u8
        {
            LocalFunctionStatement,
            FundamentalType,
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
            Operation              op{};
            std::vector<Statement> params{};
            Token                  token{};
        };

        struct ParameterDefinition
        {
            std::string name{};
            Type        type{};
        };

        struct FunctionDeclaration
        {
            std::string                      name{};
            Type                             type{};
            std::vector<ParameterDefinition> params{};
            std::vector<Statement>           statement{};
        };
    } // namespace ast

    class Parser
    {
    private:
        std::string_view m_Source{};
        Lexer            m_Lexer{};

    public:
        explicit Parser(const std::string_view source);

    public:
    };
} // namespace cmm::cmc

#endif // CMC_ANALYZER_PARSER_H
