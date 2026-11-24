#include "lowering.hh"

namespace baka {
    namespace ir {
        // Every class object starts with a vptr: the address of slot 0 of its
        // class's read-only vtable, an array of untyped code pointers. Slot 0 is
        // the destructor (null when destruction is trivial); method slots follow.
        TypeId Lowerer::vtableSlot() {
            return Result.pointer(importType(Analyzer.types().builtin(semantic::Builtin::Void)));
        }

        bool Lowerer::polymorphic(TypeId type) const {
            return Result.type(type).kind == TypeKind::Record && Vtables.count(type);
        }

        uint32_t Lowerer::pureVirtual() {
            if (PureVirtual) {
                return PureVirtual;
            }
            Function helper;
            helper.id = NextObject++;
            helper.name = "__baka_pure_virtual";
            helper.signature =
                Result.function(importType(Analyzer.types().builtin(semantic::Builtin::Void)), {}, false);
            PureVirtual = helper.id;
            Result.functions.push_back(std::move(helper));
            return PureVirtual;
        }

        // Creates a read-only vtable global for every complete polymorphic record and points its slots at
        // the code: slot 0 is the destructor (left null when trivial), slot 1 is left null for
        // lowerEqualities() to fill with the class's `.eq` function when the program compares the class,
        // pure slots use the __baka_pure_virtual helper, and the rest are the most derived overrides from
        // the semantic table. The slot array type is shared, and Vtables records each global's index.
        void Lowerer::buildVtables() {
            const auto slot = vtableSlot();
            const auto count = Result.types.size();
            for (TypeId id = 1; id < count; ++id) {
                if (Result.type(id).kind != TypeKind::Record || !Result.type(id).complete) {
                    continue;
                }
                const auto source = semanticType(id);
                if (!source.value || !Analyzer.types().polymorphic(source)) {
                    continue;
                }
                const auto& table = Analyzer.types().get(source).vtable;
                Type array;
                array.kind = TypeKind::Array;
                array.complete = true;
                array.element = slot;
                array.count = table.size();
                array.size = table.size() * Result.type(slot).size;
                array.alignment = Result.type(slot).alignment;
                TypeId arrayType = 0;
                for (TypeId i = 1; i < Result.types.size() && !arrayType; ++i) {
                    if (Result.types[i] == array) {
                        arrayType = i;
                    }
                }
                if (!arrayType) {
                    Result.types.push_back(array);
                    arrayType = static_cast<TypeId>(Result.types.size() - 1);
                }
                Object vtable;
                vtable.id = NextObject++;
                vtable.name = ".vtable." + Result.type(id).name;
                vtable.type = arrayType;
                vtable.global = vtable.internal = vtable.readOnly = true;
                for (size_t index = 0; index < table.size(); ++index) {
                    Operand target;
                    if (index == 0) {
                        if (auto found = Destructors.find(id); found != Destructors.end()) {
                            target = callable(found->second);
                        }
                    } else if (index == semantic::EqualitySlot) {
                        // Filled by lowerEqualities() only when the program compares this class.
                    } else if (table[index].pure) {
                        target = callable(pureVirtual());
                    } else {
                        const auto& member = Analyzer.types().get(table[index].owner).members.at(table[index].member);
                        target = callable(member.symbol.value);
                    }
                    if (target) {
                        vtable.initialValues.push_back({index * Result.type(slot).size, target});
                    }
                }
                Vtables[id] = Result.globals.size();
                Result.globals.push_back(std::move(vtable));
            }
        }

        Operand Lowerer::vptrAddress(Operand object) {
            return cast(object, Result.pointer(Result.pointer(vtableSlot())));
        }

        // The value a vptr holds for `type`: the address of slot 0 of its vtable.
        Operand Lowerer::vtableAddress(TypeId type) {
            const auto& global = Result.globals.at(Vtables.at(type));
            Operand table{OperandKind::Object, Result.pointer(global.type), global.id};
            return cast(table, Result.pointer(vtableSlot()));
        }

        void Lowerer::storeVptr(Operand object, TypeId type) {
            if (!Vtables.count(type)) {
                return;
            }
            emit(Instruction::store(vptrAddress(object), vtableAddress(type), State.Source));
        }

