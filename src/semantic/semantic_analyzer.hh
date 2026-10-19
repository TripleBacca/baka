#pragma once

#include "expressions.hh"
#include "types/parser/ast/program.hh"
#include "types/parser/ast/selectionStatement.hh"
#include "types/parser/ast/iterationStatement.hh"
#include "types/parser/ast/labelStatement.hh"
#include "types/parser/ast/jump.hh"
#include "types/parser/ast/statement.hh"
#include <limits>
#include <unordered_set>

namespace baka::semantic {
    // The public phase boundaries are intentional: tests and future lowering can
    // inspect the mutated tree between passes without rerunning the parser.
    enum class SemanticPhase { Parsed, Registered, Resolved, Annotated, Checked };

    class SemanticAnalyzer {
        Types TypeSystem;
        Symbols SymbolTable;
        Declarations DeclarationChecker;
        ASTDeclarations ASTResolver;
        ExpressionChecker Checker;
        ASTAggregates Aggregates;
        SemanticPhase Phase = SemanticPhase::Parsed;
        std::vector<Diagnostic> Diagnostics;

        struct Context {
            ScopeId scope;
            TypeId owner, result;
            uint32_t globalLimit = std::numeric_limits<uint32_t>::max();
            bool isStatic = false, isConst = false;
        };
        Context Current;
        enum class ExpressionUse { Value, Condition, Initializer, Default, Return, Switch, Case };
        struct ExpressionWork {
            types::ExpressionNode* expression = nullptr;
            const types::ASTNode* source = nullptr;
            Context context;
            ExpressionUse use = ExpressionUse::Value;
            SymbolId symbol;
            TypeId expected;
            bool staticStorage = false;
            ExprResult value;
        };
        struct BodyWork {
            types::ASTNode* declaration = nullptr;
            types::StatementNode* body = nullptr;
            types::FunctionParameterList* parameters = nullptr;
            Context context;
            TypeId signature;
            SymbolId symbol;
        };
        struct JumpWork {
            types::StatementNode* statement = nullptr;
            const types::StatementNode* target = nullptr;
            std::vector<SymbolId> initialized;
        };
        struct LabelWork {
            types::GotoLabelStatementNode* label = nullptr;
            std::vector<SymbolId> initialized;
        };
        struct FunctionFlow {
            std::vector<JumpWork> jumps;
            std::vector<LabelWork> labels;
        };
        struct SwitchWork {
            const types::SwitchBlockStatementNode* statement = nullptr;
            size_t condition = 0;
            std::vector<size_t> cases;
            std::vector<const types::DefaultLabelStatementNode*> defaults;
        };
        // Registration collects these once. Later phases consume the queues, not
        // repeated DFS traversals from ProgramNode. Resolution visits each body
        // in lexical order and appends contextual expression/control work.
        std::vector<types::ASTNode*> TopLevel;
        std::vector<BodyWork> Bodies;
        std::vector<ExpressionWork> Expressions;
        std::unordered_set<const types::ExpressionNode*> DeclarationExpressions;
        std::vector<FunctionFlow> Flows;
        std::vector<SwitchWork> Switches;
        std::vector<const types::ASTNode*> InvalidNodes;
        std::vector<std::pair<const types::ASTNode*, std::string>> ResolutionErrors;
        std::vector<const types::StatementNode*> ActiveTargets;
        std::vector<std::string> ActiveLoopNames;
        std::vector<size_t> ActiveSwitches;
        std::vector<SymbolId> Initialized;
        size_t CurrentFlow = 0;
        uint32_t NextTarget = 1;

        void select(const Context& context);
        void refreshDiagnostics();
        void error(std::string code, std::string message, const types::ASTNode* source);
        void registerNode(types::ASTNode* node);
        void registerFunction(types::FunctionNode* function, TypeId owner = {});
        void registerMembers(types::StructBodyNode* body, TypeId owner);
        void registerDefaults(types::FunctionParameterList* parameters, TypeId signature,
                              SymbolId symbol, const types::ASTNode* source);
        void registerDeclarations(types::DeclarationList* list);
        void registerLocal(types::DeclarationList* list);
        void resolveBody(BodyWork& body);
        void resolveStatement(types::StatementNode* statement, bool outerBody = false);
        void resolveExpression(types::ExpressionNode* expression);
        void resolveInitializer(types::ExpressionNode* expression);
        void resolveTypeOperand(types::TypeNode* type, types::DeclarationIdentifierNode* declarator);
        size_t queue(types::ExpressionNode* expression, const types::ASTNode* source,
                     ExpressionUse use = ExpressionUse::Value, SymbolId symbol = {},
                     TypeId expected = {}, bool staticStorage = false);
        std::vector<Diagnostic> checkReadOnly() const;

    public:
        SemanticAnalyzer();
        bool registerDeclarations(types::ProgramNode& program);
        bool resolveAndRename();
        bool annotateTypes();
        bool check();
        bool analyze(types::ProgramNode& program);
        SemanticPhase phase() const { return Phase; }
        const std::vector<Diagnostic>& diagnostics() const { return Diagnostics; }
        const Types& types() const { return TypeSystem; }
        const Symbols& symbols() const { return SymbolTable; }
        size_t topLevelWorkCount() const { return TopLevel.size(); }
        size_t bodyWorkCount() const { return Bodies.size(); }
        size_t expressionWorkCount() const { return Expressions.size(); }
    };
}
