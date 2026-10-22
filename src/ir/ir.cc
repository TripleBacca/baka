#include "ir.hh"
#include "base/target.hh"
#include <algorithm>
#include <stdexcept>

namespace baka {
    namespace ir {
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
            type.size = type.alignment = target::PointerBytes;
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

        namespace {
            template <class T>
            const T* indexed(const std::vector<T>& values, uint32_t id, std::unordered_map<uint32_t, size_t>& index,
                             size_t& count) {
                auto found = index.find(id);
                if (count == values.size() && found == index.end()) {
                    return nullptr;
                }
                if (count == values.size() && found != index.end() && found->second < values.size() &&
                    values[found->second].id == id) {
                    return &values[found->second];
                }
                index.clear();
                for (size_t i = 0; i < values.size(); ++i) {
                    index.try_emplace(values[i].id, i);
                }
                count = values.size();
                found = index.find(id);
                return found == index.end() ? nullptr : &values[found->second];
            }
        }

        const Object* Module::object(const Function& function, uint32_t id) const {
            if (auto* local = indexed(function.objects, id, function.objectIndex, function.indexedObjectCount)) {
                return local;
            }
            return indexed(globals, id, globalIndex, indexedGlobalCount);
        }

        const Function* Module::functionById(uint32_t id) const {
            return indexed(functions, id, functionIndex, indexedFunctionCount);
        }

        bool IsTerminator(Opcode opcode) {
            return opcode == Opcode::Jump || opcode == Opcode::Branch || opcode == Opcode::Return;
        }

        bool DefinesTemporary(const Instruction& instruction) {
            return instruction.result.kind == OperandKind::Temporary && instruction.opcode != Opcode::Store &&
                   instruction.opcode != Opcode::ZeroMemory;
        }

        std::vector<Operand> Uses(const Instruction& instruction) {
            std::vector<Operand> result;
            if (instruction.left) {
                result.push_back(instruction.left);
            }
            if (instruction.right) {
                result.push_back(instruction.right);
            }
            if (instruction.opcode == Opcode::Store || instruction.opcode == Opcode::ZeroMemory) {
                result.push_back(instruction.result);
            }
            result.insert(result.end(), instruction.arguments.begin(), instruction.arguments.end());
            return result;
        }
    }
}
