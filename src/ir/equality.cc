#include "lowering.hh"
#include <algorithm>

namespace baka {
    namespace ir {
        // Record `==` compares values field by field, never raw memory: padding bytes are arbitrary,
        // floating point needs IEEE equality (NaN != NaN, -0.0 == 0.0) and a class's vptr is not a
        // value. Each compared record type gets one internal function `.eq.<T>(ptr<T>, ptr<T>) -> u1`
        // with no vptr check; callers do that. Bodies are emitted by lowerEqualities() after every
        // other body, so a comparison anywhere (including global initializers) can request one.
        uint32_t Lowerer::equalityFunction(TypeId record) {
            if (auto found = EqualityFunctions.find(record); found != EqualityFunctions.end()) {
                return found->second;
            }
            const auto boolean = importType(Analyzer.types().builtin(semantic::Builtin::Bool));
            const auto pointer = Result.pointer(record);
            Function function;
            function.id = NextObject++;
            function.name = ".eq." + Result.type(record).name;
            function.signature = Result.function(boolean, {pointer, pointer}, false);
            function.defined = function.internal = true;
            for (const char* name : {"a", "b"}) {
                Object parameter;
                parameter.id = NextObject++;
                parameter.name = name;
                parameter.type = pointer;
                function.parameters.push_back(parameter.id);
                function.objects.push_back(std::move(parameter));
            }
            const auto id = function.id;
            Result.functions.push_back(std::move(function));
            EqualityFunctions[record] = id;
            EqualityWork.push_back(record);
            return id;
        }

        // Compares the values at `left` and `right` (pointers to `type`) and jumps to `unequal` on the
        // first difference; otherwise falls through in a new current block. Nested records call their
        // own equality function (a member object always has exactly its declared type); arrays compare
        // element by element.
        void Lowerer::compareValues(TypeId type, Operand left, Operand right, BlockId unequal) {
            const auto boolean = importType(Analyzer.types().builtin(semantic::Builtin::Bool));
            const auto info = Result.type(type);
            Operand different;
            if (info.kind == TypeKind::Record) {
                auto same = invoke(equalityFunction(type), {left, right});
                different = emitValue(Opcode::LogicalNot, boolean, same);
            } else if (info.kind == TypeKind::Array) {
                // Short arrays are unrolled; longer ones use one index for both operands.
                const auto element = info.element;
                const auto size = Result.type(element).size;
                const auto pointer = Result.pointer(element);
                auto at = [&](Operand base, Operand offset) {
                    return emitValue(Opcode::AddressOffset, pointer, base, offset);
                };
                if (info.count <= 4) {
                    for (uint64_t i = 0; i < info.count; ++i) {
                        auto offset = integer(integerType(), i * size);
                        auto first = at(left, offset);
                        compareValues(element, first, at(right, offset), unequal);
                    }
                    return;
                }
                auto index = temporary(integerType());
                emit(Opcode::Copy, index, integer(integerType(), 0));
                auto condition = newBlock(), body = newBlock(), done = newBlock();
                jump(condition);
                select(condition);
                branch(emitValue(Opcode::Less, boolean, index, integer(integerType(), info.count)), body, done);
                select(body);
                auto offset = emitValue(Opcode::Multiply, integerType(), index, integer(integerType(), size));
                auto first = at(left, offset);
                compareValues(element, first, at(right, offset), unequal);
                emit(Opcode::Copy, index, emitValue(Opcode::Add, integerType(), index, integer(integerType(), 1)));
                jump(condition);
                select(done);
                return;
            } else {
                auto first = load(left, type);
                different = emitValue(Opcode::NotEqual, boolean, first, load(right, type));
            }
            auto next = newBlock();
            branch(different, unequal, next);
            select(next);
        }

        // Base subobject first, then the record's own fields in declaration order. The vptr is not a
        // field (memberOffsets never covers it), so it is skipped automatically.
        void Lowerer::compareFields(TypeId record, Operand left, Operand right, BlockId unequal) {
            const auto info = Result.type(record);
            if (info.isUnion) {
                fail(nullptr, "unions cannot be compared");
            }
            if (info.base && info.baseOffset) {
                auto offset = integer(integerType(), *info.baseOffset);
                const auto pointer = Result.pointer(info.base);
                auto first = emitValue(Opcode::AddressOffset, pointer, left, offset);
                compareValues(info.base, first, emitValue(Opcode::AddressOffset, pointer, right, offset), unequal);
            }
            for (size_t i = 0; i < info.memberOffsets.size(); ++i) {
                if (!info.memberOffsets[i] || i >= info.memberTypes.size() || !info.memberTypes[i]) {
                    continue;
                }
                const auto field = info.memberTypes[i];
                const auto pointer = Result.pointer(field);
                auto offset = integer(integerType(), *info.memberOffsets[i]);
                auto first = emitValue(Opcode::AddressOffset, pointer, left, offset);
                compareValues(field, first, emitValue(Opcode::AddressOffset, pointer, right, offset), unequal);
            }
        }

