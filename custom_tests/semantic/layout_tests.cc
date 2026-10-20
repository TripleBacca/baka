#include "semantic/types.hh"
#include <iostream>
#include <stdexcept>

using namespace baka::semantic;

namespace {
    void require(bool value, const char* message) {
        if (!value) throw std::runtime_error(message);
    }
    void layouts() {
        Types types;
        const auto integer = types.builtin(Builtin::Int), character = types.builtin(Builtin::Char);
        auto node = types.nominal(TypeKind::Struct, "Node");
        require(!types.get(node).objectLayout && types.layout(node).state == QueryState::Incomplete,
                "forward record acquired layout metadata");
        require(types.beginDefinition(node), "cannot open record");
        require(types.addMember(node, {"tag", character}), "cannot add tag");
        require(types.addMember(node, {"value", integer}), "cannot add value");
        require(types.addMember(node, {"next", types.pointer(node)}), "cannot add self pointer");
        Member shared{"shared", types.builtin(Builtin::Long)};
        shared.isStatic = true;
        require(types.addMember(node, shared), "cannot add static");
        require(types.addMember(node, {"method", types.function(integer)}), "cannot add method");
        Member alias{"Alias", integer}; alias.isType = true;
        require(types.addMember(node, alias), "cannot add alias");
        require(bool(types.finishDefinition(node)), "record layout failed");
        const auto* stored = &*types.get(node).objectLayout;
        const auto* offsets = stored->offsets.data();
        require(stored->size == 16 && stored->alignment == 8 && stored->offsets.size() == 6 &&
                stored->offsets[0] == 0 && stored->offsets[1] == 4 && stored->offsets[2] == 8 &&
                !stored->offsets[3] && !stored->offsets[4] && !stored->offsets[5],
                "stored padding/member offsets are wrong");
        auto qualified = types.constant(node);
        for (int i = 0; i < 10; ++i) {
            auto result = types.layout(qualified);
            require(result && result.layout.size == 16 && result.layout.offsets[1] == 4, "const alias lost layout");
        }
        require(types.bindMemberSymbol(node, 1, {42}), "late symbol binding failed");
        require(&*types.get(node).objectLayout == stored && stored->offsets.data() == offsets,
                "read queries or symbol binding replaced stored layout");
        require(!types.addMember(node, {"late", integer}) && !types.finishDefinition(node),
                "completed storage shape was reopened");
        require(stored->offsets[1] == 4, "rejected mutation damaged cached offsets");

        auto derived = types.nominal(TypeKind::Class, "Derived");
        require(types.beginDefinition(derived) && types.setBase(derived, node), "cannot set base");
        require(types.addMember(derived, {"tail", character}) &&
                types.addMember(derived, {"shortValue", types.builtin(Builtin::Short)}), "cannot add derived fields");
        require(bool(types.finishDefinition(derived)), "derived layout failed");
        const auto& d = *types.get(derived).objectLayout;
        require(d.baseOffset == 0 && d.offsets[0] == 16 && d.offsets[1] == 18 && d.size == 24,
                "stored base/derived offsets are wrong");
        auto array = types.array(node, {BoundKind::Constant, 3});
        auto size = types.layout(array);
        require(size && size.layout.size == 48 && size.layout.offsets.empty() && !size.layout.baseOffset,
                "array layout leaked or modified element member offsets");
        require(stored->offsets.data() == offsets && stored->offsets[2] == 8, "array query changed record layout");

        auto overlay = types.nominal(TypeKind::Union, "Overlay");
        require(types.beginDefinition(overlay) && types.addMember(overlay, {"tag", character}) &&
                types.addMember(overlay, {"large", types.builtin(Builtin::Long)}) &&
                bool(types.finishDefinition(overlay)), "union layout failed");
        require(types.get(overlay).objectLayout->size == 8 && types.get(overlay).objectLayout->offsets[0] == 0 &&
                types.get(overlay).objectLayout->offsets[1] == 0, "union offsets are not retained");

        auto inner = types.nominal(TypeKind::Struct, "Inner"), outer = types.nominal(TypeKind::Class, "Outer");
        require(types.beginDefinition(outer) && types.addMember(outer, {"value", inner}), "cannot add forward dependency");
        require(!types.finishDefinition(outer) && !types.get(outer).objectLayout, "failed layout was published");
        require(types.beginDefinition(inner) && types.addMember(inner, {"value", integer}) &&
                bool(types.finishDefinition(inner)) && bool(types.finishDefinition(outer)), "completion retry failed");
        require(types.get(outer).objectLayout->size == 4 && types.get(outer).objectLayout->offsets[0] == 0,
                "retry retained stale incomplete layout");
        auto recursive = types.nominal(TypeKind::Struct, "Recursive");
        require(types.beginDefinition(recursive) && types.addMember(recursive, {"self", recursive}), "cannot build invalid control");
        require(!types.finishDefinition(recursive) && !types.get(recursive).objectLayout,
                "recursive by-value layout was cached as valid");
    }
}

int main() {
    try { layouts(); }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
