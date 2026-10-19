#include "ir.hh"
#include <algorithm>
#include <map>
#include <set>

namespace baka::ir {
    std::vector<Diagnostic> Verify(const Module& module) {
        std::vector<Diagnostic> errors;
        auto error = [&](std::string message, SourceSpan source = {}) {
            errors.push_back({std::move(message), source});
        };
        auto validType = [&](TypeId id) {
            return id && id < module.types.size() && module.types[id].kind != TypeKind::Invalid;
        };
        auto scalar = [&](TypeId id) {
            if (!validType(id)) {
                return false;
            }
            auto kind = module.type(id).kind;
            return kind == TypeKind::Integer || kind == TypeKind::Floating || kind == TypeKind::Pointer;
        };
        for (TypeId id = 1; id < module.types.size(); ++id) {
            const auto& type = module.type(id);
            if ((type.kind == TypeKind::Pointer || type.kind == TypeKind::Array || type.kind == TypeKind::Function) &&
                !validType(type.element)) {
                error("invalid derived IR type");
            }
            for (auto parameter : type.parameters) {
                if (!validType(parameter)) {
                    error("invalid IR parameter type");
                }
            }
            if (type.kind == TypeKind::Integer && type.bits != 1 && type.bits != 8 && type.bits != 16 &&
                type.bits != 32 && type.bits != 64) {
                error("invalid IR integer width");
            }
            if (type.kind == TypeKind::Floating && type.bits != 32 && type.bits != 64) {
                error("invalid IR floating width");
            }
        }
        // All later queries may safely dereference derived types now.
        if (!errors.empty()) {
            return errors;
        }
        auto storageType = [&](TypeId id) {
            if (!validType(id)) {
                return false;
            }
            const auto& type = module.type(id);
            return scalar(id) || ((type.kind == TypeKind::Record || type.kind == TypeKind::Array) && type.complete);
        };
        std::set<uint32_t> functionIds, objectIds;
        for (const auto& global : module.globals) {
            if (!global.id || !objectIds.insert(global.id).second || !storageType(global.type)) {
                error("invalid or duplicate global object");
            }
            if (global.external && (!global.initialValues.empty() || !global.bytes.empty())) {
                error("external object has an initializer");
            }
            if (validType(global.type)) {
                auto size = module.type(global.type).size;
                if (global.bytes.size() > size) {
                    error("string initializer exceeds object size");
                }
                for (const auto& value : global.initialValues) {
                    if (!validType(value.value.type)) {
                        error("invalid global initializer type");
                        continue;
                    }
                    if (!scalar(value.value.type) || value.offset > size ||
                        module.type(value.value.type).size > size - value.offset) {
                        error("global initializer exceeds object size or has invalid type");
                    }
                    if (value.value.kind != OperandKind::Integer && value.value.kind != OperandKind::Floating &&
                        value.value.kind != OperandKind::Function && value.value.kind != OperandKind::Object) {
                        error("global initializer is not a constant or relocation");
                    }
                    if (value.value.kind == OperandKind::Object) {
                        bool exists = false;
                        for (const auto& object : module.globals) {
                            if (object.id == value.value.id) {
                                exists = true;
                            }
                        }
                        if (!exists || (module.type(value.value.type).kind != TypeKind::Pointer &&
                                        module.type(value.value.type).kind != TypeKind::Integer)) {
                            error("invalid object relocation");
                        }
                    }
                    if (value.value.kind == OperandKind::Function &&
                        (!module.functionById(value.value.id) ||
                         (module.type(value.value.type).kind != TypeKind::Pointer &&
                          module.type(value.value.type).kind != TypeKind::Integer))) {
                        error("invalid function relocation");
                    }
                }
            }
        }
        for (const auto& function : module.functions) {
            if (!function.id || !functionIds.insert(function.id).second || !validType(function.signature) ||
                module.type(function.signature).kind != TypeKind::Function) {
                error("invalid or duplicate function signature");
                continue;
            }
            const auto& signature = module.type(function.signature);
            if (signature.element && module.type(signature.element).kind != TypeKind::Void &&
                !scalar(signature.element)) {
                error("aggregate function return is unsupported");
            }
            for (auto parameter : signature.parameters) {
                if (!scalar(parameter)) {
                    error("aggregate function parameter is unsupported");
                }
            }
            if (!function.defined) {
                if (!function.blocks.empty()) {
                    error("declaration contains blocks");
                }
                continue;
            }
            if (function.blocks.empty()) {
                error("defined function has no entry block");
                continue;
            }
            if (function.parameters.size() != signature.parameters.size()) {
                error("function parameter count mismatch");
            }
            for (size_t i = 0; i < function.parameters.size(); ++i) {
                auto* object = module.object(function, function.parameters[i]);
                if (!object || i >= signature.parameters.size() || object->type != signature.parameters[i]) {
                    error("function parameter type mismatch");
                }
            }
            for (const auto& object : function.objects) {
                if (!object.id || !objectIds.insert(object.id).second || !storageType(object.type)) {
                    error("invalid or duplicate local object");
                }
            }
            std::map<BlockId, const Block*> blocks;
            for (const auto& block : function.blocks) {
                if (!block.id || !blocks.emplace(block.id, &block).second) {
                    error("duplicate/invalid block ID");
                }
            }
            Function expected = function;
            BuildCFG(expected);
            std::set<uint32_t> universe;
            for (uint32_t i = 1; i < function.temporaries.size(); ++i) {
                universe.insert(i);
                if (!scalar(function.temporaries[i])) {
                    error("temporary is not a scalar");
                }
            }
            for (size_t bi = 0; bi < function.blocks.size(); ++bi) {
                const auto& block = function.blocks[bi];
                if (block.predecessors != expected.blocks[bi].predecessors ||
                    block.successors != expected.blocks[bi].successors) {
                    error("CFG edge lists disagree with terminators");
                }
                if (block.instructions.empty() || !IsTerminator(block.instructions.back().opcode)) {
                    error("block lacks terminator");
                }
                std::vector<Operand> arguments;
                auto checkOperand = [&](Operand operand, SourceSpan source) {
                    if (!operand) {
                        return;
                    }
                    if (operand.kind == OperandKind::Block) {
                        if (operand.type || !blocks.count(operand.id)) {
                            error("invalid branch destination", source);
                        }
                        return;
                    }
                    if (!validType(operand.type)) {
                        error("invalid operand type", source);
                        return;
                    }
                    if (operand.kind == OperandKind::Temporary &&
                        (!operand.id || operand.id >= function.temporaries.size() ||
                         function.temporaries[operand.id] != operand.type)) {
                        error("invalid temporary identity/type", source);
                    }
                    if (operand.kind == OperandKind::Object) {
                        auto* object = module.object(function, operand.id);
                        if (!object || module.type(operand.type).kind != TypeKind::Pointer ||
                            module.type(operand.type).element != object->type) {
                            error("invalid object address type", source);
                        }
                    }
                    if (operand.kind == OperandKind::Function) {
                        auto* target = module.functionById(operand.id);
                        if (!target || module.type(operand.type).kind != TypeKind::Pointer ||
                            module.type(operand.type).element != target->signature) {
                            error("invalid function address type", source);
                        }
                    }
                    if (operand.kind == OperandKind::Integer && module.type(operand.type).kind != TypeKind::Integer &&
                        module.type(operand.type).kind != TypeKind::Pointer) {
                        error("integer literal has invalid type", source);
                    }
                    if (operand.kind == OperandKind::Integer && module.type(operand.type).kind == TypeKind::Integer &&
                        module.type(operand.type).bits < 64 && (operand.integer >> module.type(operand.type).bits)) {
                        error("integer literal exceeds its declared width", source);
                    }
                    if (operand.kind == OperandKind::Floating && module.type(operand.type).kind != TypeKind::Floating) {
                        error("floating literal has invalid type", source);
                    }
                };
                for (size_t i = 0; i < block.instructions.size(); ++i) {
                    const auto& in = block.instructions[i];
                    if (static_cast<unsigned>(in.opcode) > static_cast<unsigned>(Opcode::Return)) {
                        error("invalid IR opcode", in.source);
                        continue;
                    }
                    checkOperand(in.result, in.source);
                    checkOperand(in.left, in.source);
                    checkOperand(in.right, in.source);
                    if (IsTerminator(in.opcode) && i + 1 != block.instructions.size()) {
                        error("instruction follows terminator", in.source);
                    }
                    if (in.opcode != Opcode::Argument && in.opcode != Opcode::Call && !arguments.empty()) {
                        error("argument sequence interrupted before call", in.source);
                        arguments.clear();
                    }
                    // Skip shape checks with invalid operand types; the error above
                    // is sufficient and protects all subsequent type-table accesses.
                    if ((in.result && in.result.kind != OperandKind::Block && !validType(in.result.type)) ||
                        (in.left && in.left.kind != OperandKind::Block && !validType(in.left.type)) ||
                        (in.right && in.right.kind != OperandKind::Block && !validType(in.right.type))) {
                        continue;
                    }
                    auto require = [&](bool condition, const char* message) {
                        if (!condition) {
                            error(message, in.source);
                        }
                    };
                    auto temp = [&] { return in.result.kind == OperandKind::Temporary; };
                    auto pointerTo = [&](Operand address, TypeId value) {
                        return address && module.type(address.type).kind == TypeKind::Pointer &&
                               module.type(address.type).element == value;
                    };
                    switch (in.opcode) {
                    case Opcode::Copy:
                        require(temp() && scalar(in.left.type) && in.result.type == in.left.type && !in.right,
                                "invalid copy");
                        break;
                    case Opcode::Load:
                        require(temp() && scalar(in.result.type) && pointerTo(in.left, in.result.type) && !in.right,
                                "invalid load");
                        break;
                    case Opcode::Store:
                        require(scalar(in.left.type) && pointerTo(in.result, in.left.type) && !in.right,
                                "invalid store");
                        break;
                    case Opcode::Cast:
                        require(temp() && scalar(in.left.type) && scalar(in.result.type) && !in.right,
                                "invalid scalar cast");
                        break;
                    case Opcode::AddressOffset:
                        require(temp() && module.type(in.result.type).kind == TypeKind::Pointer && in.left &&
                                    module.type(in.left.type).kind == TypeKind::Pointer && in.right &&
                                    module.type(in.right.type).kind == TypeKind::Integer,
                                "invalid byte-offset operation");
                        break;
                    case Opcode::Argument:
                        require(!in.result && scalar(in.left.type) && in.right.kind == OperandKind::Integer &&
                                    in.right.integer == arguments.size(),
                                "invalid argument sequence/index");
                        arguments.push_back(in.left);
                        break;
                    case Opcode::Call: {
                        auto pointer = in.left ? module.type(in.left.type) : Type{};
                        auto target = pointer.kind == TypeKind::Pointer ? module.type(pointer.element) : Type{};
                        require(target.kind == TypeKind::Function && in.right.kind == OperandKind::Integer &&
                                    in.right.integer == arguments.size(),
                                "invalid call target/count");
                        if (target.kind == TypeKind::Function) {
                            require(arguments.size() >= target.parameters.size() &&
                                        (target.variadic || arguments.size() == target.parameters.size()),
                                    "call arity mismatch");
                            for (size_t a = 0; a < arguments.size() && a < target.parameters.size(); ++a) {
                                require(arguments[a].type == target.parameters[a], "call argument type mismatch");
                            }
                            require(module.type(target.element).kind == TypeKind::Void
                                        ? !in.result
                                        : temp() && in.result.type == target.element,
                                    "call result mismatch");
                        }
                        arguments.clear();
                        break;
                    }
                    case Opcode::Jump:
                        require(!in.result && in.left.kind == OperandKind::Block && !in.right, "invalid jump");
                        break;
                    case Opcode::Branch:
                        require(in.result.kind == OperandKind::Block && scalar(in.left.type) &&
                                    in.right.kind == OperandKind::Block,
                                "invalid branch");
                        break;
                    case Opcode::Return:
                        require(!in.result && !in.right &&
                                    (module.type(signature.element).kind == TypeKind::Void
                                         ? !in.left
                                         : in.left.type == signature.element && scalar(in.left.type)),
                                "return type mismatch");
                        break;
                    case Opcode::Negate:
                    case Opcode::Complement:
                    case Opcode::LogicalNot: {
                        auto kind = module.type(in.left.type).kind;
                        bool valid = in.opcode == Opcode::LogicalNot ? scalar(in.left.type)
                                     : in.opcode == Opcode::Complement
                                         ? kind == TypeKind::Integer
                                         : kind == TypeKind::Integer || kind == TypeKind::Floating;
                        require(temp() && valid && !in.right &&
                                    (in.opcode == Opcode::LogicalNot
                                         ? module.type(in.result.type).kind == TypeKind::Integer
                                         : in.result.type == in.left.type),
                                "invalid unary operation");
                        break;
                    }
                    default: {
                        bool comparison = in.opcode >= Opcode::Equal && in.opcode <= Opcode::GreaterEqual;
                        bool shift = in.opcode == Opcode::ShiftLeft || in.opcode == Opcode::ShiftRight;
                        auto leftKind = module.type(in.left.type).kind, rightKind = module.type(in.right.type).kind;
                        bool integral = shift || in.opcode == Opcode::Remainder ||
                                        (in.opcode >= Opcode::BitAnd && in.opcode <= Opcode::BitXor);
                        bool operands = comparison ? scalar(in.left.type) && scalar(in.right.type)
                                        : integral
                                            ? leftKind == TypeKind::Integer && rightKind == TypeKind::Integer
                                            : (leftKind == TypeKind::Integer || leftKind == TypeKind::Floating) &&
                                                  (rightKind == TypeKind::Integer || rightKind == TypeKind::Floating);
                        require(temp() && operands && (shift || in.left.type == in.right.type) &&
                                    (comparison ? module.type(in.result.type).kind == TypeKind::Integer
                                                : in.result.type == in.left.type),
                                "invalid binary operation");
                        break;
                    }
                    }
                }
                if (!arguments.empty()) {
                    error("dangling call arguments");
                }
            }
            // Definite assignment for mutable temporaries, including branch joins.
            // Object memory may be uninitialized according to frontend policy;
            // compiler-generated temporaries must always have a dominating write.
            std::set<BlockId> reachable;
            std::vector<BlockId> pending{function.blocks.front().id};
            while (!pending.empty()) {
                auto id = pending.back();
                pending.pop_back();
                if (!reachable.insert(id).second || !blocks.count(id)) {
                    continue;
                }
                for (auto successor : blocks.at(id)->successors) {
                    pending.push_back(successor);
                }
            }
            std::map<BlockId, std::set<uint32_t>> incoming, outgoing;
            for (auto id : reachable) {
                incoming[id] = outgoing[id] = universe;
            }
            incoming[function.blocks.front().id] = {};
            bool changed = true;
            while (changed) {
                changed = false;
                for (const auto& block : function.blocks) {
                    if (!reachable.count(block.id)) {
                        continue;
                    }
                    std::set<uint32_t> in;
                    if (block.id != function.blocks.front().id) {
                        in = universe;
                        for (auto predecessor : block.predecessors) {
                            if (!reachable.count(predecessor)) {
                                continue;
                            }
                            std::set<uint32_t> intersection;
                            std::set_intersection(in.begin(), in.end(), outgoing[predecessor].begin(),
                                                  outgoing[predecessor].end(),
                                                  std::inserter(intersection, intersection.begin()));
                            in = std::move(intersection);
                        }
                    }
                    auto out = in;
                    for (const auto& instruction : block.instructions) {
                        if (DefinesTemporary(instruction)) {
                            out.insert(instruction.result.id);
                        }
                    }
                    if (incoming[block.id] != in || outgoing[block.id] != out) {
                        changed = true;
                    }
                    incoming[block.id] = std::move(in);
                    outgoing[block.id] = std::move(out);
                }
            }
            for (const auto& block : function.blocks) {
                if (!reachable.count(block.id)) {
                    continue;
                }
                auto assigned = incoming[block.id];
                for (const auto& instruction : block.instructions) {
                    for (auto operand : Uses(instruction)) {
                        if (operand.kind == OperandKind::Temporary && !assigned.count(operand.id)) {
                            error("temporary used before definition", instruction.source);
                        }
                    }
                    if (DefinesTemporary(instruction)) {
                        assigned.insert(instruction.result.id);
                    }
                }
            }
        }
        return errors;
    }
}
