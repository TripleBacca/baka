#pragma once
#include "ir.hh"
#include "semantic/semantic_analyzer.hh"
#include "types/parser/ast/struct.hh"
#include "types/parser/ast/class.hh"
#include "types/parser/ast/union.hh"
#include <map>
#include <stdexcept>

namespace baka {
    namespace ir {
        // One instance lowers one checked program. The result owns its data; only
        // this builder borrows frontend tables. No mutable frontend API is used.
        class Lowerer {
            const semantic::SemanticAnalyzer& Analyzer;
            const types::ProgramNode* BoundProgram = nullptr;
            Module Result;
            std::vector<Diagnostic> Diagnostics;
            std::map<uint32_t, TypeId> TypeMap;

            struct MemberBinding {
                uint32_t owner = 0, index = 0;
                bool isStatic = false;
            };

            std::map<uint32_t, MemberBinding> Members;

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

            struct FunctionContext {
                size_t FunctionIndex = 0;
                BlockId CurrentBlock = 0, NextBlock = 1;
                uint32_t Self = 0;
                semantic::TypeId CurrentOwner;
                Operand SelfOverride;
                bool InDestructor = false;
                SourceSpan Source;
                std::map<uint32_t, Cleanup> LocalCleanups;
                std::vector<uint32_t> ActiveCleanups;
                std::vector<Cleanup> PendingTemps;
                std::map<uint32_t, std::vector<Cleanup>> ValueOwners;
                std::map<uint32_t, Operand> ValueLocations;
                std::map<const types::ExpressionNode*, Operand> DirectDestinations;
                std::map<uint32_t, BlockId> Breaks, Continues, Labels;
                std::map<const types::ASTNode*, BlockId> Cases;
                std::vector<Operand> SavedTargets;
            } State;

            uint32_t NextObject = 1;
            uint32_t HeapAllocate = 0, HeapDeallocate = 0;
            bool Started = false;
            std::vector<const types::SingleDeclarationNode*> GlobalInitializers;
            std::map<TypeId, semantic::TypeId> ReverseTypes;

            struct Failure : std::runtime_error {
                using std::runtime_error::runtime_error;
            };

            [[noreturn]] void fail(const types::ASTNode* node, std::string message);

            // Import frontend identities and build instructions in the current function.
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
            void emit(Instruction instruction);
            void emitCall(Operand result, Operand callee, std::vector<Operand> arguments);
            void store(Operand address, Operand value);
            void jump(BlockId destination);
            void branch(Operand condition, BlockId yes, BlockId no);
            void link(BlockId destination);
            Operand cast(Operand value, TypeId target);
            Operand load(Operand address, TypeId target);

            // Expression values and locations retain the source evaluation order.
            Operand value(const types::ExpressionNode* expression);
            Operand primaryValue(const types::PrimaryExpressionNode* expression);
            Operand memberValue(const types::ExpressionNode* expression);
            Operand unaryValue(const types::FactorNode* expression);
            Operand binaryValue(const types::BinaryExpressionNode* expression);
            Operand conditionalValue(const types::TernaryOpNode* expression);
            Operand implicitValue(const types::ImplicitCastNode* expression);
            Operand castValue(const types::CastFactorNode* expression);
            Operand compoundAssignment(const types::CompoundAssignmentNode* expression, bool returnAddress);
            Operand address(const types::ExpressionNode* expression);
            Operand primaryAddress(const types::PrimaryExpressionNode* expression);
            Operand indexAddress(const types::IndexPostfixExpr* expression);
            Operand conditionalAddress(const types::TernaryOpNode* expression);
            Operand memberAddress(const types::ASTNode* node, Operand receiver, semantic::TypeId receiverType);
            Operand self();
            Operand receiver(const types::ExpressionNode* callee, uint32_t owner);
            Operand constant(const types::ConstantNode* node, TypeId target);
            Operand binary(types::ASTBinaryOp opcode, Operand left, Operand right, TypeId result);
            Operand increment(const types::ExpressionNode* target, bool decrement, bool postfix,
                              bool returnAddress = false);
            Operand call(const types::FunctionCallPostfixExpr* node);
            Operand heapCall(const types::FunctionCallPostfixExpr* node, bool allocation);
            uint32_t heapFunction(bool allocation);
            Operand constructionValue(const types::ExpressionNode* node,
                                      const std::vector<const types::ExpressionNode*>& arguments);

