#include "semantic_analyzer.hh"

namespace baka::semantic {
    SemanticAnalyzer::SemanticAnalyzer()
        : SymbolTable(TypeSystem), DeclarationChecker(TypeSystem, SymbolTable),
          ASTResolver(TypeSystem, SymbolTable, DeclarationChecker),
          Checker(TypeSystem, SymbolTable, DeclarationChecker, ASTResolver),
          Aggregates(TypeSystem, SymbolTable, DeclarationChecker, ASTResolver) {
        ASTResolver.setEvaluator(&Checker);
        // Bounds and enumerators are declaration-time dependencies. Resolve their
        // operands in the live lexical environment before evaluating them, rather
        // than deferring lookup until scopes have acquired later declarations.
        ASTResolver.setExpressionResolver([this](types::ExpressionNode* expression) {
            resolveExpression(expression);
            if (expression && DeclarationExpressions.insert(expression).second)
                queue(expression, expression);
        });
        SymbolTable.installTypes();
        SymbolTable.installLibrary();
        Current.scope = SymbolTable.currentScope();
    }

    void SemanticAnalyzer::select(const Context& context) {
        Current = context;
        SymbolTable.selectScope(context.scope);
        SymbolTable.setGlobalVisibility(context.globalLimit);
        Checker.setFunctionContext(context.owner, context.isStatic, context.isConst);
    }
    void SemanticAnalyzer::refreshDiagnostics() { Diagnostics = SymbolTable.diagnostics(); }
    void SemanticAnalyzer::error(std::string code, std::string message, const types::ASTNode* source) {
        SymbolTable.diagnose(std::move(code), std::move(message), source);
    }
    bool SemanticAnalyzer::analyze(types::ProgramNode& program) {
        // Invalid top-level signatures/types can invalidate later work entirely.
        // Once registration succeeds, local resolution/type errors are recoverable:
        // error-valued expressions suppress cascades, while independent work still
        // receives diagnostics (e.g. a bad call after a duplicate local).
        if (!registerDeclarations(program)) return false;
        resolveAndRename();
        annotateTypes();
        check();
        return Diagnostics.empty();
    }
}
