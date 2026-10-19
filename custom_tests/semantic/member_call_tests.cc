// A bound instance method is a callable plus a receiver, not an ordinary
// function designator. Keep both on the AST so lowering can supply `this`.
#include "semantic/semantic_analyzer.hh"
#include "lexer/lexer.hh"
#include "parser/parser.hh"
#include "driver/gctx.hh"
#include <functional>
#include <iostream>
#include <stdexcept>

using namespace baka;
using namespace baka::semantic;
using namespace baka::types;
namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
template<class T> T* as(ExpressionNode* expression) {
    if (auto* cast = dynamic_cast<ImplicitCastNode*>(expression)) return as<T>(cast->getOperand());
    if (auto* comma = dynamic_cast<CommaExpressionNode*>(expression);
        comma && comma->getExpressions().size() == 1) return as<T>(comma->getExpressions().front());
    return dynamic_cast<T*>(expression);
}
size_t conversionCount(ExpressionNode* expression, ImplicitConversionKind kind) {
    if (!expression) return 0;
    size_t count = 0;
    if (auto* cast = dynamic_cast<ImplicitCastNode*>(expression); cast && cast->getConversionKind() == kind) ++count;
    expression->rewriteExpressions([&](ExpressionNode* child) {
        count += conversionCount(child, kind);
        return child;
    });
    return count;
}
ExpressionNode* statement(CompoundStatementNode* body, size_t index) {
    auto* expression = dynamic_cast<ExpressionStatementNode*>(std::get<StatementNode*>(body->getStatements().at(index)));
    require(expression != nullptr, "expected expression statement");
    return expression->getExpression();
}
void instanceCall(FunctionCallPostfixExpr* call, const SemanticAnalyzer& analyzer) {
    require(call != nullptr, "instance invocation disappeared");
    const auto& info = call->semanticInfo();
    require(info.CallSignature && info.MemberOwner && info.MemberIndex && info.Symbol,
            "instance call is missing selected declaration metadata");
    const auto& member = analyzer.types().get({info.MemberOwner}).members.at(info.MemberIndex - 1);
    require(member.type.value == info.CallSignature && !member.isStatic,
            "instance call selected the wrong member/signature");
    require(conversionCount(call->getCallee(), ImplicitConversionKind::FunctionToPointer) == 0,
            "instance member callee was incorrectly converted to an ordinary function pointer");
}
}
int main() {
    try {
        const std::string source =
            "class A{int b;int c;int balls();int defined(){return 1;}"
            "int implicit(){return balls();}static int stat(){return 2;}int (*field)();int usefield(){return field();}};"
            "int freefn(){return 3;}"
            "int main(int a){A d;A *p=&d;d.balls();d.defined();p->balls();"
            "d.stat();freefn();int (*fp)()=freefn;fp();d.field=freefn;d.field();p->field();(d.balls)();(p->balls)();(d.field)();return 0;}";
        driver::Gctx::AttachLineIndex(std::make_shared<base::LineIndex>(source));
        auto tokens = lexer::Tokenize(source);
        parser::Parser parser(tokens.Tokens, tokens.Locations);
        auto* program = dynamic_cast<ProgramNode*>(parser.Parse());
        require(program && !driver::Gctx::ErrorFound(), "member call fixture failed to parse");
        SemanticAnalyzer analyzer;
        if (!analyzer.analyze(*program)) {
            for (const auto& diagnostic : analyzer.diagnostics()) std::cerr << diagnostic.message << '\n';
            throw std::runtime_error("member call fixture failed semantic analysis");
        }
        auto* main = dynamic_cast<FunctionNode*>(program->getBody().back());
        auto* body = dynamic_cast<CompoundStatementNode*>(main->getBody());
        for (size_t index : {2u, 3u, 4u}) instanceCall(as<FunctionCallPostfixExpr>(statement(body, index)), analyzer);
        auto* declared = as<FunctionCallPostfixExpr>(statement(body, 2));
        auto* member = as<MemberPostfixExpr>(declared->getCallee());
        require(member && as<PrimaryExpressionNode>(member->getBase()) &&
                std::holds_alternative<IdentifierNode*>(as<PrimaryExpressionNode>(member->getBase())->getExpression()), "dot call lost its explicit receiver");
        require(member->getBase()->semanticInfo().LValue, "dot receiver was loaded as an aggregate value");
        auto* arrow = as<ArrowPostfixExpr>(as<FunctionCallPostfixExpr>(statement(body, 4))->getCallee());
        require(arrow && as<PrimaryExpressionNode>(arrow->getBase()) &&
                std::holds_alternative<IdentifierNode*>(as<PrimaryExpressionNode>(arrow->getBase())->getExpression()), "arrow call lost its pointer receiver");
        require(conversionCount(arrow->getBase(), ImplicitConversionKind::LValueToRValue) == 1,
                "arrow receiver did not load its pointer");

        auto* definition = dynamic_cast<ClassDefinitionNode*>(program->getBody().front());
        // balls is a declaration; defined and implicit are function definitions.
        FunctionCallPostfixExpr* implicit = nullptr;
        for (auto* method : definition->getBody()->getFunctions()) {
            if (!method->getBody()) continue;
            auto* block = dynamic_cast<CompoundStatementNode*>(method->getBody());
            auto* result = dynamic_cast<ReturnStatementNode*>(std::get<StatementNode*>(block->getStatements().front()));
            if (auto* call = as<FunctionCallPostfixExpr>(result->getExpression()); call &&
                method->getDeclarator()->getVariableName() == "implicit") implicit = call;
            if (auto* fieldCall = as<FunctionCallPostfixExpr>(result->getExpression()); fieldCall &&
                method->getDeclarator()->getVariableName() == "usefield") {
                require(conversionCount(fieldCall->getCallee(), ImplicitConversionKind::LValueToRValue) == 1 &&
                        conversionCount(fieldCall->getCallee(), ImplicitConversionKind::FunctionToPointer) == 0,
                        "implicit function-pointer field was not loaded as a pointer");
            }
        }
        instanceCall(implicit, analyzer);

        auto* staticCall = as<FunctionCallPostfixExpr>(statement(body, 5));
        require(staticCall && staticCall->semanticInfo().CallSignature, "static call lost its signature");
        require(conversionCount(staticCall->getCallee(), ImplicitConversionKind::FunctionToPointer) == 1,
                "static member function did not retain ordinary function decay");
        auto* freeCall = as<FunctionCallPostfixExpr>(statement(body, 6));
        require(freeCall && freeCall->semanticInfo().CallSignature, "free function call lost its signature");
        require(conversionCount(freeCall->getCallee(), ImplicitConversionKind::FunctionToPointer) == 1,
                "free function did not retain ordinary function decay");
        auto* pointerCall = as<FunctionCallPostfixExpr>(statement(body, 8));
        require(pointerCall && pointerCall->semanticInfo().CallSignature, "function pointer call lost its signature");
        require(conversionCount(pointerCall->getCallee(), ImplicitConversionKind::FunctionToPointer) == 0 &&
                conversionCount(pointerCall->getCallee(), ImplicitConversionKind::LValueToRValue) == 1,
                "function pointer callee was not loaded exactly once");
        for (size_t index : {10u, 11u, 14u}) {
            auto* fieldCall = as<FunctionCallPostfixExpr>(statement(body, index));
            require(fieldCall && fieldCall->semanticInfo().CallSignature && !fieldCall->semanticInfo().Symbol,
                    "function-pointer field was mistaken for a direct method");
            require(conversionCount(fieldCall->getCallee(), ImplicitConversionKind::FunctionToPointer) == 0 &&
                    conversionCount(fieldCall->getCallee(), ImplicitConversionKind::LValueToRValue) >= 1,
                    "function-pointer field did not retain a pointer load");
            auto* load = dynamic_cast<ImplicitCastNode*>(fieldCall->getCallee());
            require(load && analyzer.types().get({load->getDestinationType()}).kind == TypeKind::Pointer,
                    "function-pointer field callee lost its pointer type");
        }
        for (size_t index : {12u, 13u}) instanceCall(as<FunctionCallPostfixExpr>(statement(body, index)), analyzer);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
