#include "Parser.h"

#include <fmt/core.h>

// TODO: Improve and instead use exceptions.
#define CompileError(token, ...)                                                                                       \
    std::cerr << fmt::format("Compile Error @ line ({}, {}): ", (token).span.line, (token).span.cur)                   \
              << fmt::format(__VA_ARGS__) << std::endl;                                                                \
    std::exit(-1);

namespace cmm::cmc {
    using namespace ast;

    namespace ast {
        Type Type::Integer32 = Type{ .name = "Integer32", .ftype = FundamentalType::Integer32 };
        Type Type::Integer64 = Type{ .name = "Integer64", .ftype = FundamentalType::Integer64 };
        Type Type::String    = Type{ .name = "CString", .ftype = FundamentalType::String };
        Type Type::Character = Type{ .name = "Character8", .ftype = FundamentalType::Character };
        Type Type::Boolean   = Type{ .name = "Boolean", .ftype = FundamentalType::Boolean };

        bool Type::IsArray() const noexcept
        {
            return length > 0;
        }

        bool Type::operator==(const Type& type) const noexcept
        {
            return this->ftype == type.ftype && this->name == type.name && this->length == type.length;
        }
    } // namespace ast

    void SymbolTable::AddSymbol(Symbol symbol) noexcept
    {
        m_Symbols[symbol.name] = std::move(symbol);
    }

    std::optional<Symbol> SymbolTable::GetSymbol(const std::string& name) noexcept
    {
        if (m_Symbols.contains(name))
            return m_Symbols[name];
        return std::nullopt;
    }

