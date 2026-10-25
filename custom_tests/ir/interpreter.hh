#pragma once
// Test-only executable specification of the TAC contracts. This is not a
// compiler backend: it lets tests compare optimized and unoptimized behavior.
#include "ir/ir.hh"
#include <bit>
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <stdexcept>
#include <unordered_map>

namespace ir_test {
    using namespace baka::ir;

    struct Value {
        uint64_t bits = 0;
        long double floating = 0;
        std::vector<unsigned char> record;
    };

    class Interpreter {
        const Module& ModuleValue;
        std::map<uint64_t, unsigned char> Memory;
        std::map<uint32_t, uint64_t> Globals;

        struct Allocation {
            uint64_t address, size;
            unsigned char unwritten;
        };

        std::vector<Allocation> Allocations;
        uint64_t NextAddress = 4096;
        size_t Steps = 0;
        bool Initialized = false;
        std::vector<std::pair<uint32_t, Value>> ExitActions;
        static constexpr uint64_t FunctionTag = uint64_t{1} << 60;

        struct Frame {
            std::map<uint32_t, uint64_t> addresses;
            std::map<uint32_t, Value> temporaries;
            std::vector<Value> arguments;
        };

        uint64_t allocate(TypeId type, bool zero = false) {
            auto address = NextAddress;
            const auto size = ModuleValue.type(type).size;
            Allocations.push_back({address, size, static_cast<unsigned char>(zero ? 0 : 0xa5)});
            NextAddress += size + 16;
            return address;
        }

        unsigned char readByte(uint64_t address) const {
            if (auto value = Memory.find(address); value != Memory.end()) {
                return value->second;
            }
            auto allocation = std::upper_bound(
                Allocations.begin(), Allocations.end(), address,
                [](uint64_t address, const Allocation& allocation) { return address < allocation.address; });
            if (allocation != Allocations.begin()) {
                --allocation;
                if (address - allocation->address < allocation->size) {
                    return allocation->unwritten;
                }
            }
            throw std::runtime_error("test interpreter: read outside object storage");
        }

        uint64_t mask(TypeId type) const {
            auto bits = ModuleValue.type(type).bits;
            return bits >= 64 ? ~uint64_t{0} : (uint64_t{1} << bits) - 1;
        }

        int64_t signedValue(Value value, TypeId type) const {
            auto bits = ModuleValue.type(type).bits;
            if (!bits || bits > 64) {
                throw std::runtime_error("test interpreter: expected an integer width");
            }
            value.bits &= mask(type);
            if (bits < 64 && (value.bits & (uint64_t{1} << (bits - 1)))) {
                value.bits |= ~mask(type);
            }
            return std::bit_cast<int64_t>(value.bits);
        }

        bool truth(Value value, TypeId type) const {
            return ModuleValue.type(type).kind == TypeKind::Floating ? value.floating != 0 : value.bits != 0;
        }

        Value operand(Operand operand, Frame& frame) {
            switch (operand.kind) {
            case OperandKind::Integer:
                return {operand.integer};
            case OperandKind::Floating:
                return {0, operand.floating};
            case OperandKind::Temporary:
                return frame.temporaries.at(operand.id);
            case OperandKind::Function:
                return {FunctionTag | operand.id};
            case OperandKind::Object:
                return {(Globals.count(operand.id) ? Globals.at(operand.id) : frame.addresses.at(operand.id)) +
                        operand.integer};
            case OperandKind::None:
                return {};
            default:
                throw std::runtime_error("test interpreter: invalid value operand");
            }
        }

        void store(uint64_t address, Value value, TypeId type) {
            const auto& info = ModuleValue.type(type);
            if (info.kind == TypeKind::Record) {
                if (value.record.size() != info.size) {
                    throw std::runtime_error("test interpreter: invalid record store");
                }
                for (uint64_t i = 0; i < info.size; ++i) {
                    Memory[address + i] = value.record[i];
                }
                return;
            }
            uint64_t bits = value.bits;
            if (info.kind == TypeKind::Floating) {
                bits = info.bits == 32 ? std::bit_cast<uint32_t>(static_cast<float>(value.floating))
                                       : std::bit_cast<uint64_t>(static_cast<double>(value.floating));
            }
            for (uint64_t i = 0; i < info.size; ++i) {
                Memory[address + i] = static_cast<unsigned char>(bits >> (i * 8));
            }
        }

