#include "lowering.hh"

namespace baka {
    namespace ir {
        Operand Lowerer::conditionalValue(const types::TernaryOpNode* ternary) {
            const auto resultType = type(ternary);
            Operand direct;
            if (Result.type(resultType).kind == TypeKind::Record) {
                if (auto location = destination(ternary)) {
                    direct = location;
                    State.DirectDestinations[valueRoot(ternary->getThen())] = location;
                    State.DirectDestinations[valueRoot(ternary->getElse())] = location;
                }
            }
            auto condition = value(ternary->getCondition());
            auto yes = newBlock(), no = newBlock(), join = newBlock();
            auto result = Result.type(resultType).kind == TypeKind::Void ? Operand{} : temporary(resultType);
            auto ownedAddress =
                needsDestruction(resultType) && !direct ? temporary(Result.pointer(resultType)) : Operand{};
            branch(condition, yes, no);
            select(yes);
            auto left = value(ternary->getThen());
            if (ownedAddress) {
                if (State.ValueOwners[left.id].empty()) {
                    left = ownTemporary(left);
                }
                emit(Opcode::Copy, ownedAddress, State.ValueLocations.at(left.id));
            }
            if (result) {
                emit(Opcode::Copy, result, left);
            }
            jump(join);
            select(no);
            auto right = value(ternary->getElse());
            if (ownedAddress) {
                if (State.ValueOwners[right.id].empty()) {
                    right = ownTemporary(right);
                }
                emit(Opcode::Copy, ownedAddress, State.ValueLocations.at(right.id));
            }
            if (result) {
                emit(Opcode::Copy, result, right);
            }
            jump(join);
            select(join);
            if (result) {
                if (ownedAddress) {
                    State.ValueLocations[result.id] = ownedAddress;
                } else if (direct) {
                    State.ValueLocations[result.id] = direct;
                }
                auto& owners = State.ValueOwners[result.id];
                if (left.kind == OperandKind::Temporary) {
                    owners = State.ValueOwners[left.id];
                }
                if (right.kind == OperandKind::Temporary) {
                    const auto other = State.ValueOwners[right.id];
                    owners.insert(owners.end(), other.begin(), other.end());
                }
            }
            return result;
        }

        Operand Lowerer::conditionalAddress(const types::TernaryOpNode* ternary) {
            auto condition = value(ternary->getCondition());
            auto yes = newBlock(), no = newBlock(), join = newBlock();
            auto pointer = Result.pointer(type(ternary));
            auto result = temporary(pointer);
            branch(condition, yes, no);
            select(yes);
            auto left = address(ternary->getThen());
            emit(Opcode::Copy, result, cast(left, pointer));
            jump(join);
            select(no);
            auto right = address(ternary->getElse());
            emit(Opcode::Copy, result, cast(right, pointer));
            jump(join);
            select(join);
            return result;
        }
    }
}