        // Assignment changes an object's value, never its dynamic type: the
        // destination keeps its vptr even when the source was a derived object.
        void Lowerer::assign(Operand location, Operand value) {
            if (!polymorphic(value.type)) {
                store(location, value);
                return;
            }
            auto vptr = vptrAddress(location);
            auto saved = load(vptr, Result.pointer(vtableSlot()));
            emit(Instruction::store(location, value, State.Source));
            emit(Instruction::store(vptr, saved, State.Source));
        }

        // `a ==== b` / `a !=== b`: every class has exactly one vtable, so equal vptrs mean the same dynamic
        // class. Both operands are always evaluated, left to right (an operand such as `arr[i++]` has side
        // effects); only the vptr loads are skipped when a class is known at compile time. An exact operand
        // (named object, field or element of one) contributes its vtable's address as a constant; two exact
        // operands make the whole result a constant. A pointer operand is the object's address.
        Operand Lowerer::strictEquality(const types::BinaryExpressionNode* node) {
            const bool equal = node->getBinaryOp() == types::OP_STRICT_EQ;
            const auto resultType = type(node);
            struct Side {
                Operand address;
                TypeId exactClass = 0;
            };
            auto side = [&](const types::ExpressionNode* expression) {
                const auto& types = Analyzer.types();
                auto source = types.unConst({expression->semanticInfo().Type});
                if (types.get(source).kind == semantic::TypeKind::Pointer) {
                    return Side{value(expression), 0};
                }
                return Side{address(expression), exactType(expression) ? importType(source) : 0};
            };
            const auto left = side(node->getLeft());
            const auto right = side(node->getRight());
            if (left.exactClass && right.exactClass) {
                return integer(resultType, (left.exactClass == right.exactClass) == equal ? 1 : 0);
            }
            const auto table = Result.pointer(vtableSlot());
            auto vptr = [&](const Side& operand) {
                return operand.exactClass ? vtableAddress(operand.exactClass) : load(vptrAddress(operand.address), table);
            };
            auto leftTable = vptr(left);
            auto rightTable = vptr(right);
            return emitValue(equal ? Opcode::Equal : Opcode::NotEqual, resultType, leftTable, rightTable);
        }

        Operand Lowerer::virtualCallee(Operand instance, uint32_t slot, TypeId signature) {
            auto table = load(vptrAddress(instance), Result.pointer(vtableSlot()));
            auto entry = emitValue(Opcode::AddressOffset, Result.pointer(vtableSlot()), table,
                                   integer(integerType(), uint64_t{slot} * Result.type(vtableSlot()).size));
            return cast(load(entry, vtableSlot()), Result.pointer(signature));
        }

        // Whether the object an expression names certainly has exactly its static type, so a call on
        // it needs no dispatch and ==/==== can decide its class at compile time. True for a named
        // object that is not a reference (including a by-value parameter), a field of such an object,
        // and an element of a real array (not of a pointer or an array parameter, which is a pointer).
        // Caveat: a named object is trusted even while a base constructor still runs on it.
        bool Lowerer::exactType(const types::ExpressionNode* expression) const {
            if (auto* index = dynamic_cast<const types::IndexPostfixExpr*>(expression)) {
                const types::ExpressionNode* base = index->getBase();
                if (auto* decay = dynamic_cast<const types::ImplicitCastNode*>(base);
                    decay && decay->getConversionKind() == types::ImplicitConversionKind::ArrayToPointer) {
                    base = decay->getOperand();
                }
                const auto& types = Analyzer.types();
                return types.get(types.unConst({base->semanticInfo().Type})).kind == semantic::TypeKind::Array;
            }
            if (auto* dot = dynamic_cast<const types::MemberPostfixExpr*>(expression)) {
                const auto symbol = dot->semanticInfo().Symbol;
                return symbol && !Analyzer.types().isReference(Analyzer.symbols().get({symbol}).type) &&
                       exactType(dot->getBase());
            }
            auto* primary = dynamic_cast<const types::PrimaryExpressionNode*>(expression);
            if (!primary || !std::holds_alternative<types::IdentifierNode*>(primary->getExpression())) {
                return false;
            }
            auto symbol = std::get<types::IdentifierNode*>(primary->getExpression())->semanticInfo().Symbol;
            if (!symbol) {
                symbol = primary->semanticInfo().Symbol;
            }
            if (!symbol || Members.count(symbol)) {
                return false;
            }
            const auto& selected = Analyzer.symbols().get({symbol});
            return (selected.kind == semantic::SymbolKind::Object || selected.kind == semantic::SymbolKind::Parameter) &&
                   !Analyzer.types().isReference(selected.type);
        }
    }
}