        // Emits every requested equality body. Dynamic class comparisons marked a static class in
        // EqualityDispatch; every class deriving from it (or it) gets a function, stored in vtable
        // slot 1, before the bodies are emitted. Bodies may request further functions (nested
        // records), so the work list is drained by index.
        void Lowerer::lowerEqualities() {
            for (auto root : EqualityDispatch) {
                const auto source = semanticType(root);
                for (TypeId id = 1; id < Result.types.size(); ++id) {
                    if (!polymorphic(id)) {
                        continue;
                    }
                    const auto candidate = semanticType(id);
                    if (id == root || Analyzer.types().derivesFrom(candidate, source)) {
                        equalityFunction(id);
                    }
                }
            }
            const auto boolean = importType(Analyzer.types().builtin(semantic::Builtin::Bool));
            for (size_t index = 0; index < EqualityWork.size(); ++index) {
                const auto record = EqualityWork[index];
                const auto id = EqualityFunctions.at(record);
                size_t position = 0;
                while (Result.functions[position].id != id) ++position;
                State = FunctionContext{};
                State.FunctionIndex = position;
                select(newBlock());
                const auto pointer = Result.pointer(record);
                const auto parameters = Result.functions[position].parameters;
                auto left = load(Operand{OperandKind::Object, Result.pointer(pointer), parameters[0]}, pointer);
                auto right = load(Operand{OperandKind::Object, Result.pointer(pointer), parameters[1]}, pointer);
                const auto unequal = newBlock();
                compareFields(record, left, right, unequal);
                emit(Opcode::Return, {}, integer(boolean, 1));
                select(unequal);
                emit(Opcode::Return, {}, integer(boolean, 0));
            }
            // Fill slot 1 of every class that has an equality function; other classes keep null.
            for (auto [record, function] : EqualityFunctions) {
                auto table = Vtables.find(record);
                if (table == Vtables.end()) {
                    continue;
                }
                const uint64_t offset = semantic::EqualitySlot * Result.type(vtableSlot()).size;
                for (auto& global : Result.globals) {
                    if (global.id != table->second) {
                        continue;
                    }
                    auto& values = global.initialValues; // keep entries ordered by offset
                    auto position = std::find_if(values.begin(), values.end(),
                                                 [&](const Object::InitialValue& entry) { return entry.offset > offset; });
                    values.insert(position, {offset, callable(function)});
                }
            }
        }

        // The address a record comparison reads: an lvalue in place (so a class keeps its dynamic
        // vptr), an rvalue spilled to a fresh local. `exact` reports whether the operand's dynamic
        // class is certainly its static one.
        Operand Lowerer::comparand(const types::ExpressionNode* expression, bool& exact) {
            if (expression->semanticInfo().LValue) {
                exact = exactType(expression);
                return address(expression);
            }
            exact = true;
            auto result = value(expression);
            auto slot = storage(result.type, ".compared");
            store(slot, result);
            return slot;
        }

        // Lowers `a == b` / `a != b` on records. Semantics already required structs to have the same
        // type. Classes: different static types that are not base/derived are always unequal; otherwise
        // the dynamic classes must match (decided at compile time for exact operands, else by vptr) and
        // the matching class's equality function compares every field, called directly when the class
        // is known and through vtable slot 1 when it is not.
        Operand Lowerer::recordEquality(const types::BinaryExpressionNode* node) {
            const bool negate = node->getBinaryOp() == types::OP_NEQ;
            const auto resultType = type(node);
            const auto& types = Analyzer.types();
            bool leftExact = false, rightExact = false;
            auto left = comparand(node->getLeft(), leftExact);
            auto right = comparand(node->getRight(), rightExact);
            const auto L = types.unConst({node->getLeft()->semanticInfo().Type});
            const auto R = types.unConst({node->getRight()->semanticInfo().Type});
            auto finish = [&](Operand same) {
                return negate ? emitValue(Opcode::LogicalNot, resultType, same) : same;
            };
            auto unequal = integer(resultType, negate ? 1 : 0);
            semantic::TypeId common;
            if (L == R) {
                common = L;
            } else if (types.derivesFrom(L, R)) {
                common = R;
            } else if (types.derivesFrom(R, L)) {
                common = L;
            } else {
                return unequal; // unrelated classes; operands were still evaluated
            }
            const auto record = importType(common);
            if (!polymorphic(record)) {
                return finish(invoke(equalityFunction(record), {left, right}));
            }
            const TypeId leftClass = leftExact ? importType(L) : 0, rightClass = rightExact ? importType(R) : 0;
            if (leftClass && rightClass) {
                if (leftClass != rightClass) {
                    return unequal;
                }
                const auto pointer = Result.pointer(leftClass);
                return finish(invoke(equalityFunction(leftClass), {cast(left, pointer), cast(right, pointer)}));
            }
            const auto table = Result.pointer(vtableSlot());
            auto leftTable = leftClass ? vtableAddress(leftClass) : load(vptrAddress(left), table);
            auto rightTable = rightClass ? vtableAddress(rightClass) : load(vptrAddress(right), table);
            auto result = temporary(resultType);
            auto same = newBlock(), different = newBlock(), join = newBlock();
            branch(emitValue(Opcode::Equal, resultType, leftTable, rightTable), same, different);
            select(same);
            Operand fields;
            if (auto known = leftClass ? leftClass : rightClass) {
                const auto pointer = Result.pointer(known);
                fields = invoke(equalityFunction(known), {cast(left, pointer), cast(right, pointer)});
            } else {
                EqualityDispatch.insert(record);
                const auto pointer = Result.pointer(record);
                const auto signature = Result.function(resultType, {pointer, pointer}, false);
                auto callee = virtualCallee(cast(left, pointer), semantic::EqualitySlot, signature);
                fields = temporary(resultType);
                emitCall(fields, callee, {cast(left, pointer), cast(right, pointer)});
            }
            emit(Opcode::Copy, result, finish(fields));
            jump(join);
            select(different);
            emit(Opcode::Copy, result, unequal);
            jump(join);
            select(join);
            return result;
        }
    }
}
