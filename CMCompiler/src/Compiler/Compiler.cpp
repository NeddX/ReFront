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
        Symbol sym{};
        sym.stmt = var;
        sym.name = var.name;
        sym.kind = SymbolKind::Variable;

        auto& current_table = m_SymbolTableStack.back();

        switch (var.children[0].kind)
        {
            using enum StatementKind;

            case Initializer: {
                CompileLiteral(var.children[0]);
                current_table.AddSymbol(sym);
                break;
            }
            default: break;
        }
    }

    void Compiler::CompileLiteral(const Statement& literal)
    {
        auto& current_table = m_SymbolTableStack.back();
        switch (literal.children[0].type.ftype)
        {
            using enum FundamentalType;

            case Integer64: {
                m_CompiledCode.push_back(
                    Instruction{ .opcode       = OpCode::Store,
                                 .imm64        = (u64)literal.children[0].GetToken(TokenType::NumberLiteral)->num,
                                 .sreg         = RegType::BP,
                                 .displacement = (i32)current_table.GetOffset(),
                                 .size         = 64 });
                break;
            }
            case Boolean: {
                m_CompiledCode.push_back(Instruction{ .opcode = OpCode::Store,
                                                      .imm64  = (u64)literal.GetToken(TokenType::NumberLiteral)->num,
                                                      .sreg   = RegType::BP,
                                                      .displacement = (i32)current_table.GetOffset(),
                                                      .size         = 8 });
                break;
            }
            case Character: {
                m_CompiledCode.push_back(Instruction{ .opcode = OpCode::Store,
                                                      .imm64  = (u64)literal.GetToken(TokenType::NumberLiteral)->num,
                                                      .sreg   = RegType::BP,
                                                      .displacement = (i32)current_table.GetOffset(),
                                                      .size         = 8 });
                break;
            }
            case String: {
                break;
            }

            default: break;
        }
    }
} // namespace cmm::cmc
