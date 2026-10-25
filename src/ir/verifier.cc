#include "ir.hh"
#include <algorithm>
#include <map>
#include <set>

namespace baka {
    namespace ir {
        namespace {
            class Verifier {
                using BlockMap = std::map<BlockId, const Block*>;

                const Module& module;
                std::vector<Diagnostic> errors;
                std::set<uint32_t> functionIds, objectIds;
                std::vector<unsigned char> colors;

                void error(std::string message, SourceSpan source = {});
                void require(const Instruction& instruction, bool condition, const char* message);
                bool validType(TypeId id) const;
                bool scalar(TypeId id) const;
                bool storageType(TypeId id) const;
                bool valueType(TypeId id) const;
                bool signatureValueType(TypeId id, bool definition) const;
                bool pointerTo(Operand address, TypeId value) const;
                bool acyclic(TypeId id);
                bool acyclicStorage(TypeId id, std::vector<unsigned char>& storageColors);
                void checkTypes();
                void checkInitializer();
                void checkGlobals();
                void checkFunction(const Function& function);
                void checkOperand(Operand operand, SourceSpan source, const Function& function, const BlockMap& blocks);
                void checkInstruction(const Instruction& instruction, const Function& function, const BlockMap& blocks,
                                      bool last);
                void checkShape(const Instruction& instruction, TypeId resultType);
                void checkDefinitions(const Function& function, const BlockMap& blocks,
                                      const std::set<uint32_t>& universe);

            public:
                explicit Verifier(const Module& module) : module(module), colors(module.types.size()) {}

                std::vector<Diagnostic> check();
            };

            void Verifier::error(std::string message, SourceSpan source) {
                errors.push_back({std::move(message), source});
            }

            void Verifier::require(const Instruction& instruction, bool condition, const char* message) {
                if (!condition) {
                    error(message, instruction.source);
                }
            }

            bool Verifier::pointerTo(Operand address, TypeId value) const {
                return address && module.type(address.type).kind == TypeKind::Pointer &&
                       module.type(address.type).element == value;
            }

            std::vector<Diagnostic> Verifier::check() {
                module.invalidateIndexes();
                checkTypes();
                if (!errors.empty()) {
                    return errors;
                }
                checkInitializer();
                checkGlobals();
                for (const auto& function : module.functions) {
                    checkFunction(function);
                }
                return errors;
            }

            bool Verifier::validType(TypeId id) const {
                return id && id < module.types.size() && module.types[id].kind != TypeKind::Invalid &&
                       static_cast<unsigned>(module.types[id].kind) <= static_cast<unsigned>(TypeKind::Record);
            }

            bool Verifier::scalar(TypeId id) const {
                if (!validType(id)) {
                    return false;
                }
                auto kind = module.type(id).kind;
                return kind == TypeKind::Integer || kind == TypeKind::Floating || kind == TypeKind::Pointer;
            }

            bool Verifier::storageType(TypeId id) const {
                if (!validType(id)) {
                    return false;
                }
                const auto& type = module.type(id);
                return scalar(id) || ((type.kind == TypeKind::Record || type.kind == TypeKind::Array) && type.complete);
            }

            bool Verifier::valueType(TypeId id) const {
                return scalar(id) ||
                       (validType(id) && module.type(id).kind == TypeKind::Record && module.type(id).complete);
            }

            bool Verifier::signatureValueType(TypeId id, bool definition) const {
                return valueType(id) || (!definition && validType(id) && module.type(id).kind == TypeKind::Record);
            }

