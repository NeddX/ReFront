#ifndef CMC_COMPILER_H
#define CMC_COMPILER_H

#include <ALVM.h>
#include <vector>

#include "../Analyzer/Parser.h"

namespace cmm::cmc {
    namespace codegen {
        enum class SymbolKind : u8
        {
            None,
            Variable,
            Function
        };

        struct Symbol
        {
            std::string    name{};
            SymbolKind     kind{};
            ast::Statement stmt{};
            usize          size{};
            usize          address{};
        };

        struct SymbolTable
        {
        private:
            std::unordered_map<std::string, Symbol> m_Symbol{};
            usize                                   m_Offset{};

        public:
            inline usize GetOffset() const noexcept { return m_Offset; }

        public:
            void          AddSymbol(Symbol symbol) noexcept;
            bool          ContainsSymbol(const std::string& name) const noexcept;
            Symbol&       GetSymbol(const std::string& name) noexcept;
            const Symbol& GetSymbol(const std::string& name) const noexcept;
        };
    } // namespace codegen

    class Compiler
    {
    private:
        ast::SyntaxTree                   m_Tree{};
        rlang::alvm::InstructionList      m_CompiledCode{};
        std::vector<codegen::SymbolTable> m_SymbolTableStack{};

    public:
        Compiler(ast::SyntaxTree tree);

    public:
        rlang::alvm::InstructionList Compile();
        void                         CompileFunctionBody(const ast::Statement& fnStmt);
        void                         CompileBlockStatement(const ast::Statement& block);
        void                         CompileVariableDeclaration(const ast::Statement& var);
        void                         CompileLiteral(const ast::Statement& var);
    };
} // namespace cmm::cmc

#endif // CMC_COMPILER_H
