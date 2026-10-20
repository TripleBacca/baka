#pragma once
#include "ir.hh"
#include "semantic/semantic_analyzer.hh"
#include "types/parser/ast/struct.hh"
#include "types/parser/ast/class.hh"
#include "types/parser/ast/union.hh"
#include <map>
#include <stdexcept>

namespace baka::ir {
    // One instance lowers one checked program. The result owns its data; only
    // this builder borrows frontend tables. No mutable frontend API is used.
    class Lowerer {
        const semantic::SemanticAnalyzer& Analyzer;
        Module Result;
        std::vector<Diagnostic> Diagnostics;
        std::map<uint32_t, TypeId> TypeMap;

        struct MemberBinding {
            uint32_t owner = 0, index = 0;
            bool isStatic = false;
        };

        std::map<uint32_t, MemberBinding> Members;
        size_t FunctionIndex = 0;
        BlockId CurrentBlock = 0, NextBlock = 1;
        uint32_t NextObject = 1, Self = 0;
        std::map<uint32_t, BlockId> Breaks, Continues, Labels;
        std::map<const types::ASTNode*, BlockId> Cases;
        std::vector<Operand> SavedTargets;
        std::vector<const types::SingleDeclarationNode*> GlobalInitializers;
        SourceSpan Source;
        bool Started = false, BuildingExpression = false;

        struct ExpressionRegion {
            BlockId entry = 0, exit = 0;
            Operand value;
            bool connected = false;
        };

        std::map<const types::ExpressionNode*, ExpressionRegion> ExpressionRegions;
        void lowerExpressionRegion(const types::ExpressionNode* expression);
        void collectExpressionRegions(const types::ASTNode* node);

        struct Failure : std::runtime_error {
            using std::runtime_error::runtime_error;
        };

        [[noreturn]] void fail(const types::ASTNode* node, std::string message);
        TypeId importType(semantic::TypeId id);
        TypeId type(const types::ASTNode* node);
        TypeId integerType() const;
        Function& function();
        Block& block();
        BlockId newBlock();
        void select(BlockId id);
        Operand temporary(TypeId type);
        Operand integer(TypeId type, uint64_t value) const;
        Operand label(BlockId id) const;
        Operand object(uint32_t symbol);
        Operand callable(uint32_t symbol);
        Operand emitValue(Opcode opcode, TypeId type, Operand left, Operand right = {});
        void emit(Opcode opcode, Operand result = {}, Operand left = {}, Operand right = {});
        void jump(BlockId destination);
        void branch(Operand condition, BlockId yes, BlockId no);
        void link(BlockId destination);
        Operand cast(Operand value, TypeId target);
        Operand load(Operand address, TypeId target);
        Operand value(const types::ExpressionNode* expression);
        Operand address(const types::ExpressionNode* expression);
        Operand memberAddress(const types::ASTNode* node, Operand receiver, semantic::TypeId receiverType);
        Operand self();
        Operand receiver(const types::ExpressionNode* callee, uint32_t owner);
        Operand constant(const types::ConstantNode* node, TypeId target);
        Operand binary(types::ASTBinaryOp opcode, Operand left, Operand right, TypeId result);
        Operand increment(const types::ExpressionNode* target, bool decrement, bool postfix,
                          bool returnAddress = false);
        Operand call(const types::FunctionCallPostfixExpr* node);
        void node(const types::ASTNode* node);
        void statement(const types::StatementNode* statement);
        void declarations(const types::DeclarationList* declarations, bool global = false);
        void initialize(Operand destination, TypeId target, const types::ExpressionNode* initializer,
                        bool zero = false);
        void lowerGlobalInitializers();
        void initializeStatic(uint32_t symbol, const types::ExpressionNode* initializer);

        struct InitializerEntry {
            uint64_t offset;
            TypeId type;
            const types::ExpressionNode* expression;
        };

        std::vector<std::pair<TypeId, uint64_t>> subobjects(TypeId type);
        std::vector<InitializerEntry> initializerPlan(TypeId type, const types::ExpressionNode* initializer);
        void collectCases(const types::StatementNode* statement,
                          std::vector<const types::CaseLabelStatementNode*>& cases,
                          const types::DefaultLabelStatementNode*& fallback);
        void lowerFunction(const types::FunctionNode* node);
        void collect(const types::ASTNode* node);
        void collectBody(const types::StructBodyNode* body);
        void importSymbols();
        bool containsLifetime(semantic::TypeId type) const;

    public:
        explicit Lowerer(const semantic::SemanticAnalyzer& analyzer) : Analyzer(analyzer) {}

        bool lower(const types::ProgramNode& program, bool optimize = false);

        const Module& module() const {
            return Result;
        }

        const std::vector<Diagnostic>& diagnostics() const {
            return Diagnostics;
        }

        Module takeModule() {
            return std::move(Result);
        }
    };
}