            void Verifier::checkTypes() {
                if (module.types.empty() || module.types.front().kind != TypeKind::Invalid) {
                    error("missing invalid-type sentinel");
                    return;
                }
                for (TypeId id = 1; id < module.types.size(); ++id) {
                    const auto& type = module.type(id);
                    if (!validType(id)) {
                        error("invalid type kind");
                        continue;
                    }
                    if (type.kind != TypeKind::Record && (!type.memberOffsets.empty() || !type.memberTypes.empty() ||
                                                          type.baseOffset || type.base || type.isUnion)) {
                        error("record offsets attached to a non-record type");
                    }
                    if (type.kind == TypeKind::Record) {
                        if (!type.complete && (!type.memberOffsets.empty() || !type.memberTypes.empty() ||
                                               type.baseOffset || type.base)) {
                            error("incomplete record has stored offsets");
                        }
                        if (type.memberTypes.size() != type.memberOffsets.size()) {
                            error("record field types and offsets disagree");
                        }
                        for (size_t i = 0; i < type.memberOffsets.size(); ++i) {
                            const auto offset = type.memberOffsets[i];
                            if (offset && *offset >= type.size) {
                                error("stored member offset exceeds record size");
                            }
                            if (i >= type.memberTypes.size()) {
                                continue;
                            }
                            const auto field = type.memberTypes[i];
                            if (bool(offset) != bool(field)) {
                                error("record field types and offsets disagree");
                            } else if (field) {
                                if (!storageType(field)) {
                                    error("invalid record field type");
                                } else if (offset && *offset <= type.size &&
                                           module.type(field).size > type.size - *offset) {
                                    error("record field exceeds record size");
                                }
                            }
                        }
                        if (type.baseOffset && *type.baseOffset >= type.size) {
                            error("stored base offset exceeds record size");
                        }
                        if (bool(type.base) != bool(type.baseOffset)) {
                            error("record base type and offset disagree");
                        } else if (type.base) {
                            if (!validType(type.base) || module.type(type.base).kind != TypeKind::Record ||
                                !module.type(type.base).complete || type.isUnion) {
                                error("invalid record base type");
                            } else if (*type.baseOffset <= type.size &&
                                       module.type(type.base).size > type.size - *type.baseOffset) {
                                error("record base exceeds record size");
                            }
                        }
                    }
                    if ((type.kind == TypeKind::Pointer || type.kind == TypeKind::Array ||
                         type.kind == TypeKind::Function) &&
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
                if (!errors.empty()) {
                    return;
                }
                // A structural cycle must pass through a nominal record.
                for (TypeId id = 1; id < module.types.size(); ++id) {
                    if (!acyclic(id)) {
                        error("cyclic derived type graph");
                        return;
                    }
                }
                // Named records may recurse through pointers, but their stored
                // fields and bases must never contain themselves by value.
                std::vector<unsigned char> storageColors(module.types.size());
                for (TypeId id = 1; id < module.types.size(); ++id) {
                    if (!acyclicStorage(id, storageColors)) {
                        error("recursive by-value record layout");
                        return;
                    }
                }
            }

            bool Verifier::acyclicStorage(TypeId id, std::vector<unsigned char>& storageColors) {
                if (storageColors[id] == 1) {
                    return false;
                }
                if (storageColors[id] == 2) {
                    return true;
                }
                storageColors[id] = 1;
                const auto& type = module.type(id);
                if (type.kind == TypeKind::Array && !acyclicStorage(type.element, storageColors)) {
                    return false;
                }
                if (type.kind == TypeKind::Record) {
                    if (type.base && !acyclicStorage(type.base, storageColors)) {
                        return false;
                    }
                    for (auto field : type.memberTypes) {
                        if (field && !acyclicStorage(field, storageColors)) {
                            return false;
                        }
                    }
                }
                storageColors[id] = 2;
                return true;
            }

            bool Verifier::acyclic(TypeId id) {
                if (colors[id] == 1) {
                    return false;
                }
                if (colors[id] == 2) {
                    return true;
                }
                colors[id] = 1;
                const auto& type = module.type(id);
                if (type.kind == TypeKind::Pointer || type.kind == TypeKind::Array || type.kind == TypeKind::Function) {
                    if (!acyclic(type.element)) {
                        return false;
                    }
                    for (auto parameter : type.parameters) {
                        if (!acyclic(parameter)) {
                            return false;
                        }
                    }
                }
                colors[id] = 2;
                return true;
            }

            void Verifier::checkInitializer() {
                if (module.initializer) {
                    const auto* startup = module.functionById(module.initializer);
                    if (!startup || !startup->defined || !startup->internal || !validType(startup->signature) ||
                        module.type(startup->signature).kind != TypeKind::Function) {
                        error("invalid module initializer function");
                    } else {
                        const auto& signature = module.type(startup->signature);
                        if (module.type(signature.element).kind != TypeKind::Void || !signature.parameters.empty() ||
                            signature.variadic) {
                            error("module initializer must have signature fn()->void");
                        }
                    }
                }
            }

            void Verifier::checkGlobals() {
                for (const auto& global : module.globals) {
                    if (!global.id || !objectIds.insert(global.id).second || !storageType(global.type)) {
                        error("invalid or duplicate global object");
                    }
                    if (global.external && (!global.initialValues.empty() || !global.bytes.empty())) {
                        error("external object has an initializer");
                    }
                    if (global.runtimeInitialized &&
                        (global.external || !global.initialValues.empty() || !global.bytes.empty())) {
                        error("runtime-initialized object must have zero-filled initial storage");
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
            }

            void Verifier::checkFunction(const Function& function) {
                if (!function.id || !functionIds.insert(function.id).second || !validType(function.signature) ||
                    module.type(function.signature).kind != TypeKind::Function) {
                    error("invalid or duplicate function signature");
                    return;
                }
                const auto& signature = module.type(function.signature);
                if (signature.element && module.type(signature.element).kind != TypeKind::Void &&
                    !signatureValueType(signature.element, function.defined)) {
                    error("invalid function return value type");
                }
                for (auto parameter : signature.parameters) {
                    if (!signatureValueType(parameter, function.defined)) {
                        error("invalid function parameter value type");
                    }
                }
                if (!function.defined) {
                    if (!function.blocks.empty()) {
                        error("declaration contains blocks");
                    }
                    return;
                }
                if (function.blocks.empty()) {
                    error("defined function has no entry block");
                    return;
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
                    if (!valueType(function.temporaries[i])) {
                        error("temporary has invalid value type");
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
                    bool validArguments = true;
                    for (size_t i = 0; i < block.instructions.size(); ++i) {
                        const auto& in = block.instructions[i];
                        const auto errorsBefore = errors.size();
                        checkInstruction(in, function, blocks, i + 1 == block.instructions.size());
                        if (in.opcode == Opcode::Param) {
                            arguments.push_back(in.left);
                            validArguments &= errors.size() == errorsBefore;
                        } else if (in.opcode == Opcode::Call) {
                            if (validArguments && errors.size() == errorsBefore) {
                                const auto& target = module.type(module.type(in.left.type).element);
                                require(in,
                                        arguments.size() >= target.parameters.size() &&
                                            (target.variadic || arguments.size() == target.parameters.size()),
                                        "call arity mismatch");
                                for (size_t a = 0; a < arguments.size() && a < target.parameters.size(); ++a) {
                                    require(in, arguments[a].type == target.parameters[a],
                                            "call argument type mismatch");
                                }
                            }
                            arguments.clear();
                            validArguments = true;
                        } else if (!arguments.empty()) {
                            error("param sequence must immediately precede call", in.source);
                            arguments.clear();
                            validArguments = true;
                        }
                        if (in.opcode == Opcode::JumpIf &&
                            (i + 2 != block.instructions.size() || block.instructions.back().opcode != Opcode::Jump)) {
                            error("conditional jump must be followed by the block's final goto", in.source);
                        }
                    }
                    if (!arguments.empty()) {
                        error("param sequence lacks call", block.instructions.back().source);
                    }
                }
                checkDefinitions(function, blocks, universe);
            }

            void Verifier::checkOperand(Operand operand, SourceSpan source, const Function& function,
                                        const BlockMap& blocks) {
                if (operand.kind == OperandKind::None) {
                    if (operand.type || operand.id || operand.integer || operand.floating) {
                        error("nonempty absent operand", source);
                    }
                    return;
                }
                if (static_cast<unsigned>(operand.kind) > static_cast<unsigned>(OperandKind::Block)) {
                    error("invalid operand kind", source);
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
            }

            void Verifier::checkInstruction(const Instruction& in, const Function& function, const BlockMap& blocks,
                                            bool last) {
                if (static_cast<unsigned>(in.opcode) > static_cast<unsigned>(Opcode::Return)) {
                    error("invalid IR opcode", in.source);
                    return;
                }
                const auto errorsBeforeOperands = errors.size();
                checkOperand(in.result, in.source, function, blocks);
                checkOperand(in.left, in.source, function, blocks);
                checkOperand(in.right, in.source, function, blocks);
                if (errors.size() != errorsBeforeOperands) {
                    return;
                }
                if (IsTerminator(in.opcode) && !last) {
                    error("instruction follows terminator", in.source);
                }
                // Skip shape checks with invalid operand types; the error above
                // is sufficient and protects all subsequent type-table accesses.
                if ((in.result && in.result.kind != OperandKind::Block && !validType(in.result.type)) ||
                    (in.left && in.left.kind != OperandKind::Block && !validType(in.left.type)) ||
                    (in.right && in.right.kind != OperandKind::Block && !validType(in.right.type))) {
                    return;
                }
                checkShape(in, module.type(function.signature).element);
            }

            void Verifier::checkShape(const Instruction& in, TypeId resultType) {
                const bool resultTemporary = in.result.kind == OperandKind::Temporary;
                switch (in.opcode) {
                case Opcode::Copy:
                    require(in,
                            resultTemporary && valueType(in.left.type) && in.result.type == in.left.type && !in.right,
                            "invalid copy");
                    break;
                case Opcode::Load:
                    require(in,
                            resultTemporary && valueType(in.result.type) && pointerTo(in.left, in.result.type) &&
                                !in.right,
                            "invalid load");
                    break;
                case Opcode::Store:
                    require(in, !in.result && valueType(in.right.type) && pointerTo(in.left, in.right.type),
                            "invalid store");
                    break;
                case Opcode::ZeroMemory:
                    require(in,
                            !in.result && in.left && module.type(in.left.type).kind == TypeKind::Pointer && in.right &&
                                module.type(in.right.type).kind == TypeKind::Integer,
                            "invalid bulk zero operation");
                    break;
                case Opcode::Cast:
                    require(in, resultTemporary && scalar(in.left.type) && scalar(in.result.type) && !in.right,
                            "invalid scalar cast");
                    break;
                case Opcode::AddressOffset:
                    require(in,
                            resultTemporary && module.type(in.result.type).kind == TypeKind::Pointer && in.left &&
                                module.type(in.left.type).kind == TypeKind::Pointer && in.right &&
                                module.type(in.right.type).kind == TypeKind::Integer,
                            "invalid byte-offset operation");
                    break;
                case Opcode::Param:
                    require(in, !in.result && valueType(in.left.type) && !in.right, "invalid param");
                    break;
                case Opcode::Call: {
                    auto pointer = in.left ? module.type(in.left.type) : Type{};
                    auto target = pointer.kind == TypeKind::Pointer ? module.type(pointer.element) : Type{};
                    require(in, target.kind == TypeKind::Function && !in.right, "invalid call operands");
                    if (target.kind == TypeKind::Function) {
                        require(in,
                                module.type(target.element).kind == TypeKind::Void
                                    ? !in.result
                                    : resultTemporary && in.result.type == target.element,
                                "call result mismatch");
                    }
                    break;
                }
                case Opcode::RegisterDestructor: {
                    const auto pointer = module.type(in.left.type);
                    const auto target = pointer.kind == TypeKind::Pointer ? module.type(pointer.element) : Type{};
                    require(in,
                            !in.result && target.kind == TypeKind::Function &&
                                module.type(target.element).kind == TypeKind::Void && !target.variadic &&
                                target.parameters.size() == 1 && target.parameters.front() == in.right.type,
                            "invalid destructor registration");
                    break;
                }
                case Opcode::Jump:
                    require(in, !in.result && in.left.kind == OperandKind::Block && !in.right, "invalid jump");
                    break;
                case Opcode::JumpIf:
                    require(in, !in.result && scalar(in.left.type) && in.right.kind == OperandKind::Block,
                            "invalid conditional jump");
                    break;
                case Opcode::Return:
                    require(in,
                            !in.result && !in.right &&
                                (module.type(resultType).kind == TypeKind::Void
                                     ? !in.left
                                     : in.left.type == resultType && valueType(in.left.type)),
                            "return type mismatch");
                    break;
                case Opcode::Negate:
                case Opcode::Complement:
                case Opcode::LogicalNot: {
                    auto kind = module.type(in.left.type).kind;
                    bool valid;
                    if (in.opcode == Opcode::LogicalNot) {
                        valid = scalar(in.left.type);
                    } else if (in.opcode == Opcode::Complement) {
                        valid = kind == TypeKind::Integer;
                    } else {
                        valid = kind == TypeKind::Integer || kind == TypeKind::Floating;
                    }
                    require(in,
                            resultTemporary && valid && !in.right &&
                                (in.opcode == Opcode::LogicalNot ? module.type(in.result.type).kind == TypeKind::Integer
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
                    bool operands;
                    if (comparison) {
                        operands = scalar(in.left.type) && scalar(in.right.type);
                    } else if (integral) {
                        operands = leftKind == TypeKind::Integer && rightKind == TypeKind::Integer;
                    } else {
                        operands = (leftKind == TypeKind::Integer || leftKind == TypeKind::Floating) &&
                                   (rightKind == TypeKind::Integer || rightKind == TypeKind::Floating);
                    }
                    require(in,
                            resultTemporary && operands && (shift || in.left.type == in.right.type) &&
                                (comparison ? module.type(in.result.type).kind == TypeKind::Integer
                                            : in.result.type == in.left.type),
                            "invalid binary operation");
                    break;
                }
                }
            }

            void Verifier::checkDefinitions(const Function& function, const BlockMap& blocks,
                                            const std::set<uint32_t>& universe) {
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
        }

        std::vector<Diagnostic> Verify(const Module& module) {
            Verifier verifier(module);
            return verifier.check();
        }
    }
}
