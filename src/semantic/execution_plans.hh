#pragma once
#include "types.hh"
#include <map>
#include <set>
#include <tuple>
#include <string>

namespace baka {
    namespace types {
        class ASTNode;
        class DeclarationList;
        class StatementNode;
        class ExpressionNode;
    }
}

namespace baka {
    namespace semantic {
        class Symbols;
        class ExpressionChecker;

        struct ConstructorSelection {
            TypeId owner, signature;
            uint32_t member = 0;
            bool available = true;
            std::string failure;
            std::vector<const types::ExpressionNode*> arguments;
        };

        struct SubobjectPlan {
            TypeId type;
            uint64_t offset = 0;
            const types::ExpressionNode* initializer = nullptr;
        };

        struct LifetimePlan {
            bool construction = false, destruction = false;
            ConstructorSelection defaultConstructor;
            std::vector<SubobjectPlan> children;
        };
        enum class InitializationKind { Value, String, Zero, Construct };

        struct InitializationStep {
            uint64_t offset = 0;
            TypeId type;
            const types::ExpressionNode* expression = nullptr;
            TypeId receiver;
            uint64_t receiverOffset = 0;
            InitializationKind kind = InitializationKind::Value;
            std::string bytes;
            uint64_t byteCount = 0, elements = 0;
        };

        struct InitializationPlan {
            std::vector<InitializationStep> steps;
            std::string failure;
        };

        struct CallPlan {
            TypeId signature, owner;
            SymbolId symbol;
            uint32_t member = 0;
            bool construction = false;
            std::vector<const types::ExpressionNode*> arguments;
        };

        struct CleanupTarget {
            std::vector<SymbolId> active;
        };

        // Published by annotation. All lowering queries are const and consume
        // decisions made against the same type/symbol arenas as the checked AST.
        class ExecutionPlans {
            const Types& TypesView;
            const Symbols& SymbolsView;
            using InitKey = std::tuple<uint32_t, const types::ExpressionNode*, bool>;
            std::map<uint32_t, LifetimePlan> Lifetimes;
            std::map<InitKey, InitializationPlan> Initializers;
            std::map<const types::ExpressionNode*, CallPlan> Calls;
            std::map<uint32_t, CleanupTarget> Labels;
            std::map<const types::ASTNode*, CleanupTarget> Cases;
            std::map<const types::StatementNode*, std::vector<SymbolId>> Exits;
            std::map<uint32_t, std::vector<SymbolId>> BreakTargets, ContinueTargets;
            std::vector<std::pair<const types::StatementNode*, std::vector<SymbolId>>> PendingGotos;
            std::map<uint32_t, std::vector<SymbolId>> Locals;
            std::set<std::pair<uint32_t, uint32_t>> RequiredConstructors;
            std::set<uint32_t> RequiredDefaults;
            std::vector<std::pair<const types::ASTNode*, std::string>> Errors;
            LifetimePlan& buildLifetime(TypeId type, const ExpressionChecker& checker);
            InitializationPlan& buildInitialization(TypeId type, const types::ExpressionNode* expression, bool zero);
            void appendInitialization(InitializationPlan& plan, TypeId type, uint64_t offset,
                                      const types::ExpressionNode* expression, InitializationKind kind);
            void initializeObject(InitializationPlan& plan, TypeId type, uint64_t offset,
                                  const types::ExpressionNode* expression);
            void consumeInitializer(InitializationPlan& plan, TypeId type, uint64_t offset,
                                    const std::vector<types::ExpressionNode*>& expressions, size_t& position);
            void initializeElements(InitializationPlan& plan, TypeId type, uint64_t offset,
                                    const std::vector<types::ExpressionNode*>& expressions, size_t& position);
            void initializeMembers(InitializationPlan& plan, TypeId type, uint64_t offset,
                                   const std::vector<types::ExpressionNode*>& expressions, size_t& position);
            void requireDefault(TypeId type, const types::ASTNode* source);
            void requireMembers(TypeId type, const types::ASTNode* source);
            void validate(const InitializationPlan& plan, const types::ASTNode* source);
            void exit(const types::StatementNode* source, const std::vector<SymbolId>& active,
                      const std::vector<SymbolId>& retained);
            void appendObjects(const types::DeclarationList* declarations, std::vector<SymbolId>& active) const;

        public:
            ExecutionPlans(const Types& types, const Symbols& symbols) : TypesView(types), SymbolsView(symbols) {}

            void prepare(const ExpressionChecker& checker);
            void addInitialization(TypeId type, const types::ExpressionNode* expression, const types::ASTNode* source);
            void addExpression(const types::ExpressionNode* expression);
            void addBody(const types::StatementNode* body, std::vector<SymbolId> active = {});
            void finish();

            const std::vector<SymbolId>& cleanup(const types::StatementNode* source) const {
                return Exits.at(source);
            }

            const LifetimePlan& lifetime(TypeId type) const;
            const InitializationPlan& initialization(TypeId type, const types::ExpressionNode* expression,
                                                     bool zero = false) const;
            const CallPlan& call(const types::ExpressionNode* expression) const;

            const CleanupTarget& label(uint32_t target) const {
                return Labels.at(target);
            }

            const CleanupTarget& caseTarget(const types::ASTNode* node) const {
                return Cases.at(node);
            }

            const std::vector<SymbolId>& locals(ScopeId function) const;
            bool requiresConstructor(TypeId type, uint32_t member) const;

            const auto& errors() const {
                return Errors;
            }
        };
    }
}
