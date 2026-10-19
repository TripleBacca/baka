#include "ir.hh"
#include <algorithm>
#include <stdexcept>

namespace baka::ir {
    TypeId Module::pointer(TypeId pointee) {
        for (TypeId i = 1; i < types.size(); ++i) {
            if (types[i].kind == TypeKind::Pointer && types[i].element == pointee) {
                return i;
            }
        }
        Type type;
        type.kind = TypeKind::Pointer;
        type.complete = true;
        type.element = pointee;
        type.size = type.alignment = 8;
        types.push_back(std::move(type));
        return static_cast<TypeId>(types.size() - 1);
    }

    TypeId Module::function(TypeId result, std::vector<TypeId> parameters, bool variadic) {
        Type type;
        type.kind = TypeKind::Function;
        type.element = result;
        type.parameters = std::move(parameters);
        type.variadic = variadic;
        for (TypeId i = 1; i < types.size(); ++i) {
            if (types[i] == type) {
                return i;
            }
        }
        types.push_back(std::move(type));
        return static_cast<TypeId>(types.size() - 1);
    }

    const Type& Module::type(TypeId id) const {
        return types.at(id);
    }

    const Object* Module::object(const Function& function, uint32_t id) const {
        for (const auto& object : function.objects) {
            if (object.id == id) {
                return &object;
            }
        }
        for (const auto& object : globals) {
            if (object.id == id) {
                return &object;
            }
        }
        return nullptr;
    }

    const Function* Module::functionById(uint32_t id) const {
        for (const auto& function : functions) {
            if (function.id == id) {
                return &function;
            }
        }
        return nullptr;
    }

    bool IsTerminator(Opcode opcode) {
        return opcode == Opcode::Jump || opcode == Opcode::Branch || opcode == Opcode::Return;
    }

    bool DefinesTemporary(const Instruction& instruction) {
        return instruction.result.kind == OperandKind::Temporary && instruction.opcode != Opcode::Store;
    }

    std::vector<Operand> Uses(const Instruction& instruction) {
        std::vector<Operand> result;
        if (instruction.left) {
            result.push_back(instruction.left);
        }
        if (instruction.right) {
            result.push_back(instruction.right);
        }
        if (instruction.opcode == Opcode::Store) {
            result.push_back(instruction.result);
        }
        return result;
    }
}
