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
    } // namespace ast

    void SymbolTable::AddSymbol(Symbol symbol) noexcept
    {
        m_Symbols[symbol.name] = std::move(symbol);
    }

    bool SymbolTable::ContainsSymbol(const std::string& name) const noexcept
    {
        return m_Symbols.contains(name);
    }

    Symbol& SymbolTable::GetSymbol(const std::string& name) noexcept
    {
        return m_Symbols[name];
    }

    const Symbol& SymbolTable::GetSymbol(const std::string& name) const noexcept
    {
        return ((SymbolTable*)this)->GetSymbol(name);
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

    std::optional<Token> Parser::Peek() noexcept
    {
        return m_Lexer.PeekToken();
    }

    std::optional<Statement> Parser::ExpectFunctionDecl()
    {
        if (m_CurrentToken->type == TokenType::KeywordFn)
        {
            // Consume the Fn keyword.
            auto prev_token = *Consume();

            // Our function declaration statement.
            Statement func_stmt{};

            // If the following token is an identifier.
            if (m_CurrentToken->type == TokenType::Identifier)
            {
                // Consume the identifier.
                prev_token     = *Consume();
                func_stmt.kind = StatementKind::FunctionDeclaration;
                func_stmt.tokens.push_back(std::move(prev_token));

                // Parse possible parameter list, if there's none then our parameter list statement will just be empty.
                auto param_list = ExpectFunctionParameterList();
                func_stmt.children.push_back(std::move(param_list));

                // Parse the possible return type or a function scope start.
                if (m_CurrentToken->IsValid())
                {
                    // Parse the possible arrow return type specifier.
                    if (m_CurrentToken->type == TokenType::Minus)
                    {
                        // Consume the dash.
                        Consume();

                        if (m_CurrentToken->IsValid() && m_CurrentToken->type == TokenType::RightAngleBracket)
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
                                CompileError(*m_CurrentToken, "Unknown type '{}'.", m_CurrentToken->span.text);
                            }
                        }
                        else
                        {
                            CompileError(*m_CurrentToken, "Expected an arrow return type specifier.");
                        }
                    }

                    // Parse the function body.
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
                             m_CurrentToken->ToString());
            }
        }
        return std::nullopt;
    }

    Statement Parser::ExpectFunctionParameterList()
    {
        Statement params{};
        if (m_CurrentToken->type == TokenType::LeftBrace)
        {
            // Consume the left brace.
            auto prev_token = *Consume();

            // Our possible parameter.
            Statement parameter{};

            // If our token is not eof.
            while (m_CurrentToken->IsValid())
            {
                // Possible parameter definition.
                if (m_CurrentToken->type == TokenType::Identifier)
                {
                    // Consume the identifier.
                    auto ident = *Consume();

                    parameter.name = ident.span.text;
                    parameter.kind = StatementKind::FunctionParameter;
                    parameter.tokens.push_back(std::move(ident));

                    // Next, we expect the token to be valid and a colon because
                    // types are defined in the following syntax: identifier: type, ...
                    if (m_CurrentToken->IsValid() && m_CurrentToken.value().type == TokenType::Colon)
                    {
                        // Consume the colon.
                        Consume();

                        // If the following token is valid and a keyword,
                        // hopefully a type.
                        if (m_CurrentToken->IsValid() && m_CurrentToken.value().IsKeyword())
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
                else if (m_CurrentToken->type == TokenType::Comma)
                {
                    // There are more parameters so just progress forward.
                    Consume();
                }
                else if (m_CurrentToken->type == TokenType::RightBrace)
                {
                    // We've reached the end so terminate.
                    break;
                }
                else
                {
                    CompileError(*m_CurrentToken, "Expected a function parameter.");
                }
            }

            if (!m_CurrentToken->IsValid())
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
        if (!result)
            result = ExpectVariableDeclaration();

        // Else check for a keyword statement.
        if (!result)
            result = ExpectKeyword();

        // Else just check for a possible expression statement.
        if (!result)
            result = ExpectExpression();

        // Check for the semicolon.
        if (m_CurrentToken->type == TokenType::SemiColon)
            // Consume the semicolon.
            Consume();
        else
        {
            CompileError(*m_CurrentToken, "Expected a semicolon but got {} instead.", m_CurrentToken->ToString());
        }

        return result;
    }

    std::optional<ast::Statement> Parser::ExpectBlockStatement()
    {
        // Check for a start of a block statement.
        if (m_CurrentToken->type == TokenType::LeftCurlyBrace)
        {
            // Create a new symbol table for our compound statement and push it onto the stack.
            m_SymbolTableStack.push_back(SymbolTable{});

            // Consume the left curly brace.
            auto brace_token = *Consume();

            // Our block statement.
            Statement block_stmt;
            block_stmt.kind = StatementKind::BlockStatement;
            block_stmt.tokens.push_back(std::move(brace_token));

            // Iterate through the tokens until we hit a closing curly brace.
            while (m_CurrentToken->type != TokenType::RightCurlyBrace)
            {
                // If we meet a EOF instead of a closing curly brace.
                if (!m_CurrentToken->IsValid())
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
            m_SymbolTableStack.pop_back();
            return block_stmt;
        }
        return std::nullopt;
    }

    std::optional<ast::Statement> Parser::ExpectVariableDeclaration()
    {
        if (m_CurrentToken->IsValid() && m_CurrentToken.value().type == TokenType::KeywordLet)
        {
            // Consume our let token.
            Token let_token = *Consume();
            Token ident_token{};

            // Our variable declaration statement.
            Statement var_decl{};
            var_decl.kind = StatementKind::VariableDeclaration;
            var_decl.tokens.push_back(std::move(let_token));

            // The following token must be valid and an identifier.
            if (m_CurrentToken->IsValid() && m_CurrentToken.value().type == TokenType::Identifier)
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
            if (auto token = Consume(); !token->IsValid() || token.value().type != TokenType::Colon)
            {
                CompileError(*m_CurrentToken, "Expected a colon type specifier.");
            }

            // The following token now must be a type.
            if (m_CurrentToken->IsValid())
            {
                // Consume our type token then try and create type from it.
                auto type_token = *Consume();
                auto type_opt   = Type::FromToken(type_token);

                if (type_opt)
                {
                    // Check if it is a possible array.
                    if (m_CurrentToken->type == TokenType::LeftSquareBracket)
                    {
                        // Consume the opening square bracket.
                        Consume();

                        // The following token must be a length specifier in the form of a number literal.
                        if (m_CurrentToken->type == TokenType::NumberLiteral)
                            type_opt->length = Consume().value().num;
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
            if (m_CurrentToken->IsValid())
            {
                // It is an initializer.
                if (m_CurrentToken->type == TokenType::Equals)
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
                            if (init_expr->children.size() != var_decl.type.length)
                            {
                                CompileError(init_expr->tokens[0],
                                             "'{}' is an array of {} elements but is initialized with an initializer "
                                             "list of length {}.",
                                             var_decl.name, var_decl.type.length, init_expr->children.size());
                            }

                            // Check if there's a type mismatch.
                            for (const auto& e : init_expr->children)
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
                            if (init_expr->type != var_decl.type)
                            {
                                CompileError(init_stmt.tokens[0],
                                             "Type mismatch. Cannot perform implicit conversion from '{}' to '{}'.",
                                             init_expr->type.name, var_decl.type.name);
                            }
                        }

                        // If we reached here then everything is fine so just append our initializer and move on.
                        init_stmt.children.push_back(std::move(*init_expr));
                    }

                    // Append our initializer statement.
                    var_decl.children.push_back(std::move(init_stmt));
                }

                // Check if the variable already exists in our block's symbol table.
                if (m_SymbolTableStack.back().ContainsSymbol(var_decl.name))
                {
                    auto& sym          = m_SymbolTableStack.back().GetSymbol(var_decl.name);
                    auto& redecl_token = sym.statement.tokens[0];
                    CompileError(var_decl.tokens[0],
                                 "Redeclaration of an already existing name '{}' in the same context previously "
                                 "defined @ line ({}, {}).",
                                 var_decl.name, redecl_token.span.line, redecl_token.span.cur);
                }
                else
                {
                    // Append our new variable to our symbol table and return it.
                    m_SymbolTableStack.back().AddSymbol(Symbol{ .name = ident_token.span.text, .statement = var_decl });
                }
                return var_decl;
            }
        }
        return std::nullopt;
    }

    std::optional<ast::Statement> Parser::ExpectKeyword()
    {
        // If our token is valid and an actual keyword (obviously).
        if (m_CurrentToken->IsValid() && m_CurrentToken.value().IsKeyword())
        {
            switch (m_CurrentToken->type)
            {
                using enum TokenType;

                // TODO: For else and else if statements you can use an if statement stack to determine which if
                // statement do they belong but for now I am not going to support else and else if statements.
                case KeywordIf: {
                    // Consume the if keyword.
                    auto if_keyword = *Consume();

                    // Our If statement.
                    Statement if_stmt{};
                    if_stmt.kind = StatementKind::IfStatement;
                    if_stmt.tokens.push_back(std::move(if_keyword));

                    // Save the token.
                    auto pre_cond_token = *m_CurrentToken;

                    // Else it's just a regular if statement.
                    auto condition = ExpectExpression();
                    if (condition)
                    {
                        // Check if the expression type is a boolean.
                        if (condition->type == Type::Boolean)
                        {
                            // Append our condition statement.
                            if_stmt.children.push_back(std::move(*condition));

                            // Save the token.
                            auto pre_body_token = *m_CurrentToken;

                            // The body for the if statement.
                            auto body_stmt = ExpectLocalStatement();
                            if (body_stmt)
                                if_stmt.children.push_back(std::move(*body_stmt));
                            else
                            {
                                CompileError(pre_body_token, "Expected a body for the if statement.");
                            }

                            // Finally return our if statement.
                            return if_stmt;
                        }
                        else
                        {
                            CompileError(pre_cond_token,
                                         "Type mismatch. Cannot perform implicit conversion from '{}' to '{}'.",
                                         condition->type.name, Type::Boolean.name);
                        }
                    }
                    else
                    {
                        CompileError(pre_cond_token, "Expected an expression evaluating to bool.");
                    }
                    break;
                }
                case KeywordWhile: {
                    // Consume the while keyword.
                    auto while_keyword = *Consume();

                    // Our If statement.
                    Statement while_stmt{};
                    while_stmt.kind = StatementKind::WhileStatement;
                    while_stmt.tokens.push_back(std::move(while_keyword));

                    // Save the token.
                    auto pre_cond_token = *m_CurrentToken;

                    // Our while's condition statement.
                    auto condition = ExpectExpression();
                    if (condition)
                    {
                        // Check if the expression type is a boolean.
                        if (condition->type == Type::Boolean)
                        {
                            // Append our condition statement.
                            while_stmt.children.push_back(std::move(*condition));

                            // Save the token.
                            auto pre_body_token = *m_CurrentToken;

                            // The body for the if statement.
                            auto body_stmt = ExpectLocalStatement();
                            if (body_stmt)
                                while_stmt.children.push_back(std::move(*body_stmt));
                            else
                            {
                                CompileError(pre_body_token, "Expected a body for the while statement.");
                            }

                            // Finally return our while statement.
                            return while_stmt;
                        }
                        else
                        {
                            CompileError(pre_cond_token,
                                         "Type mismatch. Cannot perform implicit conversion from '{}' to '{}'.",
                                         condition->type.name, Type::Boolean.name);
                        }
                    }
                    else
                    {
                        CompileError(pre_cond_token, "Expected an expression evaluating to bool.");
                    }
                    break;
                }
                case KeywordReturn: {
                    // Consume the return token.
                    Consume();

                    // The return statement.
                    Statement stmt{};
                    stmt.kind = StatementKind::ReturnStatement;

                    // If an expression follows our return statement.
                    auto exp = ExpectExpression();
                    if (exp)
                        stmt.children.push_back(std::move(*exp));

                    // Check for the semicolon of course.
                    if (m_CurrentToken->IsValid() && m_CurrentToken.value().type == TokenType::SemiColon)
                    {
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

        // Else check if it's an assignment expression.
        result = ExpectAssignment();
        if (result)
            return result;

        // Else check for an initializer list expression.
        result = ExpectInitializerList();
        if (result)
            return result;

        // Else check if it's a function call expression.
        result = ExpectFunctionCall();
        if (result)
            return result;

        // Else check for an identifier expression.
        result = ExpectIdentifierName();
        if (result)
            return result;

        // more to come!

        return std::nullopt;
    }

    std::optional<Statement> Parser::ExpectLiteral()
    {
        // If our token is valid.
        if (m_CurrentToken->IsValid())
        {
            // Check for the type of the literal.
            switch (m_CurrentToken->type)
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
        if (m_CurrentToken->IsValid() && m_CurrentToken.value().type == TokenType::Identifier)
        {
            // Consume the identifier token.
            auto ident_token = *Consume();

            // Create our identifier statement.
            Statement name_stmt{};
            name_stmt.kind = StatementKind::IdentifierName;
            name_stmt.name = ident_token.span.text;

            // Perform a symbol table lookup.
            for (auto it = m_SymbolTableStack.rbegin(); it != m_SymbolTableStack.rend(); ++it)
            {
                const auto& e = *it;
                if (e.ContainsSymbol(name_stmt.name))
                    name_stmt.type = e.GetSymbol(name_stmt.name).statement.type;
            }

            // If the type is still void then the lookup most likely failed.
            if (name_stmt.type.IsVoid())
            {
                CompileError(ident_token, "The name '{}' does not exist in the current context.", name_stmt.name);
            }
            else
            {
                // Else everything is fine so return our identifier name reference statement.
                return name_stmt;
            }
        }
        return std::nullopt;
    }

    std::optional<Statement> Parser::ExpectInitializerList()
    {
        // Check if the token is infact an opening curly brace.
        if (m_CurrentToken->type == TokenType::LeftCurlyBrace)
        {
            // Consume the opening curly brace.
            auto left_curly = *Consume();

            // Our initializer list statement.
            Statement init_list{};
            init_list.kind = StatementKind::InitializerList;
            init_list.tokens.push_back(std::move(left_curly));

            // Parse the tokens until we hit a closing curly brace.
            while (m_CurrentToken->type != TokenType::RightCurlyBrace)
            {
                // Parse the expression element.
                auto expr = ExpectExpression();
                if (expr)
                {
                    // Either a comma must follow our parsed expression or the initializer list should end, otherwise
                    // it's a compile error.
                    if (m_CurrentToken->type == TokenType::Comma)
                        // Consume the comma and move on.
                        Consume();
                    else if (m_CurrentToken->type != TokenType::RightCurlyBrace)
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

    std::optional<Statement> Parser::ExpectAssignment()
    {
        // Check if the left hand value of the assignment expression is an identifier.
        if (m_CurrentToken->type == TokenType::Identifier)
        {
            // Peek the next token cause this might not really be an assignment expression.
            auto next = Peek();
            if (next->type == TokenType::Equals)
            {
                // Consume the equals operator.
                Consume();

                // Parse the lhv and unwrap it immediately because we surely now that if it got parsed, then it is valid
                // identifier.
                auto ident_token = *m_CurrentToken;
                auto lhv         = *ExpectIdentifierName();

                // Our assignment expression statement.
                Statement assign_expr{};

                assign_expr.type = lhv.type;
                assign_expr.kind = StatementKind::AssignmentExpression;

                // Save the token.
                auto rhv_token = *m_CurrentToken;

                // Regular assignment expression.
                auto rhv = ExpectExpression();
                if (!rhv)
                {
                    CompileError(rhv_token, "Bad assignment expression.");
                }

                // Check if the types match.
                if (rhv->type == lhv.type)
                {
                    // We're done here so just push the rhv and lhv and return our assignment expression.
                    lhv.name  = "lhv";
                    rhv->name = "rhv";
                    assign_expr.children.push_back(std::move(lhv));
                    assign_expr.children.push_back(std::move(*rhv));
                    return assign_expr;
                }
                else
                {
                    CompileError(rhv_token, "Type mistmatch. Cannot perform implicit conversion from '{}', '{}'.",
                                 lhv.type.name, rhv->type.name);
                }
            }
        }
        return std::nullopt;
    }

    std::optional<ast::Statement> Parser::ExpectFunctionCall()
    {
        return std::nullopt;
    }
} // namespace cmm::cmc
