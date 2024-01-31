#include "Parser.h"

#include <fmt/core.h>

// TODO: Improve and instead use exceptions.
#define CompileError(token, ...)                                                                                       \
    std::cerr << fmt::format("Compile Error @ line ({}, {}): ", (token).span.line, (token).span.cur)                   \
              << fmt::format(__VA_ARGS__) << std::endl;                                                                \
    std::exit(-1);

namespace cmm::cmc {
    using namespace ast;

    Type Type::Integer32 = Type{ .name = "Integer32", .ftype = FundamentalType::Integer32 };
    Type Type::Integer64 = Type{ .name = "Integer64", .ftype = FundamentalType::Integer64 };
    Type Type::String    = Type{ .name = "CString", .ftype = FundamentalType::String };
    Type Type::Character = Type{ .name = "Character8", .ftype = FundamentalType::Character };
    Type Type::Boolean   = Type{ .name = "Boolean", .ftype = FundamentalType::Boolean };

    std::optional<Type> Type::FromToken(const Token& token) noexcept
    {
        switch (token.type)
        {
            using enum TokenType;

            case KeywordI32: return Type{ .name = "i32", .ftype = FundamentalType::Integer32 };
            case KeywordI64: return Type{ .name = "i64", .ftype = FundamentalType::Integer64 };
            case KeywordString: return Type{ .name = "string", .ftype = FundamentalType::String };
            case Identifier: return Type{ .name = token.span.text, .ftype = FundamentalType::UserDefined };
            default: break;
        }
        return std::nullopt;
    }

    Parser::Parser(const std::string_view source) noexcept : m_Source(source)
    {
    }

    std::vector<Statement> Parser::Parse()
    {
        m_Lexer = Lexer(m_Source);
        for (m_CurrentToken = m_Lexer.NextToken();
             m_CurrentToken.has_value() && m_CurrentToken.value().type != TokenType::Eof;
             m_CurrentToken = m_Lexer.NextToken())
        {
            if (auto c = ExpectFunctionDecl(); c.has_value())
                m_GlobalStatements.push_back(std::move(*c));
        }
        return m_GlobalStatements;
    }

    std::optional<Token> Parser::Consume() noexcept
    {
        auto current   = m_CurrentToken;
        m_CurrentToken = m_Lexer.NextToken();
        return current;
    }

    std::optional<Statement> Parser::ExpectFunctionDecl()
    {
        if (m_CurrentToken.value().type == TokenType::KeywordFn)
        {
            // Consume the Fn keyword.
            auto prev_token = *Consume();

            // Our function declaration statement.
            Statement func_stmt{};

            // If the following token is an identifier.
            if (m_CurrentToken.value().type == TokenType::Identifier)
            {
                // Consume the identifier.
                prev_token      = *Consume();
                func_stmt.kind  = StatementKind::FunctionDeclaration;
                func_stmt.token = std::move(prev_token);

                // Parse possible parameter list, if there's none then our parameter list statement will just be empty.
                auto param_list = ExpectFunctionParameterList();
                func_stmt.children.push_back(std::move(param_list));

                // Parse the possible return type or a function scope start.
                if (m_CurrentToken.value().IsValid())
                {
                    // Parse the possible arrow return type specifier.
                    if (m_CurrentToken.value().type == TokenType::Minus)
                    {
                        // Consume the dash.
                        Consume();

                        if (m_CurrentToken.value().IsValid() &&
                            m_CurrentToken.value().type == TokenType::RightAngleBracket)
                        {
                            // Consume the arrow.
                            Consume();

                            auto type_opt = Type::FromToken(*m_CurrentToken);
                            if (type_opt)
                            {
                                // Consume the type.
                                Consume();
                                func_stmt.type = std::move(*type_opt);
                            }
                            else
                            {
                                CompileError(*m_CurrentToken, "Unknown type '{}'.", m_CurrentToken.value().span.text);
                            }
                        }
                        else
                        {
                            CompileError(*m_CurrentToken, "Expected an arrow return type specifier.");
                        }
                    }

                    // Parse the function's scope.
                    if (m_CurrentToken.value().type == TokenType::LeftCurlyBrace)
                    {
                        // Consume the left curly brace.
                        Consume();

                        // Our scope statement.
                        Statement block_scope{};
                        block_scope.kind = StatementKind::Block;

                        // Parse the function's statements.
                        auto body_stmts      = ParseFunctionBody();
                        block_scope.children = std::move(body_stmts);

                        // After parsing the function body we should be left with just a right curly brace so just
                        // consume it and finally, end the function declaration.
                        if (m_CurrentToken.value().IsValid() &&
                            m_CurrentToken.value().type == TokenType::RightCurlyBrace)
                        {
                            Consume();
                            func_stmt.children.push_back(std::move(block_scope));
                        }
                        else
                        {
                            CompileError(*m_CurrentToken, "Expected a closing curly brace but got a {} instead.",
                                         m_CurrentToken.value().ToString());
                        }
                    }
                    else
                    {
                        CompileError(*m_CurrentToken, "Expected a function scope start but got a {} instead.",
                                     m_CurrentToken.value().ToString());
                    }
                }
                else
                {
                    CompileError(*m_CurrentToken,
                                 "Expected a function return type specifier or a function scope start.");
                }

                return func_stmt;
            }
            else
            {
                CompileError(prev_token, "Expected an Identifier token but got an {} token.",
                             m_CurrentToken.value().ToString());
            }
        }
        return std::nullopt;
    }

