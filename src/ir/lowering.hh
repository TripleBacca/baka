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
        semantic::TypeId CurrentOwner;
        Operand SelfOverride;
        bool InDestructor = false;

        // Cleanup has two independent facts: a lexical owner/destination and a
        // runtime flag saying construction completed on this execution path.
        // The lexical stack selects which objects a jump leaves; flags make
        // cleanup safe for conditional paths and repeated loop executions.

        struct Cleanup {
            Operand address;
            TypeId type = 0;
            Operand active;
        };

        struct LifetimeWork {
            uint32_t function = 0;
            TypeId type = 0;
            const types::ASTNode* declaration = nullptr;
            const types::StatementNode* body = nullptr;
            bool destructor = false;
        };

        std::vector<LifetimeWork> LifetimeBodies;
        std::map<std::pair<uint32_t, uint32_t>, uint32_t> SpecialFunctions;
        std::map<TypeId, uint32_t> DefaultConstructors, Destructors;
        std::map<uint32_t, Cleanup> LocalCleanups;
        std::vector<uint32_t> ActiveCleanups;
        std::map<uint32_t, std::vector<uint32_t>> LabelCleanups;
        std::map<const types::ASTNode*, std::vector<uint32_t>> CaseCleanups;
        std::map<uint32_t, size_t> BreakDepths, ContinueDepths;
        // Per-full-expression temporaries are emitted with their expression
        // region, but destroyed only after its enclosing store/condition/use.
        // ValueOwners permits transferring a prvalue to a return or parameter
        // without also destroying it in its original full-expression cleanup.
        std::vector<Cleanup> BuildingTemps, PendingTemps;
        std::map<uint32_t, std::vector<Cleanup>> ValueOwners;
        std::map<uint32_t, Operand> ValueLocations;

        struct Destination {
            uint32_t object = 0;
            uint64_t offset = 0;
            TypeId type = 0;
        };

        std::map<const types::ExpressionNode*, Destination> Destinations;
        // Inline initialization (constructor members and module startup) can
        // already have a computed destination pointer instead of a source ID.
        std::map<const types::ExpressionNode*, Operand> DirectDestinations;
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
            std::vector<Cleanup> cleanups;
        };

        std::map<const types::ExpressionNode*, std::vector<ExpressionRegion>> ExpressionRegions;
        // Default member AST nodes are shared between construction sites. Each
        // use gets a separate region; AST pointer identity is not evaluation ID.
        std::map<const types::ExpressionNode*, size_t> RegionCursor;
        void lowerExpressionRegion(const types::ExpressionNode* expression, Destination receiver = {0, 0, 0});
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
            TypeId owner = 0;
            uint64_t receiverOffset = 0;
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
        semantic::TypeId semanticType(TypeId type) const;
        bool needsConstruction(TypeId type) const;
        bool needsDestruction(TypeId type) const;
        Operand storage(TypeId type, std::string name);
        Operand activeFlag();
        void importLifetimes();
        void lowerLifetimes();
        void lowerCallable(const types::ASTNode* declaration, const types::StatementNode* body, uint32_t symbol,
                           semantic::TypeId owner = {}, bool constructor = false, bool destructor = false);
        void construct(Operand destination, TypeId type, uint32_t selected = 0,
                       std::vector<const types::ExpressionNode*> arguments = {});
        void initializeMembers(Operand destination, TypeId type);
        void zeroObject(Operand destination, TypeId type);
        void destroy(Operand destination, TypeId type);
        void destroyMembers(Operand destination, TypeId type);
        Operand invoke(uint32_t function, const std::vector<Operand>& arguments);
        void registerDestructor(Operand destination, TypeId type);
        void cleanup(const Cleanup& entry);
        void cleanupTo(size_t depth);
        void finishTemps();
        Operand ownTemporary(Operand value, Operand location = {});
        void transfer(Operand value);
        void planCleanupTargets(const types::ASTNode* node, std::vector<uint32_t> active);
        const types::ExpressionNode* valueRoot(const types::ExpressionNode* expression) const;
        Operand destination(const types::ExpressionNode* expression);

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
