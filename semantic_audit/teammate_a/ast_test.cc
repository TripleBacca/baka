#include "semantic/aggregates.hh"
#include <iostream>
#include <stdexcept>
using namespace baka::semantic;
namespace ast = baka::types;
#define CHECK(x)                                                                                                       \
    do {                                                                                                               \
        if (!(x))                                                                                                      \
            throw std::runtime_error(std::string("line ") + std::to_string(__LINE__) + ": " #x);                       \
    } while (false)

struct Expr : ast::ExpressionNode {
    int64_t value;
    std::string reference;
    SymbolId expected;

    Expr(int64_t value, std::string reference = {}, SymbolId expected = {})
        : value(value), reference(std::move(reference)), expected(expected) {}

    void Print(size_t = 0) const override {}
};

struct Evaluator : DeclarationEvaluator {
    Symbols& symbols;

    explicit Evaluator(Symbols& symbols) : symbols(symbols) {}

    IntegerResult integer(const ast::ExpressionNode* expression, EvaluationContext context) override {
        CHECK(context.scope == symbols.currentScope());
        auto* expr = dynamic_cast<const Expr*>(expression);
        CHECK(expr);
        if (!expr->reference.empty()) {
            CHECK(symbols.bind(expr->reference) == expr->expected);
        }
        return {ConstantState::Value, expr->value};
    }
};

int main() {
    Types t;
    Symbols s(t);
    s.installTypes();
    Declarations d(t, s);
    Evaluator e(s);
    ASTDeclarations a(t, s, d, &e);
    ASTAggregates aggregates(t, s, d, a);
    auto integer = t.builtin(Builtin::Int);
    ast::IdentifierNode intName("int"), aliasName("Alias"), varName("v"), countName("count");
    Expr two(2), three(3), four(4);
    ast::DeclarationIdentifierNode matrix(&varName, {&two, &three});
    auto matrixType = a.declarator(integer, &matrix);
    CHECK(t.layout(matrixType).layout.size == 24);
    CHECK(t.get(matrixType).bound.count == 2);
    CHECK(t.get(t.get(matrixType).element).bound.count == 3);
    ast::DeclarationIdentifierNode ptr(&varName, {});
    ptr.appendPointer();
    ast::DeclarationIdentifierNode ptrToArray;
    ptrToArray.setInnerDeclaration(&ptr);
    ptrToArray.appendArraySize(&three);
    auto pta = a.declarator(integer, &ptrToArray);
    CHECK(t.get(pta).kind == TypeKind::Pointer);
    CHECK(t.get(t.get(pta).element).kind == TypeKind::Array);
    ast::DeclarationIdentifierNode arrayOfPtrs(&varName, {&three});
    arrayOfPtrs.appendPointer();
    auto aop = a.declarator(integer, &arrayOfPtrs);
    CHECK(t.get(aop).kind == TypeKind::Array);
    CHECK(t.get(t.get(aop).element).kind == TypeKind::Pointer);
    ast::FunctionParameterList empty({});
    ast::DeclarationIdentifierNode fnPtr;
    fnPtr.setInnerDeclaration(&ptr);
    fnPtr.setFunctionParameters(&empty);
    auto fp = a.declarator(integer, &fnPtr);
    CHECK(t.get(fp).kind == TypeKind::Pointer);
    CHECK(t.get(t.get(fp).element).kind == TypeKind::Function);
    ast::DeclarationIdentifierNode returnsPtr(&varName, {});
    returnsPtr.appendPointer();
    returnsPtr.setFunctionParameters(&empty);
    auto rp = a.declarator(integer, &returnsPtr);
    CHECK(t.get(rp).kind == TypeKind::Function);
    CHECK(t.get(t.get(rp).element).kind == TypeKind::Pointer);
    ast::DeclarationIdentifierNode returnsArray(&varName, {&three});
    returnsArray.setFunctionParameters(&empty);
    CHECK(!a.declarator(integer, &returnsArray).value);
    ast::TypedefNode alias(&intName, ast::TypeSpecifierModifier::NONE, true);
    alias.AddVariable(&arrayOfPtrs);
    CHECK(a.aliases(alias).front().value);
    auto aliasType = s.lookupType("v");
    CHECK(t.get(aliasType).kind == TypeKind::Array);
    CHECK(t.isConst(t.get(t.get(aliasType).element).element));
    s.enter(ScopeKind::Function);
    auto old = s.declare({"count", SymbolKind::Object, integer, true});
    s.enter(ScopeKind::Block);
    Expr bound(4, "count", old);
    ast::DeclarationIdentifierNode count(&countName, {&bound});
    ast::SingleDeclarationNode countDecl(&count, &two);
    ast::DeclarationList list(false, false, ast::TypeSpecifierModifier::NONE, &intName, {&countDecl});
    bool initialized = false;
    auto ids = a.declarationList(list, [&](SymbolId symbol, const ast::ExpressionNode* init) {
        CHECK(s.bind("count") == symbol);
        CHECK(symbol != old);
        CHECK(init == &two);
        initialized = true;
    });
    CHECK(ids.size() == 1 && ids[0].value && initialized);
    CHECK(t.get(s.get(ids[0]).type).bound.count == 4);
    ast::IdentifierNode inferName("inferred");
    ast::DeclarationIdentifierNode infer(&inferName, {nullptr});
    ast::SingleDeclarationNode inferDecl(&infer, &three);
    ast::DeclarationList inferred(false, false, ast::TypeSpecifierModifier::NONE, &intName, {&inferDecl});
    auto inferredIds = a.declarationList(inferred, [&](SymbolId id, const ast::ExpressionNode*) {
        CHECK(s.bind("inferred") == id);
        CHECK(s.completeObject(id, t.array(integer, {BoundKind::Constant, 3})));
    });
    CHECK(t.layout(s.get(inferredIds[0]).type).layout.size == 12);
    ast::IdentifierNode first("first"), second("second");
    ast::DeclarationIdentifierNode firstVar(&first, {}), secondVar(&second, {});
    ast::SingleDeclarationNode firstDecl(&firstVar, &two), secondDecl(&secondVar, &two);
    ast::DeclarationList sequence(false, false, ast::TypeSpecifierModifier::NONE, &intName, {&firstDecl, &secondDecl});
    size_t visited = 0;
    auto sequenceIds = a.declarationList(sequence, [&](SymbolId id, const ast::ExpressionNode*) {
        CHECK(s.bind(s.get(id).name) == id);
        if (visited++ == 0) {
            CHECK(s.lookup("second").empty());
        } else {
            CHECK(s.bind("first").value);
        }
    });
    CHECK(sequenceIds.size() == 2 && visited == 2);
    s.leave();
    s.leave();
    ast::DeclarationIdentifierNode param(&first, {});
    ast::SingleDeclarationNode paramDecl(&param, nullptr);
    ast::FunctionParameter parameter(false, ast::TypeSpecifierModifier::NONE, &intName, &paramDecl);
    ast::FunctionParameterList duplicate({&parameter, &parameter});
    auto before = s.diagnostics().size();
    ast::DeclarationIdentifierNode dupFn(&varName, {});
    dupFn.setFunctionParameters(&duplicate);
    CHECK(!a.declarator(integer, &dupFn).value);
    CHECK(s.diagnostics().size() > before);
    s.enter(ScopeKind::Function);
    ast::FunctionParameterList single({&parameter});
    CHECK(a.parameters(single).front().value);
    CHECK(!s.declare({"first", SymbolKind::Object, integer, true}).value);
    s.leave();
    ast::IdentifierNode enumName("Numbers"), oneName("One"), nextName("Next");
    ast::EnumNode enumeration(&enumName);
    enumeration.addEnumValue(&oneName, &four);
    enumeration.addEnumValue(&nextName);
    auto en = a.enumeration(enumeration);
    CHECK(en.value);
    CHECK(s.get(s.lookup("One").front()).constant == 4);
    CHECK(s.get(s.lookup("Next").front()).constant == 5);
    ast::IdentifierNode missingEnum("MissingEvaluator"), ev("EV");
    ast::EnumNode noEvaluator(&missingEnum);
    noEvaluator.addEnumValue(&ev, &two);
    ASTDeclarations unavailable(t, s, d);
    CHECK(!unavailable.enumeration(noEvaluator).value);
    ast::IdentifierNode aggregateName("Record"), fieldName("field");
    ast::DeclarationIdentifierNode fieldVar(&fieldName, {});
    ast::SingleDeclarationNode fieldDecl(&fieldVar, nullptr);
    ast::DeclarationList fields(false, false, ast::TypeSpecifierModifier::NONE, &intName, {&fieldDecl});
    ast::StructBodyNode body;
    body.AddDeclaration(&fields);
    ast::StructDefinitionNode definition(&aggregateName, &body);
    auto aggregate = aggregates.definition(definition);
    CHECK(aggregate.value);
    CHECK(t.layout(aggregate).layout.size == 4);
    CHECK(t.lookupMember(aggregate, "field").indices.size() == 1);
    ast::UnionNode unionNode(&body);
    CHECK(!unionNode.getName());
    CHECK(aggregates.definition(unionNode).value);
    // Member ordering survives interleaving and special members are never overwritten.
    ast::ConstructorNode ctor1(&empty, nullptr), ctor2(&empty, nullptr);
    ast::DestructorNode dtor1(nullptr), dtor2(nullptr);
    ast::StructBodyNode ordered;
    ordered.SetConstructor(&ctor1);
    ordered.AddDeclaration(&fields);
    ordered.SetDestructor(&dtor1);
    ordered.SetConstructor(&ctor2);
    ordered.SetDestructor(&dtor2);
    CHECK(ordered.getMembers()[0] == &ctor1 && ordered.getMembers()[1] == &fields && ordered.getMembers()[3] == &ctor2);
    CHECK(ordered.getConstructors().size() == 2 && ordered.getDestructors().size() == 2);
    ast::DeclarationIdentifierNode malformed;
    CHECK(!malformed.getIdentifier());
    CHECK(malformed.getVariableName().empty());
    std::cout << "Teammate A AST services: all checks passed\n";
}
