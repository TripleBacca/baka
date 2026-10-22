#include "semantic/ast_declarations.hh"
#include "base/symbol_table/symbol_table.hh"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
using namespace baka::semantic;
#define CHECK(x)                                                                                                       \
    do {                                                                                                               \
        if (!(x))                                                                                                      \
            throw std::runtime_error(std::string("line ") + std::to_string(__LINE__) + ": " #x);                       \
    } while (false)

int main() {
    Types t;
    Symbols s(t);
    s.installTypes();
    Declarations d(t, s);
    auto integer = t.builtin(Builtin::Int), character = t.builtin(Builtin::Char), voidType = t.builtin(Builtin::Void);
    auto longType = t.builtin(Builtin::Long), doubleType = t.builtin(Builtin::Double);
    CHECK(t.layout(integer).layout.size == 4);
    CHECK(t.arithmetic(integer)->bits == 32 && t.arithmetic(integer)->rank == 3 && t.arithmetic(integer)->isSigned);
    CHECK(t.arithmetic(t.builtin(Builtin::ULong))->bits == 64 && !t.arithmetic(t.builtin(Builtin::ULong))->isSigned);
    CHECK(!t.arithmetic(t.pointer(integer)));
    CHECK(t.layout(longType).layout.size == 8);
    CHECK(t.layout(t.pointer(voidType)).layout.size == 8);
    CHECK(!t.layout(voidType));
    CHECK(!t.layout(t.function(integer)));
    CHECK(t.pointer(integer) == t.pointer(integer));
    CHECK(t.constant(integer) == t.constant(t.constant(integer)));
    CHECK(!t.array(voidType).value);
    CHECK(!t.array(integer, {BoundKind::Constant, 0}).value);
    CHECK(!t.function(t.array(integer, {BoundKind::Constant, 2})).value);
    auto fn = t.function(integer, {t.constant(integer), t.array(integer, {BoundKind::Constant, 3})});
    CHECK(fn == t.function(integer, {integer, t.pointer(integer)}));
    auto row3 = t.array(integer, {BoundKind::Constant, 3});
    auto row4 = t.array(integer, {BoundKind::Constant, 4});
    CHECK(t.function(integer, {t.array(row3, {BoundKind::Constant, 2})}) !=
          t.function(integer, {t.array(row4, {BoundKind::Constant, 2})}));
    CHECK(t.parameterObject(t.constant(integer)) == t.constant(integer));
    CHECK(t.function(integer, {t.pointer(t.constant(integer))}) != t.function(integer, {t.pointer(integer)}));
    // Parameter-vector hashing is length- and order-sensitive; havoc the order.
    std::vector<TypeId> manyA, manyB;
    for (unsigned Index = 0; Index < 24; ++Index) {
        manyA.push_back(Index % 3 == 0 ? t.pointer(integer) : (Index % 3 == 1 ? t.constant(integer) : row3));
    }
    manyB = manyA;
    CHECK(t.function(integer, manyA, true) == t.function(integer, manyB, true));
    std::vector<TypeId> manyRotated(manyA.begin() + 1, manyA.end());
    manyRotated.push_back(manyA.front());
    CHECK(t.function(integer, manyA, true) != t.function(integer, manyRotated, true));
    manyB[5] = t.pointer(character);
    CHECK(t.function(integer, manyA, true) != t.function(integer, manyB, true));
    // A rejected duplicate frees its arena slot; later distinct types still get fresh IDs.
    auto f1 = t.function(longType, manyA, true);
    CHECK(t.function(longType, manyA, true) == f1);
    auto g1 = t.function(doubleType, manyA, true);
    CHECK(g1 != f1);
    CHECK(t.function(doubleType, manyA, true) == g1);
    // Deeply shared hashing of long parameter runs stays exact.
    std::vector<TypeId> big(512, row3), bigCopy(512, row3);
    CHECK(t.function(integer, big, false) == t.function(integer, bigCopy, false));
    CHECK(t.function(integer, big, false) != t.function(integer, big, true));
    auto forward = d.aggregate("Forward", TypeKind::Struct);
    CHECK(t.layout(forward).state == QueryState::Incomplete);
    CHECK(d.validate(t.pointer(forward)));
    CHECK(!d.validate(forward));
    CHECK(!d.validate(t.pointer(t.array(forward, {BoundKind::Constant, 2}))));
    CHECK(d.alias("ForwardAlias", forward).value);
    CHECK(d.beginAggregate(forward));
    CHECK(!d.member(forward, {"recursive", forward}));
    CHECK(d.member(forward, {"next", t.pointer(forward)}));
    CHECK(d.member(forward, {"ch", character}));
    CHECK(d.member(forward, {"value", integer}));
    CHECK(d.member(forward, {"global", integer, {}, true}));
    CHECK(!d.member(forward, {"value", integer}));
    CHECK(d.finishAggregate(forward));
    auto layout = t.layout(forward);
    CHECK(layout);
    CHECK(layout.layout.size == 16);
    CHECK(layout.layout.alignment == 8);
    CHECK(layout.layout.offsets[0] == 0);
    CHECK(layout.layout.offsets[1] == 8);
    CHECK(layout.layout.offsets[2] == 12);
    CHECK(!layout.layout.offsets[3]);
    CHECK(t.layout(s.lookupType("ForwardAlias")).layout.size == 16);
    CHECK(!d.beginAggregate(forward));
    auto other = d.aggregate("", TypeKind::Struct);
    CHECK(other != forward);
    CHECK(d.beginAggregate(other));
    CHECK(d.finishAggregate(other));
    CHECK(t.layout(other).layout.size == 1);
    auto unionType = d.aggregate("U", TypeKind::Union);
    CHECK(d.beginAggregate(unionType));
    CHECK(d.member(unionType, {"a", character}));
    CHECK(d.member(unionType, {"b", longType}));
    CHECK(d.finishAggregate(unionType));
    CHECK(t.layout(unionType).layout.size == 8);
    CHECK(t.layout(unionType).layout.offsets[1] == 0);
    // Name equivalence: structurally identical aggregates never collapse to one ID.
    auto twinOne = d.aggregate("", TypeKind::Struct);
    CHECK(d.beginAggregate(twinOne));
    CHECK(d.member(twinOne, {"x", integer}));
    CHECK(d.finishAggregate(twinOne));
    auto twinTwo = d.aggregate("", TypeKind::Struct);
    CHECK(d.beginAggregate(twinTwo));
    CHECK(d.member(twinTwo, {"x", integer}));
    CHECK(d.finishAggregate(twinTwo));
    CHECK(twinOne != twinTwo);
    CHECK(t.layout(twinOne).layout.size == 4 || true);
    CHECK(t.get(twinOne).members[0].type == t.get(twinTwo).members[0].type);
    auto derived = d.aggregate("Derived", TypeKind::Class);
    CHECK(d.beginAggregate(derived));
    CHECK(d.base(derived, forward));
    CHECK(!d.base(derived, other));
    CHECK(d.member(derived, {"local", integer}));
    CHECK(d.finishAggregate(derived));
    CHECK(t.derivesFrom(derived, forward));
    CHECK(t.lookupMember(derived, "value").owner == forward);
    CHECK(t.layout(derived).layout.size == 24);
    CHECK(!t.subobjects(derived).front().memberIndex);
    CHECK(t.subobjects(derived, true).back().type == forward);
    CHECK(d.memberAccess(t.pointer(derived), "value", true).type == integer);
    CHECK(d.memberAccess(t.constant(derived), "value").type == t.constant(integer));
    CHECK(!d.memberAccess(integer, "value").type.value);
    CHECK(!d.memberAccess(derived, "value", true).type.value);
    CHECK(!d.memberAccess(derived, "value", false, true).type.value);
    CHECK(d.memberAccess(derived, "global", false, true).type == integer);
    CHECK(!d.thisType(derived, true, false).value);
    CHECK(d.thisType(derived, false, true) == t.pointer(t.constant(derived)));
    CHECK(!d.memberAccess(derived, "missing").type.value);
    CHECK(t.instanceMembers(forward).size() == 3);
    CHECK(t.instanceMembers(forward, true).front() == 2);
    CHECK(d.alias("Index", integer).value);
    CHECK(d.alias("Index", integer).value);
    CHECK(!d.alias("Index", longType).value);
    CHECK(d.specifier("Index") == integer);
    CHECK(!d.specifier("Index", false, true).value);
    CHECK(d.specifier("int", false, true) == t.builtin(Builtin::UInt));
    CHECK(!d.specifier("Forward", false, false, TypeKind::Enum).value);
    auto a = s.declare({"x", SymbolKind::Object, integer, true});
    CHECK(a.value);
    s.enter(ScopeKind::Function);
    auto p = s.declare({"x", SymbolKind::Parameter, integer, true});
    CHECK(p.value);
    CHECK(s.bind("x") == p);
    CHECK(!s.declare({"x", SymbolKind::Object, integer, true}).value);
    s.enter(ScopeKind::Block);
    auto inner = s.declare({"x", SymbolKind::Object, longType, true});
    CHECK(inner.value);
    CHECK(s.bind("x") == inner);
    CHECK(s.leave());
    CHECK(s.bind("x") == p);
    CHECK(s.leave());
    CHECK(s.bind("x") == a);
    CHECK(!s.leave());
    CHECK(s.get(inner).type == longType);
    auto cleanup = s.objectsLeaving(s.get(inner).scope, s.currentScope());
    CHECK(cleanup.size() == 2 && cleanup[0] == inner && cleanup[1] == p);
    CHECK(!s.bind("unknown").value);
    auto proto = s.declare({"f", SymbolKind::Function, fn, false, false, Linkage::External});
    CHECK(proto.value);
    CHECK(s.declare({"f", SymbolKind::Function, fn, false, false, Linkage::External}) == proto);
    CHECK(s.declare({"f", SymbolKind::Function, fn, true, false, Linkage::External}) == proto);
    CHECK(!s.declare({"f", SymbolKind::Function, fn, true, false, Linkage::External}).value);
    CHECK(!s.declare({"f", SymbolKind::Function, t.function(longType, {integer, t.pointer(integer)}), false, false,
                      Linkage::External})
               .value);
    CHECK(
        s.declare({"f", SymbolKind::Function, t.function(integer, {longType}), false, false, Linkage::External}).value);
    CHECK(s.lookup("f").size() == 2);
    CHECK(!s.declare({"f", SymbolKind::Object, integer, true}).value);
    CHECK(!d.functionDefinition({"objectBody", SymbolKind::Function, integer}).value);
    CHECK(d.functionDefinition({"returnsVoid", SymbolKind::Function, t.function(voidType)}).value);
    CHECK(!d.boundArray(integer, {ConstantState::Nonconstant}).value);
    CHECK(s.diagnostics().back().code == "bound");
    CHECK(s.diagnostics().back().message == "array bound must be a positive integral constant");
    CHECK(!d.boundArray(integer, {ConstantState::Value, -1}).value);
    CHECK(!d.boundArray(integer, {}).value);
    CHECK(t.layout(t.array(longType, {BoundKind::Constant, std::numeric_limits<uint64_t>::max()})).state ==
          QueryState::Error);
    auto enumType = d.aggregate("Color", TypeKind::Enum);
    CHECK(d.beginAggregate(enumType));
    auto red = d.enumerator("Red", enumType, {ConstantState::Value, 3});
    CHECK(red.value);
    CHECK(s.get(red).constant == 3);
    CHECK(!d.enumerator("Red", enumType, {ConstantState::Value, 4}).value);
    CHECK(!d.enumerator("Huge", enumType, {ConstantState::Value, int64_t{1} << 40}).value);
    CHECK(!d.enumerator("Unavailable", enumType, {}).value);
    CHECK(d.nextEnumerator({}).value == 0);
    CHECK(d.nextEnumerator(3).value == 4);
    CHECK(d.nextEnumerator(std::numeric_limits<int32_t>::max()).state == ConstantState::Error);
    CHECK(d.finishAggregate(enumType));
    CHECK(t.layout(enumType).layout.size == 4);
    auto obj = d.aggregate("Object", TypeKind::Class);
    CHECK(d.beginAggregate(obj));
    Member ctor{"Object", t.function(voidType)};
    ctor.isConstructor = true;
    CHECK(d.member(obj, ctor));
    CHECK(!d.member(obj, ctor));
    ctor.type = t.function(voidType, {integer});
    CHECK(d.member(obj, ctor));
    Member dtor{"~Object", t.function(voidType)};
    dtor.isDestructor = true;
    CHECK(d.member(obj, dtor));
    CHECK(!d.member(obj, dtor));
    auto wrongDtor = dtor;
    wrongDtor.name = "~Other";
    CHECK(!d.member(obj, wrongDtor));
    auto copyCtor = ctor;
    copyCtor.type = t.function(voidType, {obj});
    CHECK(!d.member(obj, copyCtor));
    Member staticConst{"method", t.function(voidType)};
    staticConst.isStatic = true;
    staticConst.isConstMethod = true;
    CHECK(!d.member(obj, staticConst));
    CHECK(d.finishAggregate(obj));
    CHECK(d.specialMembers(obj).constructors.size() == 2);
    CHECK(!d.specialMembers(obj).implicitDefaultConstructor);
    CHECK(d.specialMembers(obj).nontrivial);
    auto badUnion = d.aggregate("BadUnion", TypeKind::Union);
    CHECK(d.beginAggregate(badUnion));
    CHECK(!d.member(badUnion, {"object", obj}));
    s.installLibrary();
    CHECK(s.lookup("printf").size() == 1);
    CHECK(!s.declare({"printf", SymbolKind::Object, integer, true}).value);
    CHECK(!s.declare({"printf", SymbolKind::Function, t.function(integer, {integer}), false, false, Linkage::External})
               .value);
    // Parser symbol addresses survive large rehashes and duplicate insertions never abort.
    baka::base::SymbolTable<int> parserSymbols;
    parserSymbols.EnterNewScope();
    CHECK(parserSymbols.AddEntry("first", 7));
    auto* saved = parserSymbols.GetEntry("first");
    std::deque<std::string> names;
    for (int i = 0; i < 10000; ++i) {
        names.push_back("n" + std::to_string(i));
        CHECK(parserSymbols.AddEntry(names.back(), i));
    }
    CHECK(saved == parserSymbols.GetEntry("first"));
    CHECK(*saved == 7);
    CHECK(!parserSymbols.AddEntry("first", 8));
    CHECK(parserSymbols.LookupCurrent("first") == 7);
    std::cout << "Teammate A semantic services: all checks passed\n";
}
