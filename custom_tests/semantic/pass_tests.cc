// Structural tests for phase boundaries. End-to-end diagnostics alone cannot
// prove that resolution mutated the AST or that the final checker is read-only.
#include "semantic/semantic_analyzer.hh"
#include "lexer/lexer.hh"
#include "parser/parser.hh"
#include "driver/gctx.hh"
#include <iostream>
#include <stdexcept>
#include <sstream>

using namespace baka;
using namespace baka::semantic;

namespace {
    void require(bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    }
    std::string printed(types::ASTNode* node) {
        std::ostringstream output;
        auto* previous = std::cout.rdbuf(output.rdbuf());
        node->Print();
        std::cout.rdbuf(previous);
        return output.str();
    }
    types::ExpressionNode* withoutConversions(types::ExpressionNode* expression) {
        while (auto* cast = dynamic_cast<types::ImplicitCastNode*>(expression)) expression = cast->getOperand();
        return expression;
    }
    types::FunctionNode* function(types::ProgramNode* program, size_t index) {
        return dynamic_cast<types::FunctionNode*>(program->getBody().at(index));
    }
    types::CompoundStatementNode* body(types::FunctionNode* node) {
        return dynamic_cast<types::CompoundStatementNode*>(node->getBody());
    }
    types::SingleDeclarationNode* declaration(types::CompoundStatementNode* block, size_t index) {
        return std::get<types::DeclarationList*>(block->getStatements().at(index))->getDeclarations().front();
    }
}