    Statement Parser::ExpectFunctionParameterList()
    {
        Statement params{};
        if (m_CurrentToken.value().type == TokenType::LeftBrace)
        {
            // Consume the left brace.
            auto prev_token = *Consume();

            // If our token is not eof.
            while (m_CurrentToken.value().IsValid())
            {
                // Our possible parameter.
                Statement parameter{};

                // Possible parameter definition.
                if (m_CurrentToken.value().type == TokenType::Identifier)
                {
                    // Consume the identifier.
                    auto ident = *Consume();

                    parameter.name  = ident.span.text;
                    parameter.kind  = StatementKind::FunctionParameter;
                    parameter.token = ident;

                    // Next, we expect the token to be valid and a colon because
                    // types are defined in the following syntax: identifier: type, ...
                    if (m_CurrentToken.value().IsValid() && m_CurrentToken.value().type == TokenType::Colon)
                    {
                        // Consume the colon.
                        Consume();

                        // If the following token is valid and a keyword,
                        // hopefully a type.
                        if (m_CurrentToken.value().IsValid() && m_CurrentToken.value().IsKeyword())
                        {
                            // Consume the possible type token.
                            auto type_token = *Consume();

                            // Try and create a type from the token. If we get a nothing then it was not a type
                            // so throw a compile error and exit.
                            auto type_opt = Type::FromToken(type_token);
                            if (type_opt.has_value())
                                parameter.type = *type_opt;
                            else
                            {
                                CompileError(type_token, "Expected a type, instead got a {}", type_token.ToString());
                            }
                        }
                        else
                        {
                            CompileError(*m_CurrentToken, "Expected a type specifier for the parameter.");
                        }
                    }
                    else
                    {
                        CompileError(*m_CurrentToken, "Expected a type specifier for the parameter.");
                    }
                }
                else if (m_CurrentToken.value().type == TokenType::Comma)
                {
                    // More parameters so just progress forward and push our current function parameter.
                    Consume();
                    params.children.push_back(std::move(parameter));
                }
                else if (m_CurrentToken.value().type == TokenType::RightBrace)
                {
                    // We've reached the end so terminate.
                    break;
                }
            }

            if (!m_CurrentToken.value().IsValid())
            {
                CompileError(*m_CurrentToken, "Expected a closing brace after function parameter list declaration.");
            }
            else
            {
                // Consume the right brace.
                Consume();
                params.kind = StatementKind::FunctionParemeterList;
            }
        }
        else
        {
            CompileError(*m_CurrentToken, "Expected a parameter list.");
        }
        return params;
    }