        Value load(uint64_t address, TypeId type) {
            const auto& info = ModuleValue.type(type);
            if (info.kind == TypeKind::Record) {
                Value value;
                for (uint64_t i = 0; i < info.size; ++i) {
                    value.record.push_back(readByte(address + i));
                }
                return value;
            }
            uint64_t bits = 0;
            for (uint64_t i = 0; i < info.size; ++i) {
                bits |= uint64_t{readByte(address + i)} << (i * 8);
            }
            if (info.kind == TypeKind::Floating) {
                return {0, info.bits == 32 ? static_cast<long double>(std::bit_cast<float>(static_cast<uint32_t>(bits)))
                                           : static_cast<long double>(std::bit_cast<double>(bits))};
            }
            return {bits};
        }

        Value convert(Value value, TypeId from, TypeId to) {
            const auto& source = ModuleValue.type(from);
            const auto& target = ModuleValue.type(to);
            if (target.kind == TypeKind::Integer && target.bits == 1) {
                return {truth(value, from)};
            }
            if (target.kind == TypeKind::Floating) {
                long double number = source.kind == TypeKind::Floating ? value.floating
                                     : source.isSigned ? static_cast<long double>(signedValue(value, from))
                                                       : static_cast<long double>(value.bits);
                return {0, target.bits == 32 ? static_cast<long double>(static_cast<float>(number))
                                             : static_cast<long double>(static_cast<double>(number))};
            }
            uint64_t bits;
            if (source.kind == TypeKind::Floating) {
                const auto number = std::trunc(value.floating);
                const auto high = std::ldexp(1.L, target.isSigned ? target.bits - 1 : target.bits);
                const auto low = target.isSigned ? -high : 0.L;
                if (!std::isfinite(number) || number < low || number >= high) {
                    throw std::runtime_error("test interpreter: out-of-range floating conversion");
                }
                bits = target.isSigned ? static_cast<uint64_t>(static_cast<int64_t>(number))
                                       : static_cast<uint64_t>(number);
            } else {
                bits = source.kind == TypeKind::Integer && source.isSigned
                           ? static_cast<uint64_t>(signedValue(value, from))
                           : value.bits;
            }
            return {target.kind == TypeKind::Integer ? bits & mask(to) : bits};
        }