int main(int argc, char** argv) {
    try {
        require(argc == 2, "expected phase scenario");
        const std::string scenario = argv[1];
        const bool negative = scenario.ends_with("_error");
        std::string source =
            "int helper(int x){return x;} "
            "int main(int argc,char **argv){int n=1;{int n=2;n=n+1;}"
            "while(n;outer){break outer;}return helper(n);}";
        if (scenario == "annotation_scoped_type") source =
            "typedef int T;int main(){int a=sizeof(T);{typedef char T;int b=sizeof(T);}return a;}";
        if (scenario == "resolution_loop_targets") source =
            "int main(){for(;;;a){for(;;;b){continue a;continue;break a;break;"
            "switch(0){default:continue a;}}}goto done;done:return 0;}";
        if (scenario == "resolution_this") source = "struct S{S *self(){return this;}};";
        if (negative && scenario.starts_with("registration")) source = "int f(){return 1;}int f(){return 2;}";
        if (negative && scenario.starts_with("resolution")) source = "int main(){int a=b,b=1;return a;}";
        if (negative && scenario.starts_with("annotation")) source = "int main(){return 1.5 & 2;}";
        if (negative && scenario.starts_with("checks")) source = "int main(){break;return 0;}";
        driver::Gctx::AttachLineIndex(std::make_shared<base::LineIndex>(source));
        auto tokens = lexer::Tokenize(source);
        parser::Parser parser(tokens.Tokens, tokens.Locations);
        auto* program = dynamic_cast<types::ProgramNode*>(parser.Parse());
        require(program && !driver::Gctx::ErrorFound(), "test input did not parse");
        if (scenario == "resolution_loop_targets")
            require(printed(program).find("Target:") == std::string::npos, "parser output contains unresolved semantic targets");
        SemanticAnalyzer analyzer;
        bool ok = analyzer.registerDeclarations(*program);
        require(analyzer.phase() == SemanticPhase::Registered, "registration phase missing");
        if (scenario.starts_with("registration")) {
            require(ok != negative, "registration result mismatch");
            if (!negative) {
                require(analyzer.topLevelWorkCount() == 2 && analyzer.bodyWorkCount() == 2, "bodies were not collected once");
                require(function(program, 0)->semanticInfo().Symbol != 0, "function was not registered on AST");
                auto* local = declaration(body(function(program, 1)), 0);
                require(!local->semanticInfo().Resolved, "registration unexpectedly traversed a function body");
            }
            return 0;
        }
        require(ok, "registration failed before requested phase");
        ok = analyzer.resolveAndRename();
        require(analyzer.phase() == SemanticPhase::Resolved, "resolution phase missing");
        if (scenario.starts_with("resolution")) {
            require(ok != negative, "resolution result mismatch");
            if (scenario == "resolution_loop_targets") {
                auto* block = body(function(program, 0));
                auto* outer = dynamic_cast<types::ForBlockStatementNode*>(std::get<types::StatementNode*>(block->getStatements()[0]));
                auto* outerBody = dynamic_cast<types::CompoundStatementNode*>(outer->getBody());
                auto* inner = dynamic_cast<types::ForBlockStatementNode*>(std::get<types::StatementNode*>(outerBody->getStatements()[0]));
                auto* innerBody = dynamic_cast<types::CompoundStatementNode*>(inner->getBody());
                auto* named = dynamic_cast<types::ContinueStatementNode*>(std::get<types::StatementNode*>(innerBody->getStatements()[0]));
                auto outerId = outer->semanticInfo().Target, innerId = inner->semanticInfo().Target;
                require(outerId && innerId && outerId != innerId, "nested loops do not have unique control IDs");
                require(named->semanticInfo().Target == outerId && named->getLabel()->semanticInfo().Target == outerId &&
                        named->getLabel()->semanticInfo().Resolved, "continue a did not bind to the outer for on both nodes");
                require(std::get<types::StatementNode*>(innerBody->getStatements()[1])->semanticInfo().Target == innerId,
                        "unnamed continue did not bind to the inner loop");
                require(std::get<types::StatementNode*>(innerBody->getStatements()[2])->semanticInfo().Target == outerId &&
                        std::get<types::StatementNode*>(innerBody->getStatements()[3])->semanticInfo().Target == innerId,
                        "named/unnamed break targets are wrong");
                auto* selection = dynamic_cast<types::SwitchBlockStatementNode*>(std::get<types::StatementNode*>(innerBody->getStatements()[4]));
                auto* switchBody = dynamic_cast<types::CompoundStatementNode*>(selection->getBody());
                auto* defaultLabel = dynamic_cast<types::DefaultLabelStatementNode*>(std::get<types::StatementNode*>(switchBody->getStatements()[0]));
                require(defaultLabel->getStatement()->semanticInfo().Target == outerId,
                        "switch nesting changed the named continue target");
                auto* go = dynamic_cast<types::GotoStatementNode*>(std::get<types::StatementNode*>(block->getStatements()[1]));
                auto* label = dynamic_cast<types::GotoLabelStatementNode*>(std::get<types::StatementNode*>(block->getStatements()[2]));
                require(go->semanticInfo().Target && go->semanticInfo().Target == label->semanticInfo().Target &&
                        go->getLabel()->semanticInfo().Target == label->getTag()->semanticInfo().Target,
                        "forward goto target metadata was not propagated to label identifiers");
                require(printed(named).find("Target: " + std::to_string(outerId)) != std::string::npos,
                        "continue printer hides its resolved target");
                require(printed(outer).starts_with("For(\n    Target: " + std::to_string(outerId) + "\n"),
                        "for printer hides the matching loop identity");
                require(analyzer.annotateTypes() && analyzer.check(), "later passes rejected nested loop controls");
                require(named->semanticInfo().Target == outerId && inner->semanticInfo().Target == innerId,
                        "later passes changed resolved control IDs");
                return 0;
            }
            if (scenario == "resolution_this") {
                auto* record = dynamic_cast<types::StructDefinitionNode*>(program->getBody().front());
                auto* method = record->getBody()->getFunctions().front();
                auto* result = dynamic_cast<types::ReturnStatementNode*>(std::get<types::StatementNode*>(body(method)->getStatements().front()));
                auto* comma = dynamic_cast<types::CommaExpressionNode*>(result->getExpression());
                auto* primary = dynamic_cast<types::PrimaryExpressionNode*>(comma->getExpressions().front());
                auto* self = std::get<types::ThisNode*>(primary->getExpression());
                require(self->semanticInfo().Symbol != 0, "this was not registered and bound in the method scope");
                require(analyzer.symbols().get({self->semanticInfo().Symbol}).name == "this", "this binding identifies the wrong symbol");
                require(analyzer.annotateTypes() && self->semanticInfo().Annotated && self->semanticInfo().Type != 0,
                        "this was not annotated with its implicit-object pointer type");
                return 0;
            }
            if (!negative) {
                auto* block = body(function(program, 1));
                auto* outer = declaration(block, 0)->getDeclarator()->getIdentifier();
                auto* innerBlock = dynamic_cast<types::CompoundStatementNode*>(std::get<types::StatementNode*>(block->getStatements()[1]));
                auto* inner = declaration(innerBlock, 0)->getDeclarator()->getIdentifier();
                require(outer->getResolvedName() != inner->getResolvedName(), "shadowed declarations have the same renamed identity");
                auto* result = dynamic_cast<types::ReturnStatementNode*>(std::get<types::StatementNode*>(block->getStatements().back()));
                require(result->getExpression()->semanticInfo().Resolved && !result->getExpression()->semanticInfo().Annotated,
                        "resolution did not leave expression typing to the annotation pass");
            }
            return 0;
        }
        require(ok, "resolution failed before requested phase");
        ok = analyzer.annotateTypes();
        require(analyzer.phase() == SemanticPhase::Annotated, "annotation phase missing");
        if (scenario.starts_with("annotation")) {
            require(ok != negative, "annotation result mismatch");
            if (scenario == "annotation_scoped_type") {
                auto* block = body(function(program, 1));
                auto* outer = declaration(block, 0)->getInitializer();
                auto* nested = dynamic_cast<types::CompoundStatementNode*>(std::get<types::StatementNode*>(block->getStatements()[1]));
                auto* inner = std::get<types::DeclarationList*>(nested->getStatements()[1])->getDeclarations().front()->getInitializer();
                auto* outerSize = dynamic_cast<types::SizeofNode*>(withoutConversions(dynamic_cast<types::InitializerNode*>(outer)->expressions().front()));
                auto* innerSize = dynamic_cast<types::SizeofNode*>(withoutConversions(dynamic_cast<types::InitializerNode*>(inner)->expressions().front()));
                auto outerType = std::get<types::SizeofNode::SizeofTypeVariant>(outerSize->getPayload()).Type->semanticInfo().Type;
                auto innerType = std::get<types::SizeofNode::SizeofTypeVariant>(innerSize->getPayload()).Type->semanticInfo().Type;
                require(outerType == analyzer.types().builtin(Builtin::Int).value &&
                        innerType == analyzer.types().builtin(Builtin::Char).value,
                        "scoped typedef identities were not attached at their original use sites");
                require(outerSize->semanticInfo().Constant == 4 && innerSize->semanticInfo().Constant == 1,
                        "sizeof re-resolved a shadowed type in the wrong scope");
                return 0;
            }
            if (!negative) {
                auto* result = dynamic_cast<types::ReturnStatementNode*>(std::get<types::StatementNode*>(body(function(program, 0))->getStatements().front()));
                const auto& metadata = result->getExpression()->semanticInfo();
                require(metadata.Annotated && metadata.Type == analyzer.types().builtin(Builtin::Int).value,
                        "expression type was not written onto AST");
                require(analyzer.expressionWorkCount() != 0, "annotation did not use contextual work items");
            }
            return 0;
        }
        require(ok, "annotation failed before requested phase");
        auto* mainBody = body(function(program, negative ? 0 : 1));
        auto* finalReturn = dynamic_cast<types::ReturnStatementNode*>(std::get<types::StatementNode*>(mainBody->getStatements().back()));
        const auto before = finalReturn->getExpression()->semanticInfo();
        const auto symbols = analyzer.symbols().symbolCount();
        const auto types = analyzer.types().typeCount();
        ok = analyzer.check();
        require(analyzer.phase() == SemanticPhase::Checked && ok != negative, "read-only checks result mismatch");
        require(finalReturn->getExpression()->semanticInfo() == before, "checks mutated AST metadata");
        require(analyzer.symbols().symbolCount() == symbols && analyzer.types().typeCount() == types,
                "checks registered symbols or constructed types");
        if (!negative) {
            auto* loop = dynamic_cast<types::WhileBlockStatementNode*>(std::get<types::StatementNode*>(mainBody->getStatements()[2]));
            auto* loopBody = dynamic_cast<types::CompoundStatementNode*>(loop->getBody());
            auto* jump = std::get<types::StatementNode*>(loopBody->getStatements().front());
            require(loop->semanticInfo().Target && jump->semanticInfo().Target == loop->semanticInfo().Target,
                    "named break was not attached to its resolved loop target");
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
