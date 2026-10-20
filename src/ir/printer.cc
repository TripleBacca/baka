#include "ir.hh"
#include <algorithm>
#include <iomanip>
#include <limits>
#include <ostream>
#include <sstream>

namespace baka::ir {
    namespace {
        std::string TypeName(const Module& module, TypeId id) {
            const auto& type = module.type(id);
            switch (type.kind) {
            case TypeKind::Void:
                return "void";
            case TypeKind::Integer:
                return std::string(type.isSigned ? "i" : "u") + std::to_string(type.bits);
            case TypeKind::Floating:
                return "f" + std::to_string(type.bits);
            case TypeKind::Pointer:
                return "ptr<" + TypeName(module, type.element) + ">";
            case TypeKind::Array:
                return "[" + std::to_string(type.count) + " x " + TypeName(module, type.element) + "]";
            case TypeKind::Record:
                return "record#" + std::to_string(id) + "(" + type.name + ")";
            case TypeKind::Function: {
                std::string result = "fn(";
                for (size_t i = 0; i < type.parameters.size(); ++i) {
                    if (i) {
                        result += ", ";
                    }
                    result += TypeName(module, type.parameters[i]);
                }
                if (type.variadic) {
                    result += (type.parameters.empty() ? "..." : ", ...");
                }
                return result + ")->" + TypeName(module, type.element);
            }
            default:
                return "<invalid>";
            }
        }

        std::string Value(const Module& module, const Function& function, Operand operand) {
            std::ostringstream out;
            switch (operand.kind) {
            case OperandKind::None:
                return "_";
            case OperandKind::Temporary:
                out << "%t" << operand.id;
                break;
            case OperandKind::Object: {
                auto* object = module.object(function, operand.id);
                out << '&' << (object ? object->name : "?") << '.' << operand.id;
                if (operand.integer) {
                    out << (static_cast<int64_t>(operand.integer) < 0 ? "" : "+")
                        << static_cast<int64_t>(operand.integer);
                }
                break;
            }
            case OperandKind::Function: {
                auto* target = module.functionById(operand.id);
                out << '@' << (target ? target->name : "?") << '.' << operand.id;
                break;
            }
            case OperandKind::Block:
                return "bb" + std::to_string(operand.id);
            case OperandKind::Integer: {
                const auto& type = module.type(operand.type);
                uint64_t bits = operand.integer;
                if (type.kind == TypeKind::Integer && type.isSigned) {
                    if (type.bits < 64 && (bits & (uint64_t{1} << (type.bits - 1)))) {
                        bits |= ~((uint64_t{1} << type.bits) - 1);
                    }
                    out << static_cast<int64_t>(bits);
                } else {
                    out << bits;
                }
                break;
            }
            case OperandKind::Floating:
                out << std::setprecision(std::numeric_limits<long double>::max_digits10) << operand.floating;
                break;
            }
            return out.str() + ':' + TypeName(module, operand.type);
        }

        const char* Name(Opcode opcode) {
            switch (opcode) {
            case Opcode::Copy:
                return "copy";
            case Opcode::Load:
                return "load";
            case Opcode::Store:
                return "store";
            case Opcode::Cast:
                return "cast";
            case Opcode::AddressOffset:
                return "offset";
            case Opcode::Add:
                return "add";
            case Opcode::Subtract:
                return "sub";
            case Opcode::Multiply:
                return "mul";
            case Opcode::Divide:
                return "div";
            case Opcode::Remainder:
                return "rem";
            case Opcode::ShiftLeft:
                return "shl";
            case Opcode::ShiftRight:
                return "shr";
            case Opcode::BitAnd:
                return "and";
            case Opcode::BitOr:
                return "or";
            case Opcode::BitXor:
                return "xor";
            case Opcode::Equal:
                return "eq";
            case Opcode::NotEqual:
                return "ne";
            case Opcode::Less:
                return "lt";
            case Opcode::Greater:
                return "gt";
            case Opcode::LessEqual:
                return "le";
            case Opcode::GreaterEqual:
                return "ge";
            case Opcode::Negate:
                return "neg";
            case Opcode::Complement:
                return "complement";
            case Opcode::LogicalNot:
                return "not";
            case Opcode::Argument:
                return "arg";
            case Opcode::Call:
                return "call";
            case Opcode::Jump:
                return "jump";
            case Opcode::Branch:
                return "branch";
            case Opcode::Return:
                return "ret";
            }
            return "invalid";
        }
    }

    void Print(const Module& module, std::ostream& output) {
        output << "IR::TAC:\nmodule {\n";
        for (TypeId id = 1; id < module.types.size(); ++id) {
            const auto& type = module.type(id);
            if (type.kind == TypeKind::Record) {
                output << "  type " << TypeName(module, id);
                if (type.complete) {
                    output << " size " << type.size << " align " << type.alignment;
                } else {
                    output << " opaque";
                }
                output << '\n';
            }
        }
        Function globalContext;
        for (const auto& object : module.globals) {
            output << "  "
                   << (object.external   ? "extern "
                       : object.internal ? "internal "
                                         : "global ")
                   << object.name << '.' << object.id << ": " << TypeName(module, object.type);
            if (object.readOnly) {
                output << (object.runtimeInitialized ? " readonly-after-init" : " readonly");
            }
            if (!object.external) {
                output << " = zero";
            }
            if (!object.bytes.empty()) {
                output << " bytes[";
                for (unsigned char byte : object.bytes) {
                    output << std::hex << std::setw(2) << std::setfill('0') << unsigned(byte);
                }
                output << std::dec << std::setfill(' ') << ']';
            }
            for (const auto& initial : object.initialValues) {
                output << " {+" << initial.offset << " = " << Value(module, globalContext, initial.value) << '}';
            }
            output << '\n';
        }
        if (module.initializer) {
            const auto* initializer = module.functionById(module.initializer);
            output << "  initializer @" << initializer->name << '.' << initializer->id << '\n';
        }
        for (const auto& function : module.functions) {
            output << "  " << (function.defined ? "function " : "declare ") << '@' << function.name << '.'
                   << function.id << ": " << TypeName(module, function.signature);
            if (function.internal) {
                output << " internal";
            }
            if (!function.defined) {
                output << '\n';
                continue;
            }
            output << " {\n";
            for (auto id : function.parameters) {
                auto* object = module.object(function, id);
                output << "    parameter " << object->name << '.' << id << ": " << TypeName(module, object->type)
                       << '\n';
            }
            for (const auto& object : function.objects) {
                if (std::find(function.parameters.begin(), function.parameters.end(), object.id) !=
                    function.parameters.end()) {
                    continue;
                }
                output << "    local " << object.name << '.' << object.id << ": " << TypeName(module, object.type)
                       << '\n';
            }
            for (const auto& block : function.blocks) {
                output << "  bb" << block.id << ": ; predecessors [";
                for (size_t i = 0; i < block.predecessors.size(); ++i) {
                    output << (i ? ", " : "") << "bb" << block.predecessors[i];
                }
                output << "] successors [";
                for (size_t i = 0; i < block.successors.size(); ++i) {
                    output << (i ? ", " : "") << "bb" << block.successors[i];
                }
                output << "]\n";
                for (const auto& instruction : block.instructions) {
                    output << "    (" << Name(instruction.opcode) << ", " << Value(module, function, instruction.result)
                           << ", " << Value(module, function, instruction.left) << ", "
                           << Value(module, function, instruction.right) << ")\n";
                }
            }
            output << "  }\n";
        }
        output << "}\n";
    }
}
