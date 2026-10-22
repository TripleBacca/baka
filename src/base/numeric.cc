#include "numeric.hh"
#include <bit>
#include <cmath>
#include <limits>

namespace baka {
    namespace numeric {
        namespace {
            Value IntegerValue(uint64_t bits, Type type) {
                return Value{bits & mask(type)};
            }

            template <class Number> long double FloatingArithmetic(Op op, Number x, Number y) {
                switch (op) {
                case Op::Negate:
                    return -x;
                case Op::Add:
                    return x + y;
                case Op::Subtract:
                    return x - y;
                case Op::Multiply:
                    return x * y;
                case Op::Divide:
                    if (y != 0) {
                        return x / y;
                    }
                    break;
                default:
                    break;
                }
                return std::numeric_limits<long double>::quiet_NaN();
            }
        }

        uint64_t mask(Type type) {
            return type.bits == 64 ? ~uint64_t{0} : (uint64_t{1} << type.bits) - 1;
        }

        int64_t signedValue(uint64_t bits, Type type) {
            bits &= mask(type);
            if (type.bits < 64 && (bits & (uint64_t{1} << (type.bits - 1)))) {
                bits |= ~mask(type);
            }
            return std::bit_cast<int64_t>(bits);
        }

        bool truth(Value value, Type type) {
            return type.floating ? value.floating != 0 : (value.bits & mask(type)) != 0;
        }

        std::optional<Value> convert(Value value, Type from, Type to) {
            if (!from.bits || !to.bits) {
                return {};
            }
            if (!to.floating && to.bits == 1) {
                return Value{truth(value, from)};
            }
            if (to.floating) {
                long double number;
                if (from.floating) {
                    number = value.floating;
                } else if (from.isSigned) {
                    number = static_cast<long double>(signedValue(value.bits, from));
                } else {
                    number = static_cast<long double>(value.bits & mask(from));
                }
                number = to.bits == 32 ? static_cast<long double>(static_cast<float>(number))
                                       : static_cast<long double>(static_cast<double>(number));
                if (!std::isfinite(number)) {
                    return {};
                }
                return Value{0, number};
            }
            uint64_t bits;
            if (from.floating) {
                auto number = std::trunc(value.floating);
                auto maximum = std::ldexp(1.L, to.isSigned ? to.bits - 1 : to.bits);
                auto minimum = to.isSigned ? -maximum : 0.L;
                if (!std::isfinite(number) || number < minimum || number >= maximum) {
                    return {};
                }
                bits =
                    to.isSigned ? static_cast<uint64_t>(static_cast<int64_t>(number)) : static_cast<uint64_t>(number);
            } else {
                bits = from.isSigned ? static_cast<uint64_t>(signedValue(value.bits, from)) : value.bits & mask(from);
            }
            return Value{bits & mask(to)};
        }

        std::optional<Value> evaluate(Op op, Value a, Type left, Value b, Type right, Type result) {
            if (!left.bits || !result.bits) {
                return {};
            }
            if (op == Op::LogicalNot) {
                return IntegerValue(!truth(a, left), result);
            }
            if (left.floating) {
                auto x = a.floating, y = b.floating;
                switch (op) {
                case Op::Equal:
                    return IntegerValue(x == y, result);
                case Op::NotEqual:
                    return IntegerValue(x != y, result);
                case Op::Less:
                    return IntegerValue(x < y, result);
                case Op::Greater:
                    return IntegerValue(x > y, result);
                case Op::LessEqual:
                    return IntegerValue(x <= y, result);
                case Op::GreaterEqual:
                    return IntegerValue(x >= y, result);
                default:
                    break;
                }
                auto number = left.bits == 32 ? FloatingArithmetic(op, static_cast<float>(x), static_cast<float>(y))
                                              : FloatingArithmetic(op, static_cast<double>(x), static_cast<double>(y));
                if (!std::isfinite(number)) {
                    return {};
                }
                return convert({0, number}, left, result);
            }
            auto x = a.bits & mask(left), y = right.bits ? b.bits & mask(right) : 0;
            auto sx = signedValue(x, left), sy = right.bits ? signedValue(y, right) : 0;
            switch (op) {
            case Op::Negate:
                return IntegerValue(uint64_t{0} - x, result);
            case Op::Complement:
                return IntegerValue(~x, result);
            case Op::Add:
                return IntegerValue(x + y, result);
            case Op::Subtract:
                return IntegerValue(x - y, result);
            case Op::Multiply:
                return IntegerValue(x * y, result);
            case Op::Divide:
            case Op::Remainder:
                if (!y) {
                    return {};
                }
                if (left.isSigned) {
                    if (sx == std::numeric_limits<int64_t>::min() && sy == -1) {
                        return IntegerValue(op == Op::Divide ? x : 0, result);
                    }
                    return IntegerValue(static_cast<uint64_t>(op == Op::Divide ? sx / sy : sx % sy), result);
                }
                return IntegerValue(op == Op::Divide ? x / y : x % y, result);
            case Op::ShiftLeft:
                if (y < left.bits) {
                    return IntegerValue(x << y, result);
                }
                return {};
            case Op::ShiftRight:
                if (y >= left.bits) {
                    return {};
                }
                if (!left.isSigned || sx >= 0 || !y) {
                    return IntegerValue(x >> y, result);
                }
                return IntegerValue((x >> y) | (~uint64_t{0} << (left.bits - y)), result);
            case Op::BitAnd:
                return IntegerValue(x & y, result);
            case Op::BitOr:
                return IntegerValue(x | y, result);
            case Op::BitXor:
                return IntegerValue(x ^ y, result);
            case Op::Equal:
                return IntegerValue(x == y, result);
            case Op::NotEqual:
                return IntegerValue(x != y, result);
            case Op::Less:
                return IntegerValue(left.isSigned ? sx < sy : x < y, result);
            case Op::Greater:
                return IntegerValue(left.isSigned ? sx > sy : x > y, result);
            case Op::LessEqual:
                return IntegerValue(left.isSigned ? sx <= sy : x <= y, result);
            case Op::GreaterEqual:
                return IntegerValue(left.isSigned ? sx >= sy : x >= y, result);
            default:
                return {};
            }
        }
    }
}
