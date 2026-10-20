#pragma once
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

namespace baka::ir {
    using TypeId = uint32_t;
    using BlockId = uint32_t;
    enum class TypeKind { Invalid, Void, Integer, Floating, Pointer, Array, Function, Record };

    // IR owns all strings and type descriptions. No semantic-arena or AST pointer
    // escapes lowering, so a Module remains usable after the frontend is destroyed.
    struct Type {
        TypeKind kind = TypeKind::Invalid;
        std::string name;
        unsigned bits = 0;
        bool isSigned = false;
        bool complete = false; // distinguishes opaque records/arrays from empty complete records
        TypeId element = 0;
        uint64_t count = 0, size = 0, alignment = 1;
        std::vector<TypeId> parameters;
        bool variadic = false;
        bool operator==(const Type&) const = default;
    };

    enum class OperandKind { None, Temporary, Object, Function, Integer, Floating, Block };

    struct Operand {
        OperandKind kind = OperandKind::None;
        TypeId type = 0;
        uint32_t id = 0;
        // Integer literals store low bits. Object/function relocations use this
        // same slot as a signed byte addend, represented in two's complement.
        uint64_t integer = 0;
        long double floating = 0;

        explicit operator bool() const {
            return kind != OperandKind::None;
        }

        bool operator==(const Operand&) const = default;
    };

    enum class Opcode {
        Copy,
        Load,
        Store,
        Cast,
        AddressOffset,
        Add,
        Subtract,
        Multiply,
        Divide,
        Remainder,
        ShiftLeft,
        ShiftRight,
        BitAnd,
        BitOr,
        BitXor,
        Equal,
        NotEqual,
        Less,
        Greater,
        LessEqual,
        GreaterEqual,
        Negate,
        Complement,
        LogicalNot,
        Argument,
        Call,
        RegisterDestructor,
        Jump,
        Branch,
        Return
    };

    struct SourceSpan {
        size_t line = 0, column = 0;
    };

    // A typed quadruple: (opcode, result, left, right). Branch uses result
    // for its false label; Jump uses left. Call arguments are preceding Argument
    // quadruples, emitted only after all argument expressions have been lowered.
    // Complete records are first-class values: Load captures a snapshot, Copy
    // copies a value, and Store writes the entire record. Argument/Call/Return
    // retain record types; no target ABI or hidden return pointer is imposed.
    // RegisterDestructor has no result, left=void(T*) callable, right=T*
    // receiver. The runtime pushes this callback after successful initialization
    // and invokes callbacks in reverse registration order after entry returns.
    struct Instruction {
        Opcode opcode = Opcode::Copy;
        Operand result, left, right;
        SourceSpan source;
        bool operator==(const Instruction&) const = default;
    };

    struct Block {
        BlockId id = 0;
        std::vector<Instruction> instructions;
        std::vector<BlockId> predecessors, successors;
    };

    struct Object {
        uint32_t id = 0;
        std::string name;
        TypeId type = 0;
        bool global = false, internal = false, external = false, readOnly = false;
        // Source-level const objects can require runtime initialization writes.
        // Backends must keep these writable during initialization, rather than
        // immediately placing them in a physically read-only data section.
        bool runtimeInitialized = false;

        // Sparse initializer: omitted bytes are zero. Each entry is a scalar
        // store at a byte offset, and may contain a symbolic relocation.
        struct InitialValue {
            uint64_t offset = 0;
            Operand value;
        };

        std::vector<InitialValue> initialValues;
        std::string bytes;
    };

    struct Function {
        uint32_t id = 0;
        std::string name;
        TypeId signature = 0;
        bool defined = false, internal = false;
        std::vector<uint32_t> parameters; // object IDs, including an explicit this
        std::vector<Object> objects;
        std::vector<TypeId> temporaries{0};
        std::vector<Block> blocks;
    };

    struct Module {
        std::vector<Type> types{Type{}};
        std::vector<Object> globals;
        std::vector<Function> functions;
        // Zero-fill storage, then invoke this void/no-argument function once
        // before program entry. Its instructions preserve declaration order.
        uint32_t initializer = 0;
        TypeId pointer(TypeId pointee);
        TypeId function(TypeId result, std::vector<TypeId> parameters, bool variadic);
        const Type& type(TypeId id) const;
        const Object* object(const Function& function, uint32_t id) const;
        const Function* functionById(uint32_t id) const;
    };

    struct Diagnostic {
        std::string message;
        SourceSpan source;
    };

    struct FlatInstruction {
        BlockId block;
        Instruction instruction;
    };

    bool IsTerminator(Opcode opcode);
    bool DefinesTemporary(const Instruction& instruction);
    std::vector<Operand> Uses(const Instruction& instruction);
    void BuildCFG(Function& function);
    std::vector<FlatInstruction> Linearize(const Function& function);
    std::vector<Diagnostic> Verify(const Module& module);
    void ConstantFold(Module& module);
    void CopyPropagate(Module& module);
    void EliminateDeadCode(Module& module);
    void Optimize(Module& module);
    void Print(const Module& module, std::ostream& output);
}