    std::optional<Type> Type::FromToken(const Token& token) noexcept
    {
        switch (token.type)
        {
            using enum TokenType;

            case KeywordI32: return Type::Integer32;
            case KeywordI64: return Type::Integer64;
            case KeywordString: return Type::String;
            case KeywordChar: return Type::Character;
            case KeywordBool: return Type::Boolean;
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
                prev_token     = *Consume();
                func_stmt.kind = StatementKind::FunctionDeclaration;
                func_stmt.tokens.push_back(std::move(prev_token));

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

                    // The optional colon for one liners.
                    if (m_CurrentToken.value().IsValid() && m_CurrentToken.value().type == TokenType::Colon)
                        // Consume the token.
                        Consume();

                    auto body_stmt = ExpectLocalStatement();
                    if (body_stmt)
                        func_stmt.children.push_back(std::move(*body_stmt));
                    else
                    {
                        CompileError(*m_CurrentToken, "Expected a statement.");
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

            // Our possible parameter.
            Statement parameter{};

            // If our token is not eof.
            while (m_CurrentToken.value().IsValid())
            {
                // Possible parameter definition.
                if (m_CurrentToken.value().type == TokenType::Identifier)
                {
                    // Consume the identifier.
                    auto ident = *Consume();

                    parameter.name = ident.span.text;
                    parameter.kind = StatementKind::FunctionParameter;
                    parameter.tokens.push_back(std::move(ident));

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
                            if (type_opt)
                                parameter.type = std::move(*type_opt);
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

                    // Finally, push our parameter statement to our parameter list.
                    params.children.push_back(std::move(parameter));
                }
                else if (m_CurrentToken.value().type == TokenType::Comma)
                {
                    // There are more parameters so just progress forward.
                    Consume();
                }
                else if (m_CurrentToken.value().type == TokenType::RightBrace)
                {
                    // We've reached the end so terminate.
                    break;
                }
                else
                {
                    CompileError(*m_CurrentToken, "Expected a function parameter.");
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

    std::optional<Statement> Parser::ExpectLocalStatement()
    {
        // Check for a compound statement.
        auto result = ExpectBlockStatement();
        if (result)
            return result;

        // Else check for a variable declaration statement.
        result = ExpectVariableDeclaration();
        if (result)
            return result;

        // Else check for a keyword statement.
        result = ExpectKeyword();
        if (result)
            return result;

        // Else just check for a possible expression statement.
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

    std::optional<ast::Statement> Parser::ExpectBlockStatement()
    {
        // Check for a start of a block statement.
        if (m_CurrentToken.value().IsValid() && m_CurrentToken.value().type == TokenType::LeftCurlyBrace)
        {
            // Create a new symbol table for our compound statement and push it onto the stack.
            m_SymbolTableStack.emplace();

            // Consume the left curly brace.
            auto brace_token = *Consume();

            // Our block statement.
            Statement block_stmt;
            block_stmt.kind = StatementKind::Block;
            block_stmt.tokens.push_back(std::move(brace_token));

            // Iterate through the tokens until we hit a closing curly brace.
            while (m_CurrentToken.value().type != TokenType::RightCurlyBrace)
            {
                // If we meet a EOF instead of a closing curly brace.
                if (!m_CurrentToken.value().IsValid())
                {
                    CompileError(*m_CurrentToken, "Expected a closing curly brace to end the block statement.");
                }
                else
                {
                    // Recursevly parse statements and append them to our block statement (if any).
                    auto stmt = ExpectLocalStatement();
                    if (stmt)
                        block_stmt.children.push_back(std::move(*stmt));
                }
            }

            // Consume the closing curly brace.
            brace_token = *Consume();
            block_stmt.tokens.push_back(std::move(brace_token));

            // Pop our compound statement's symbol table out and finally return our compound statement.
            m_SymbolTableStack.pop();
            return block_stmt;
        }
        return std::nullopt;
    }

    std::optional<ast::Statement> Parser::ExpectVariableDeclaration()
    {
        if (m_CurrentToken.value().IsValid() && m_CurrentToken.value().type == TokenType::KeywordLet)
        {
            // Consume our let token.
            Token let_token = *Consume();
            Token ident_token{};

            // Our variable declaration statement.
            Statement var_decl{};
            var_decl.kind = StatementKind::VariableDeclaration;
            var_decl.tokens.push_back(std::move(let_token));

            // The following token must be valid and an identifier.
            if (m_CurrentToken.value().IsValid() && m_CurrentToken.value().type == TokenType::Identifier)
            {
                ident_token   = *Consume();
                var_decl.name = ident_token.span.text;
                var_decl.tokens.push_back(ident_token);
            }
            else
            {
                CompileError(*m_CurrentToken, "Expected an identifier after the let keyword.");
            }

            // The token following the identifier must be a colon type specifier.
            if (auto token = Consume(); !token.value().IsValid() || token.value().type != TokenType::Colon)
            {
                CompileError(*m_CurrentToken, "Expected a colon type specifier.");
            }

            // The following token now must be a type.
            if (m_CurrentToken.value().IsValid())
            {
                // Consume our type token then try and create type from it.
                auto type_token = *Consume();
                auto type_opt   = Type::FromToken(type_token);

                if (type_opt)
                {
                    // Check if it is a possible array.
                    if (m_CurrentToken.value().type == TokenType::LeftSquareBracket)
                    {
                        // Consume the opening square bracket.
                        Consume();

                        // The following token must be a length specifier in the form of a number literal.
                        if (m_CurrentToken.value().type == TokenType::NumberLiteral)
                            type_opt.value().length = Consume().value().num;
                        else
                        {
                            CompileError(*m_CurrentToken,
                                         "Expected an array length specifier in the form of an integer literal.");
                        }

                        // The following token must be a closing square bracket.
                        if (auto rsq_bracket = *Consume(); rsq_bracket.type != TokenType::RightSquareBracket)
                        {
                            CompileError(rsq_bracket, "Expected a closing square bracket.");
                        }
                    }
                    var_decl.type = std::move(*type_opt);
                }
                else
                {
                    CompileError(*m_CurrentToken, "Unknown type {}.", type_token.span.text);
                }
            }
            else
            {
                CompileError(*m_CurrentToken, "Expected a type.");
            }

            // Now we either have a semicolon or initializer.
            if (m_CurrentToken.value().IsValid())
            {
                // It is an initializer.
                if (m_CurrentToken.value().type == TokenType::Equals)
                {
                    // Consume the equals.
                    auto equals_token = *Consume();

                    // Our initializer statement.
                    Statement init_stmt{};
                    init_stmt.tokens.push_back(std::move(equals_token));
                    init_stmt.kind = StatementKind::Initializer;

                    // The initializer expression.
                    auto init_expr = ExpectExpression();
                    if (init_expr)
                    {
                        // Check if our variable is an array.
                        if (var_decl.type.IsArray())
                        {
                            // If the lengths mismatch then it's an error.
                            if (init_expr.value().children.size() != var_decl.type.length)
                            {
                                CompileError(init_expr.value().tokens[0],
                                             "'{}' is an array of {} elements but is initialized with an initializer "
                                             "list of length {}.",
                                             var_decl.name, var_decl.type.length, init_expr.value().children.size());
                            }

                            // Check if there's a type mismatch.
                            for (const auto& e : init_expr.value().children)
                            {
                                // Compare the ELEMENT types.
                                if (e.type.ftype != var_decl.type.ftype)
                                {
                                    CompileError(
                                        e.tokens.at(0),
                                        "Type mistmatch. Cannot perform implicit conversion from '{}' to '{}'.",
                                        e.type.name, var_decl.type.name);
                                }
                            }
                        }
                        else
                        {
                            // Check if there's a type mismatch between the initializer expression and the variable.
                            if (init_expr.value().type != var_decl.type)
                            {
                                CompileError(init_stmt.tokens[0],
                                             "Type mismatch. Cannot perform implicit conversion from '{}' to '{}'.",
                                             init_expr.value().type.name, var_decl.type.name);
                            }
                        }

                        // If we reached here then everything is fine so just append our initializer and move on.
                        init_stmt.children.push_back(std::move(*init_expr));
                    }

                    // Append our initializer statement.
                    var_decl.children.push_back(std::move(init_stmt));

                    // Consume and check if the last token is a semicolon or not.
                    if (auto semi_token = *Consume(); !semi_token.IsValid() || semi_token.type != TokenType::SemiColon)
                    {
                        CompileError(semi_token, "Expected a semicolon.");
                    }
                }
                else if (m_CurrentToken.value().type == TokenType::SemiColon)
                {
                    // Consume the colon, this is just an uninitialized variable.
                    Consume();
                }

                // Check if the variable already exists in our block's symbol table.
                if (auto sym = m_SymbolTableStack.top().GetSymbol(var_decl.name); sym)
                {
                    auto& redecl_token = sym.value().statement.tokens[0];
                    CompileError(var_decl.tokens[0],
                                 "Redeclaration of an already existing name {} previously defined @ line ({}, {}).",
                                 var_decl.name, redecl_token.span.line, redecl_token.span.cur);
                }
                else
                {
                    // Append our new variable to our symbol table and return it.
                    m_SymbolTableStack.top().AddSymbol(Symbol{ .name = ident_token.span.text, .statement = var_decl });
                }
                return var_decl;
            }
        }
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
        // Check for a literal expression.
        auto result = ExpectLiteral();
        if (result)
            return result;

        // Else check for an identifier expression.
        result = ExpectIdentifierName();
        if (result)
            return result;

        // Else check for an initializer list expression.
        result = ExpectInitializerList();
        if (result)
            return result;

        // more to come!

        return std::nullopt;
    }

    std::optional<Statement> Parser::ExpectLiteral()
    {
        // If our token is valid.
        if (m_CurrentToken.value().IsValid())
        {
            // Check for the type of the literal.
            switch (m_CurrentToken.value().type)
            {
                using enum TokenType;

                case NumberLiteral: {
                    auto      token = *Consume();
                    Statement stmt{};
                    stmt.kind = StatementKind::LiteralExpression;
                    stmt.type = Type::Integer64;
                    stmt.tokens.push_back(std::move(token));
                    return stmt;
                }
                case StringLiteral: {
                    auto      token = *Consume();
                    Statement stmt{};
                    stmt.kind = StatementKind::LiteralExpression;
                    stmt.type = Type::String;
                    stmt.tokens.push_back(std::move(token));
                    return stmt;
                }
                case CharacterLiteral: {
                    auto      token = *Consume();
                    Statement stmt{};
                    stmt.kind = StatementKind::LiteralExpression;
                    stmt.type = Type::Character;
                    stmt.tokens.push_back(std::move(token));
                    return stmt;
                }
                case KeywordTrue:
                case KeywordFalse: {
                    auto      token = *Consume();
                    Statement stmt{};
                    stmt.kind = StatementKind::LiteralExpression;
                    stmt.type = Type::Boolean;
                    stmt.tokens.push_back(std::move(token));
                    return stmt;
                }
                default: break;
            }
        }
        return std::nullopt;
    }

    std::optional<Statement> Parser::ExpectIdentifierName()
    {
        // If the current token is infact an identifier.
        if (m_CurrentToken.value().IsValid() && m_CurrentToken.value().type == TokenType::Identifier)
        {
            // Consume the identifier token.
            auto token = *Consume();

            // Create our identifier statement.
            Statement stmt{};
            stmt.kind = StatementKind::IdentifierName;
            stmt.name = token.span.text;

            // Perform a symbol table lookup here perhaps?
        }
        return std::nullopt;
    }

    std::optional<Statement> Parser::ExpectInitializerList()
    {
        // Check if the token is infact an opening curly brace.
        if (m_CurrentToken.value().type == TokenType::LeftCurlyBrace)
        {
            // Consume the opening curly brace.
            auto left_curly = *Consume();

            // Our initializer list statement.
            Statement init_list{};
            init_list.kind = StatementKind::InitializerList;
            init_list.tokens.push_back(std::move(left_curly));

            // Parse the tokens until we hit a closing curly brace.
            while (m_CurrentToken.value().type != TokenType::RightCurlyBrace)
            {
                // Parse the expression element.
                auto expr = ExpectExpression();
                if (expr)
                {
                    // Either a comma must follow our parsed expression or the initializer list should end, otherwise
                    // it's a compile error.
                    if (m_CurrentToken.value().type == TokenType::Comma)
                        // Consume the comma and move on.
                        Consume();
                    else if (m_CurrentToken.value().type != TokenType::RightCurlyBrace)
                    {
                        CompileError(*m_CurrentToken, "Expected a closing curly brace.");
                    }
                    init_list.children.push_back(std::move(*expr));
                }
                else
                {
                    CompileError(*m_CurrentToken, "Invalid expression inside of an initializer list.");
                }
            }

            // Check if our initializer list was properly established.
            if (auto closing_curly = *Consume(); closing_curly.IsValid())
            {
                // Consume and append our closing curly brace to the initializer list statement.
                init_list.tokens.push_back(closing_curly);
                return init_list;
            }
            else
            {
                CompileError(*m_CurrentToken, "Expected a closing curly brace.");
            }
        }
        return std::nullopt;
    }
} // namespace cmm::cmc