    std::vector<Statement> Parser::ParseFunctionBody()
    {
        std::vector<ast::Statement> stmts{};
        while (m_CurrentToken.value().IsValid() && m_CurrentToken.value().type != TokenType::RightCurlyBrace)
        {
            auto stmt = ExpectLocalFunctionStatement();
            if (stmt)
                stmts.push_back(std::move(*stmt));
            else
            {
                CompileError(*m_CurrentToken, "Invalid statement.");
            }
        }
        return stmts;
    }

    std::optional<Statement> Parser::ExpectLocalFunctionStatement()
    {
        auto result = ExpectVariableDeclaration();
        if (result)
            return result;

        result = ExpectKeyword();
        if (result)
            return result;

        result = ExpectExpression();
        if (result)
        {
            if (m_CurrentToken.value().IsValid() && m_CurrentToken.value().type == TokenType::SemiColon)
                return result;
            else
            {
                CompileError(*m_CurrentToken, "Expected a semicolon but got {} instead.",
                             m_CurrentToken.value().ToString());
            }
        }

        return std::nullopt;
    }

    std::optional<ast::Statement> Parser::ExpectVariableDeclaration()
    {
        return std::nullopt;
    }

    std::optional<ast::Statement> Parser::ExpectKeyword()
    {
        // If our token is valid and an actual keyword (obviously).
        if (m_CurrentToken.value().IsValid() && m_CurrentToken.value().IsKeyword())
        {
            switch (m_CurrentToken.value().type)
            {
                using enum TokenType;

                case KeywordReturn: {
                    // Consume the return token.
                    Consume();

                    // The return statement.
                    Statement stmt{};
                    stmt.kind = StatementKind::Return;

                    // If an expression follows our return statement.
                    auto exp = ExpectExpression();
                    if (exp)
                        stmt.children.push_back(std::move(*exp));

                    // Check for the semicolon of course.
                    if (m_CurrentToken.value().IsValid() && m_CurrentToken.value().type == TokenType::SemiColon)
                    {
                        // Consume the semicolon.
                        Consume();
                        return stmt;
                    }
                    else
                    {
                        CompileError(*m_CurrentToken, "Expected a semicolon after the return statement.");
                    }
                    break;
                }
                default: break;
            }
        }
        return std::nullopt;
    }

    std::optional<Statement> Parser::ExpectExpression()
    {
        auto result = ExpectLiteral();
        if (result)
            return result;

        result = ExpectIdentifierName();
        if (result)
            return result;

        return std::nullopt;
    }

    std::optional<Statement> Parser::ExpectLiteral()
    {
        if (m_CurrentToken.value().IsValid())
        {
            switch (m_CurrentToken.value().type)
            {
                using enum TokenType;

                case NumberLiteral: {
                    auto      token = *Consume();
                    Statement stmt{};
                    stmt.kind  = StatementKind::LiteralExpression;
                    stmt.type  = Type::Integer64;
                    stmt.token = std::move(token);
                    return stmt;
                }
                case StringLiteral: {
                    auto      token = *Consume();
                    Statement stmt{};
                    stmt.kind  = StatementKind::LiteralExpression;
                    stmt.type  = Type::String;
                    stmt.token = std::move(token);
                    return stmt;
                }
                case CharacterLiteral: {
                    auto      token = *Consume();
                    Statement stmt{};
                    stmt.kind  = StatementKind::LiteralExpression;
                    stmt.type  = Type::Character;
                    stmt.token = std::move(token);
                    return stmt;
                }
                case KeywordTrue:
                case KeywordFalse: {
                    auto      token = *Consume();
                    Statement stmt{};
                    stmt.kind  = StatementKind::LiteralExpression;
                    stmt.type  = Type::Boolean;
                    stmt.token = std::move(token);
                    return stmt;
                }
                default: break;
            }
        }
        return std::nullopt;
    }

    std::optional<Statement> Parser::ExpectIdentifierName()
    {
        if (m_CurrentToken.value().IsValid() && m_CurrentToken.value().type == TokenType::Identifier)
        {
            // Consume the identifier token.
            auto token = *Consume();

            Statement stmt{};
            stmt.kind = StatementKind::IdentifierName;
            stmt.name = token.span.text;
        }
        return std::nullopt;
    }
} // namespace cmm::cmc
