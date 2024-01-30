#include "Parser.h"

namespace cmm::cmc {
    using namespace ast;

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

    // TODO: Improve and instead use exceptions.
    template <typename... TArgs>
    void CompileError(const Token& token, const char* fmt, TArgs&&... args)
    {
        std::cerr << std::format("Compile Error @ line ({}, {}): ", token.span.line, token.span.cur)
                  << std::format(fmt, std::forward<TArgs>(args)...) << std::endl;
        std::exit(-1);
    }

    Parser::Parser(const std::string_view source) noexcept : m_Source(source)
    {
    }

    std::vector<Statement> Parser::Parse() noexcept
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

    std::optional<Statement> Parser::ExpectFunctionDecl() noexcept
    {
        if (m_CurrentToken.value().type == TokenType::KeywordFn)
        {
            // Consume the Fn keyword.
            auto prev_token = *Consume();

            // Our function declaration statement.
            Statement func_stmt{};

            // If we have a valid token.
            if (m_CurrentToken.value().type == TokenType::Identifier)
            {
                // Consume the identifier.
                prev_token      = *Consume();
                func_stmt.kind  = StatementKind::FunctionDeclaration;
                func_stmt.token = std::move(prev_token);

                auto param_list = ExpectFunctionParameterList();
                if (param_list.has_value())
                    func_stmt.children.push_back(std::move(func_stmt));

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

    std::optional<Statement> Parser::ExpectFunctionParameterList() noexcept
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
                params.kind = StatementKind::FunctionParemeterList;
                if (!params.children.empty())
                    return params;
            }
        }
        else
        {
            CompileError(*m_CurrentToken, "Expected a parameter list.");
        }
        return std::nullopt;
    }
} // namespace cmm::cmc
