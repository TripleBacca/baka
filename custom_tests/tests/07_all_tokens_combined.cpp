// ============================================================================
// Test File: 07_all_tokens_combined.cpp
// Purpose: Comprehensive C++ test file covering EVERY TokenType defined in types/token/token.hh
// ============================================================================

// [K_NAMESPACE, IDENTIFIER, LPAREN_CURLY, RPAREN_CURLY]
namespace ComprehensiveTest {

    // [K_TYPEDEF, K_UNSIGNED, K_LONG, K_INT]
    typedef unsigned long int ulong_t;

    // [K_USING]
    using IntegerType = int;

    // [K_ENUM]
    enum Status {
        STATUS_OK,
        STATUS_ERR
    };

    // [K_UNION]
    union VariantData {
        int i;
        double d;
        char c;
    };

    // [K_STRUCT]
    struct Node {
        int val;
        Node* next;
    };

    // [K_CLASS, K_CONST, K_STATIC, K_VOID, K_THIS]
    class TokenDemonstrator {
    private:
        // [K_SIGNED, K_SHORT, K_FLOAT, K_DOUBLE, K_CHAR, K_BOOL]
        signed short short_val;
        float float_val;
        double double_val;
        char char_val;
        bool bool_val;

    public:
        // [K_STATIC, K_CONST]
        static const int BUFFER_LIMIT = 256;

        // Constructor [LPAREN_ROUND, RPAREN_ROUND, OP_COLON, OP_COMMA]
        TokenDemonstrator() : short_val(0), float_val(0.0f), double_val(3.1415), char_val('A'), bool_val(true) {}

        // [K_THIS, OP_ARROW]
        TokenDemonstrator* get_instance() {
            return this;
        }

        // Variadic function [OP_ELLIPSIS]
        void log_messages(int count, ...) {
            // [K_RETURN]
            return;
        }
    };
}

// [K_INT, IDENTIFIER, LPAREN_ROUND, RPAREN_ROUND, LPAREN_CURLY]
int main() {
    // --- 1. Keywords & Literals ---
    // [K_BOOL, LITERAL_BOOL, K_TRUE, K_FALSE]
    bool flag_t = true;
    bool flag_f = false;

    // [LITERAL_INTEGER, LITERAL_FP, LITERAL_CHARACTER, LITERAL_STRING]
    int decimal_num = 42;
    double pi_val = 3.14;
    char newline_char = '\n';
    const char* message = "Baka Tokenizer Test String";

    // [LITERAL_PTR, LITERAL_NULL, K_VOID]
    void* null_ptr = nullptr;

    // [K_NEW, K_DELETE, LPAREN_SQUARE, RPAREN_SQUARE]
    int* dynamic_array = new int[5];
    delete[] dynamic_array;

    // [K_SIZEOF]
    int type_size = sizeof(ComprehensiveTest::TokenDemonstrator);

    // --- 2. Operators ---
    int a = 15;
    int b = 4;
    int c = 0;

    // Unary: [OP_TILDE, OP_UNARY_MINUS, OP_PLUS, OP_INC, OP_DEC, OP_LOGICAL_NOT]
    int tilde_res = ~a;
    int neg_res = -a;
    int plus_res = +a;
    ++a;
    a++;
    --b;
    b--;
    bool not_res = !flag_t;

    // Binary Arithmetic & Bitwise: [OP_MUL, OP_DIV, OP_MOD, OP_BIT_AND, OP_BIT_OR, OP_BIT_XOR, OP_LSHIFT, OP_RSHIFT]
    c = (a * b) / ((a % b) + 1);
    c = (a & b) | (a ^ b);
    c = (a << 1) >> 1;

    // Relational & Logical: [OP_LT, OP_GT, OP_LE, OP_GE, OP_EQ, OP_NEQ, OP_LOGICAL_AND, OP_LOGICAL_OR]
    bool cond = ((a < b) || (a > b)) && ((a <= b) || (a >= b)) && (a == b) && (a != b);

    // Assignment: [OP_ASSIGN, OP_ADD_ASSIGN, OP_SUB_ASSIGN, OP_MUL_ASSIGN, OP_DIV_ASSIGN, OP_MOD_ASSIGN, OP_AND_ASSIGN, OP_OR_ASSIGN, OP_XOR_ASSIGN, OP_LSHIFT_ASSIGN, OP_RSHIFT_ASSIGN]
    c = 10;
    c += 1;
    c -= 1;
    c *= 2;
    c /= 2;
    c %= 5;
    c &= 0xFF;
    c |= 0x01;
    c ^= 0x02;
    c <<= 1;
    c >>= 1;

    // Member & Scope: [OP_DOUBLE_COLON, OP_DOT, OP_ARROW, OP_QUESTION, OP_COLON, OP_COMMA]
    ComprehensiveTest::Node node_instance;
    node_instance.val = 100;
    node_instance.next = nullptr;

    ComprehensiveTest::Node* node_ptr = &node_instance;
    int accessed_via_arrow = node_ptr->val;

    int ternary_val = (c > 0) ? c : 0;
    int comma_expr = (a = 1, b = 2, a + b);

    // --- 3. Control Flow Keywords ---
    // [K_IF, K_ELSE]
    if (cond) {
        c = 1;
    } else {
        c = 2;
    }

    // [K_WHILE, K_CONTINUE, K_BREAK]
    while (c < 10) {
        c += 1;
        if (c == 5) {
            continue;
        }
        if (c == 8) {
            break;
        }
    }

    // [K_DO]
    do {
        c -= 1;
    } while (c > 5);

    // [K_FOR]
    for (int i = 0; i < 3; i++) {
        c += i;
    }

    // [K_SWITCH, K_CASE]
    switch (c) {
        case 5:
            c = 10;
            break;
        case 10:
            c = 20;
            break;
    }

    // [K_GOTO]
    goto exit_point;

exit_point:
    // [SEMICOLON, K_RETURN]
    ;
    return 0;
}
// [EOF_TOKEN]