            // Structured statements and the cleanup obligations on their exits.
            void node(const types::ASTNode* node);
            void statement(const types::StatementNode* statement);
            void compoundStatement(const types::CompoundStatementNode* statement);
            void ifStatement(const types::IfSuperBlockStatementNode* statement);
            void switchStatement(const types::SwitchBlockStatementNode* statement);
            void caseStatement(const types::CaseLabelStatementNode* statement);
            void defaultStatement(const types::DefaultLabelStatementNode* statement);
            void forStatement(const types::ForBlockStatementNode* statement);
            void whileStatement(const types::WhileBlockStatementNode* statement);
            void doWhileStatement(const types::DoWhileBlockStatementNode* statement);
            void returnStatement(const types::ReturnStatementNode* statement);
            void gotoStatement(const types::GotoStatementNode* statement);
            void breakStatement(const types::BreakStatementNode* statement);
            void continueStatement(const types::ContinueStatementNode* statement);
            void gotoLabel(const types::GotoLabelStatementNode* statement);
            void cleanupExit(const types::StatementNode* statement);

            // Storage initialization consumes the semantic subobject plans.
            void declarations(const types::DeclarationList* declarations, bool global = false);
            void initialize(Operand destination, const semantic::InitializationPlan& plan);
            void lowerGlobalInitializers();
            void initializeStatic(uint32_t symbol, const types::ExpressionNode* initializer);
            std::vector<std::pair<TypeId, uint64_t>> subobjects(TypeId type);

            // Function collection and special-member emission.
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
            uint32_t declareLifetime(TypeId owner, std::string name, std::vector<TypeId> parameters,
                                     const types::ASTNode* declaration, const types::StatementNode* body,
                                     bool destructor, bool variadic = false);
            void importLifetimes();
            void lowerLifetimes();
            void lowerCallable(const types::ASTNode* declaration, const types::StatementNode* body, uint32_t symbol,
                               semantic::TypeId owner = {}, bool constructor = false, bool destructor = false);
            void construct(Operand destination, TypeId type, uint32_t selected = 0,
                           std::vector<const types::ExpressionNode*> arguments = {}, uint64_t elements = 0);
            void initializeMembers(Operand destination, TypeId type);
            void zeroObject(Operand destination, TypeId type, uint64_t bytes = 0);
            void forEachElement(Operand destination, TypeId array, uint64_t count, bool reverse,
                                const std::function<void(Operand)>& emitElement);
            void destroy(Operand destination, TypeId type);
            void destroyMembers(Operand destination, TypeId type);
            Operand invoke(uint32_t function, const std::vector<Operand>& arguments);
            void registerDestructor(Operand destination, TypeId type);
            void cleanup(const Cleanup& entry);
            void cleanupTo(size_t depth);
            void finishTemps();
            Operand ownTemporary(Operand value, Operand location = {});
            void transfer(Operand value);
            const types::ExpressionNode* valueRoot(const types::ExpressionNode* expression) const;
            Operand destination(const types::ExpressionNode* expression);

        public:
            explicit Lowerer(const semantic::SemanticAnalyzer& analyzer) : Analyzer(analyzer) {}

            explicit Lowerer(semantic::CheckedProgram program)
                : Analyzer(program.analyzer()), BoundProgram(&program.program()) {}

            bool lower() {
                if (!BoundProgram) {
                    Diagnostics.push_back({"IR lowering requires a checked program view", {}});
                    return false;
                }
                return lower(*BoundProgram);
            }

            bool lower(const types::ProgramNode& program);

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
}
