#include "Compiler.h"

namespace cmm::cmc {
    using ast::FundamentalType;
    using ast::Statement;
    using ast::StatementKind;
    using ast::SyntaxTree;
    using ast::Type;
    using namespace rlang;
    using namespace rlang::alvm;

    namespace codegen {
        void SymbolTable::AddSymbol(Symbol symbol) noexcept
        {
            m_Symbol[symbol.name] = std::move(symbol);
            m_Offset += symbol.size;
        }

        bool SymbolTable::ContainsSymbol(const std::string& name) const noexcept
        {
            return m_Symbol.contains(name);
        }

        Symbol& SymbolTable::GetSymbol(const std::string& name) noexcept
        {
            return m_Symbol[name];
        }

        const Symbol& SymbolTable::GetSymbol(const std::string& name) const noexcept
        {
            return ((SymbolTable*)this)->GetSymbol(name);
        }
    } // namespace codegen

    using namespace codegen;

    Compiler::Compiler(SyntaxTree tree) : m_Tree(std::move(tree))
    {
    }

    InstructionList Compiler::Compile()
    {
        for (const auto& s : m_Tree)
        {
            switch (s.kind)
            {
                using enum StatementKind;

                case FunctionDeclaration: CompileFunctionBody(s); break;

                default: break;
            }
        }
        m_CompiledCode.push_back(Instruction{ .opcode = OpCode::End });
        return m_CompiledCode;
    }

    void Compiler::CompileFunctionBody(const Statement& fnStmt)
    {
        for (const auto& s : fnStmt.children)
        {
            if (s.kind == StatementKind::BlockStatement)
            {
                CompileBlockStatement(s);
            }
        }
    }

    void Compiler::CompileBlockStatement(const Statement& block)
    {
        m_SymbolTableStack.push_back(SymbolTable{});

        m_CompiledCode.push_back(Instruction{ .opcode = OpCode::Push, .sreg = RegType::BP });
        m_CompiledCode.push_back(Instruction{ .opcode = OpCode::Mov, .sreg = RegType::SP, .dreg = RegType::BP });

        for (const auto& s : block.children)
        {
            switch (s.kind)
            {
                using enum StatementKind;

                case VariableDeclaration: CompileVariableDeclaration(s); break;
                default: break;
            }
        }

        m_SymbolTableStack.pop_back();
        m_CompiledCode.push_back(Instruction{ .opcode = OpCode::Leave });
    }

    void Compiler::CompileVariableDeclaration(const Statement& var)
    {
        // Grab our current block's symbol table.
        auto& current_table = m_SymbolTableStack.back();

        Symbol sym{};
        sym.stmt    = var;
        sym.name    = var.name;
        sym.kind    = SymbolKind::Variable;
        sym.size    = (var.type.size / 8) * ((var.type.length == 0) ? 1 : var.type.length);
        sym.address = current_table.GetOffset();

        // Initialized.
        if (!var.children.empty())
        {
            // We know that a variable declaration statement will always have an Initializer statement if initialized
            // (but of course).
            CompileInitializer(var.children[0]);
            current_table.AddSymbol(sym);
        }
        else
        {
            // Just allocate space on the stack.
            switch (var.type.ftype)
            {
                using enum FundamentalType;

                case Boolean:
                case Character:
                case Integer32:
                case Integer64:
                    m_CompiledCode.push_back(Instruction{ .opcode = OpCode::Store,
                                                          .sreg   = RegType::BP,
                                                          .disp   = current_table.GetOffset(),
                                                          .size   = (i8)(var.type.size / 8) });
                    break;
                // FIXME: Uninitilized strings do not allocate space.
                case String: break;
                default: break;
            }
        }
    }

    void Compiler::CompileInitializer(const Statement& init)
    {
        // Check if the initializer's value is a value, expression or a initializer list.
        switch (init.children[0].kind)
        {
            using enum StatementKind;

            case LiteralExpression: {
                CompileLiteral(init.children[0]);
                break;
            }
            case InitializerList: {
                CompileInitializerList(init.children[0]);
                break;
            }
            default: break;
        }
    }

    void Compiler::CompileExpression(const ast::Statement& expr)
    {
        switch (expr.kind)
        {
            using enum StatementKind;

            case FunctionCallExpression: {
                break;
            }
            case FunctionArgumentList: {
                break;
            }
            case LiteralExpression: {
                CompileLiteral(expr);
                break;
            }
            default: break;
        }
    }

    void Compiler::CompileLiteral(const Statement& literal)
    {
        // Grab our current block's symbol table.
        auto& current_table = m_SymbolTableStack.back();

        // The literal token.
        auto& literal_token = literal.tokens[0];

        // We support fundamental types only for now.
        switch (literal.type.ftype)
        {
            using enum FundamentalType;

            // For numeric types.
            case Boolean:
            case Character:
            case Integer32:
            case Integer64: {
                m_CompiledCode.push_back(Instruction{ .opcode = OpCode::Store,
                                                      .imm64  = (u64)literal_token.num,
                                                      .sreg   = RegType::BP,
                                                      .disp   = current_table.GetOffset(),
                                                      .size   = (i8)(literal.type.size / 8) });
                break;
            }
            case String: {
                auto offset = current_table.GetOffset();
                for (char c : literal_token.span.text)
                {
                    m_CompiledCode.push_back(Instruction{ .opcode = OpCode::Store,
                                                          .imm64  = (u64)c,
                                                          .sreg   = RegType::BP,
                                                          .disp   = offset,
                                                          .size   = (i8)(Type::Character.size / 8) });
                    offset += Type::Character.size / 8;
                }
                break;
            }

            default: break;
        }
    }

    void Compiler::CompileInitializerList(const ast::Statement& initList)
    {
        // Grab our current block's symbol table.
        auto& current_table = m_SymbolTableStack.back();

        auto prev_offset = current_table.GetOffset();
        for (const auto& expr : initList.children)
        {
            CompileExpression(expr);
            current_table.GetOffset() += expr.type.size / 8;
        }

        current_table.GetOffset() = prev_offset;
    }
} // namespace cmm::cmc
