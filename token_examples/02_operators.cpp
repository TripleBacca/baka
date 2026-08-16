// ============================================================================
// Test File: 02_operators.cpp
// Purpose: Test all Operator TokenTypes specified in types/token/token.hh
// ============================================================================

struct Inner {
    int value;
};

struct Outer {
    Inner* inner_ptr;
    Inner inner_val;
};

// Ellipsis in variadic function declaration (OP_ELLIPSIS)
int variadic_sum(int count, ...) {
    return count;
}

namespace MathOps {
    int global_multiplier = 2;
}

int main() {
    int a = 10;
    int b = 20;
    int c = 0;

    // --- Unary Operators ---
    // OP_TILDE (~)
    int bitwise_not = ~a;

    // OP_UNARY_MINUS (-)
    int neg = -a;

    // OP_INC (++)
    ++a;
    a++;

    // OP_DEC (--)
    --b;
    b--;

    // OP_LOGICAL_NOT (!)
    bool flag = !false;

    // OP_PLUS (+)
    int positive = +a;

    // --- Binary Arithmetic & Bitwise Operators ---
    // OP_MUL (*)
    c = a * b;

    // OP_DIV (/)
    c = b / a;

    // OP_MOD (%)
    c = b % a;

    // OP_BIT_AND (&)
    c = a & b;

    // OP_BIT_OR (|)
    c = a | b;

    // OP_BIT_XOR (^)
    c = a ^ b;

    // OP_LSHIFT (<<)
    c = a << 2;

    // OP_RSHIFT (>>)
    c = b >> 1;

    // --- Logical & Relational Operators ---
    // OP_LOGICAL_AND (&&)
    bool cond_and = (a < b) && (b > 0);

    // OP_LOGICAL_OR (||)
    bool cond_or = (a == 10) || (b != 20);

    // OP_EQ (==)
    bool is_equal = (a == b);

    // OP_NEQ (!=)
    bool is_not_equal = (a != b);

    // OP_LT (<)
    bool is_less = (a < b);

    // OP_GT (>)
    bool is_greater = (a > b);

    // OP_LE (<=)
    bool is_less_or_equal = (a <= b);

    // OP_GE (>=)
    bool is_greater_or_equal = (a >= b);

    // --- Assignment Operators ---
    // OP_ASSIGN (=)
    c = a;

    // OP_ADD_ASSIGN (+=)
    c += 5;

    // OP_SUB_ASSIGN (-=)
    c -= 3;

    // OP_MUL_ASSIGN (*=)
    c *= 2;

    // OP_DIV_ASSIGN (/=)
    c /= 2;

    // OP_MOD_ASSIGN (%=)
    c %= 4;

    // OP_AND_ASSIGN (&=)
    c &= 0xFF;

    // OP_OR_ASSIGN (|=)
    c |= 0x01;

    // OP_XOR_ASSIGN (^=)
    c ^= 0x0F;

    // OP_LSHIFT_ASSIGN (<<=)
    c <<= 1;

    // OP_RSHIFT_ASSIGN (>>=)
    c >>= 1;

    // --- Other Operators & Punctuators ---
    // OP_DOUBLE_COLON (::)
    int scoped_val = MathOps::global_multiplier;

    // OP_QUESTION (?), OP_COLON (:)
    int ternary_res = (a > b) ? a : b;

    // OP_COMMA (,)
    int x = 1, y = 2, z = 3;

    // OP_DOT (.)
    Outer outer_obj;
    outer_obj.inner_val.value = 42;

    // OP_ARROW (->)
    outer_obj.inner_ptr = &outer_obj.inner_val;
    int accessed_val = outer_obj.inner_ptr->value;

    return 0;
}
