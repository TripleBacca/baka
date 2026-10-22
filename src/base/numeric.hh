#pragma once

#include <cstdint>
#include <optional>

namespace baka {
    namespace numeric {
        // The language's fixed-width arithmetic contract. Integers wrap at their
        // declared width; floating operations round at each operation, not only when
        // stored. Neither frontend nor optimizer may substitute host-width arithmetic.
        struct Type {
            unsigned bits = 0;
            bool isSigned = false, floating = false;
        };

        struct Value {
            uint64_t bits = 0;
            long double floating = 0;
        };
        enum class Op {
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
            LogicalNot
        };

        uint64_t mask(Type type);
        int64_t signedValue(uint64_t bits, Type type);
        bool truth(Value value, Type type);
        std::optional<Value> convert(Value value, Type from, Type to);
        std::optional<Value> evaluate(Op op, Value left, Type leftType, Value right, Type rightType, Type resultType);
    }
}