        Value arithmetic(const Instruction& instruction, Value left, Value right) {
            const auto& source = ModuleValue.type(instruction.left.type);
            const auto& target = ModuleValue.type(instruction.result.type);
            auto op = instruction.opcode;
            if (source.kind == TypeKind::Floating) {
                auto a = left.floating, b = right.floating;
                long double number;
                switch (op) {
                case Opcode::Add:
                    number = a + b;
                    break;
                case Opcode::Subtract:
                    number = a - b;
                    break;
                case Opcode::Multiply:
                    number = a * b;
                    break;
                case Opcode::Divide:
                    number = a / b;
                    break;
                case Opcode::Negate:
                    number = -a;
                    break;
                case Opcode::LogicalNot:
                    return {a == 0};
                case Opcode::Equal:
                    return {a == b};
                case Opcode::NotEqual:
                    return {a != b};
                case Opcode::Less:
                    return {a < b};
                case Opcode::Greater:
                    return {a > b};
                case Opcode::LessEqual:
                    return {a <= b};
                case Opcode::GreaterEqual:
                    return {a >= b};
                default:
                    throw std::runtime_error("test interpreter: invalid floating operation");
                }
                return {0, target.bits == 32 ? static_cast<long double>(static_cast<float>(number))
                                             : static_cast<long double>(static_cast<double>(number))};
            }
            auto a = left.bits, b = right.bits;
            auto sa =
                source.kind == TypeKind::Integer ? signedValue(left, instruction.left.type) : static_cast<int64_t>(a);
            auto sb = source.kind == TypeKind::Integer && instruction.right ? signedValue(right, instruction.right.type)
                                                                            : static_cast<int64_t>(b);
            uint64_t result;
            switch (op) {
            case Opcode::Add:
                result = a + b;
                break;
            case Opcode::Subtract:
                result = a - b;
                break;
            case Opcode::Multiply:
                result = a * b;
                break;
            case Opcode::Divide:
                if (!b) {
                    throw std::runtime_error("test division by zero");
                }
                if (source.isSigned && sa == std::numeric_limits<int64_t>::min() && sb == -1) {
                    result = a;
                } else {
                    result = source.isSigned ? static_cast<uint64_t>(sa / sb) : a / b;
                }
                break;
            case Opcode::Remainder:
                if (!b) {
                    throw std::runtime_error("test remainder by zero");
                }
                if (source.isSigned && sa == std::numeric_limits<int64_t>::min() && sb == -1) {
                    result = 0;
                } else {
                    result = source.isSigned ? static_cast<uint64_t>(sa % sb) : a % b;
                }
                break;
            case Opcode::ShiftLeft:
                result = a << b;
                break;
            case Opcode::ShiftRight:
                result = source.isSigned ? static_cast<uint64_t>(sa >> b) : a >> b;
                break;
            case Opcode::BitAnd:
                result = a & b;
                break;
            case Opcode::BitOr:
                result = a | b;
                break;
            case Opcode::BitXor:
                result = a ^ b;
                break;
            case Opcode::Equal:
                result = a == b;
                break;
            case Opcode::NotEqual:
                result = a != b;
                break;
            case Opcode::Less:
                result = source.isSigned ? sa < sb : a < b;
                break;
            case Opcode::Greater:
                result = source.isSigned ? sa > sb : a > b;
                break;
            case Opcode::LessEqual:
                result = source.isSigned ? sa <= sb : a <= b;
                break;
            case Opcode::GreaterEqual:
                result = source.isSigned ? sa >= sb : a >= b;
                break;
            case Opcode::Negate:
                result = uint64_t{0} - a;
                break;
            case Opcode::Complement:
                result = ~a;
                break;
            case Opcode::LogicalNot:
                result = !a;
                break;
            default:
                throw std::runtime_error("test interpreter: invalid arithmetic operation");
            }
            return {target.kind == TypeKind::Integer ? result & mask(instruction.result.type) : result};
        }

        Value invoke(uint32_t id, const std::vector<Value>& arguments) {
            auto* function = ModuleValue.functionById(id);
            if (!function || !function->defined) {
                throw std::runtime_error("test interpreter: external call");
            }
            Frame frame;
            for (const auto& object : function->objects) {
                frame.addresses[object.id] = allocate(object.type);
            }
            for (size_t i = 0; i < function->parameters.size(); ++i) {
                auto* object = ModuleValue.object(*function, function->parameters.at(i));
                store(frame.addresses.at(object->id), arguments[i], object->type);
            }
            auto current = function->blocks.front().id;
            for (;;) {
                const Block* block = nullptr;
                for (const auto& candidate : function->blocks) {
                    if (candidate.id == current) {
                        block = &candidate;
                    }
                }
                if (!block) {
                    throw std::runtime_error("test interpreter: missing block");
                }
                bool transferred = false;
                for (const auto& in : block->instructions) {
                    if (++Steps > 200000) {
                        throw std::runtime_error("test interpreter step limit");
                    }
                    auto a = in.left.kind == OperandKind::Block ? Value{} : operand(in.left, frame);
                    auto b = in.right.kind == OperandKind::Block ? Value{} : operand(in.right, frame);
                    Value result;
                    switch (in.opcode) {
                    case Opcode::Copy:
                        result = a;
                        break;
                    case Opcode::Cast:
                        result = convert(a, in.left.type, in.result.type);
                        break;
                    case Opcode::Load:
                        result = load(a.bits, in.result.type);
                        break;
                    case Opcode::Store:
                        store(a.bits, b, in.right.type);
                        continue;
                    case Opcode::AddressOffset:
                        result = {a.bits + b.bits};
                        break;
                    case Opcode::ZeroMemory: {
                        for (uint64_t i = 0; i < b.bits; ++i) {
                            Memory[a.bits + i] = 0;
                        }
                        continue;
                    }
                    case Opcode::Param:
                        frame.arguments.push_back(a);
                        continue;
                    case Opcode::Call: {
                        if (frame.arguments.size() != b.bits) {
                            throw std::runtime_error("test interpreter: call param count mismatch");
                        }
                        auto arguments = std::move(frame.arguments);
                        frame.arguments.clear();
                        result = invoke(static_cast<uint32_t>(a.bits & ~FunctionTag), arguments);
                        break;
                    }
                    case Opcode::RegisterDestructor:
                        ExitActions.push_back({static_cast<uint32_t>(a.bits & ~FunctionTag), b});
                        continue;
                    case Opcode::Jump:
                        current = in.left.id;
                        transferred = true;
                        break;
                    case Opcode::JumpIf:
                        if (!truth(a, in.left.type)) {
                            continue;
                        }
                        current = in.right.id;
                        transferred = true;
                        break;
                    case Opcode::Return:
                        return a;
                    default:
                        result = arithmetic(in, a, b);
                        break;
                    }
                    if (transferred) {
                        break;
                    }
                    if (in.result) {
                        frame.temporaries[in.result.id] = result;
                    }
                }
                if (!transferred) {
                    throw std::runtime_error("test interpreter: unterminated block");
                }
            }
        }

    public:
        explicit Interpreter(const Module& module) : ModuleValue(module) {
            Frame frame;
            for (const auto& object : module.globals) {
                Globals[object.id] = allocate(object.type, true);
            }
            for (const auto& object : module.globals) {
                for (size_t i = 0; i < object.bytes.size(); ++i) {
                    Memory[Globals.at(object.id) + i] = static_cast<unsigned char>(object.bytes[i]);
                }
                for (const auto& initial : object.initialValues) {
                    store(Globals.at(object.id) + initial.offset, operand(initial.value, frame), initial.value.type);
                }
            }
        }

        int64_t run(int n) {
            if (!Initialized) {
                if (ModuleValue.initializer) {
                    invoke(ModuleValue.initializer, {});
                }
                Initialized = true;
            }
            for (const auto& function : ModuleValue.functions) {
                if (function.name == "main") {
                    auto value = invoke(function.id, {{static_cast<uint32_t>(n)}});
                    while (!ExitActions.empty()) {
                        auto action = ExitActions.back();
                        ExitActions.pop_back();
                        invoke(action.first, {action.second});
                    }
                    const auto resultType = ModuleValue.type(function.signature).element;
                    const auto& result = ModuleValue.type(resultType);
                    if (result.kind == TypeKind::Void) {
                        return 0;
                    }
                    if (result.kind != TypeKind::Integer) {
                        throw std::runtime_error("test entry must return an integer or void");
                    }
                    return result.isSigned ? signedValue(value, resultType)
                                           : std::bit_cast<int64_t>(value.bits & mask(resultType));
                }
            }
            throw std::runtime_error("test interpreter: missing main");
        }

        int64_t globalInteger(const std::string& name) {
            for (const auto& object : ModuleValue.globals) {
                if (object.name == name) {
                    return ModuleValue.type(object.type).isSigned
                               ? signedValue(load(Globals.at(object.id), object.type), object.type)
                               : std::bit_cast<int64_t>(load(Globals.at(object.id), object.type).bits &
                                                        mask(object.type));
                }
            }
            throw std::runtime_error("test interpreter: missing observed global");
        }
    };
}
