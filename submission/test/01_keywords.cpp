
// ============================================================================
// Test File: 01_keywords.cpp
// Purpose: Test all 36 keyword TokenTypes specified in types/token/token.hh
// ============================================================================

// 1. K_NAMESPACE, K_USING
namespace TestNamespace {

    // 2. K_TYPEDEF
    typedef unsigned long long uint64_t;

    // 3. K_USING
    using Byte = unsigned char;

    // 4. K_ENUM
    enum Color {
        RED,
        GREEN,
        BLUE
    };

    // 5. K_CLASS, K_THIS
    class BaseClass {
    public:
        // 6. K_VIRTUAL (if needed, but not in enum), K_VOID, K_STATIC, K_CONST
        static const int default_capacity = 16;

        BaseClass() {}
        
        // 7. K_THIS
        BaseClass* get_self() {
            return this;
        }

        // 8. K_VIRTUAL/K_VOID
        void do_nothing() {}
    };

    // 9. K_STRUCT
    struct Point {
        // 10. K_INT, K_FLOAT, K_DOUBLE
        int x;
        float y;
        double z;

        // 11. K_SHORT, K_LONG, K_SIGNED, K_UNSIGNED, K_CHAR, K_BOOL
        short s;
        long l;
        signed int si;
        unsigned int ui;
        char c;
        bool flag;
    };

    // 12. K_UNION
    union ValueHolder {
        int int_val;
        double dbl_val;
        char char_val;
    };
}

// 13. K_VOID, K_INT
void test_control_flow(int x) {
    // 14. K_IF, K_ELSE
    if (x > 0) {
        x = x - 1;
    } else {
        x = x + 1;
    }

    // 15. K_WHILE, K_CONTINUE, K_BREAK
    while (x < 100) {
        x = x + 10;
        if (x == 50) {
            continue;
        }
        if (x == 80) {
            break;
        }
    }

    // 16. K_DO
    do {
        x = x + 1;
    } while (x < 90);

    // 17. K_FOR
    for (int i = 0; i < 10; i = i + 1) {
        x = x + i;
    }

    // 18. K_SWITCH, K_CASE
    switch (x) {
        case 1:
            break;
        case 2:
            break;
        default: // Note: default label if supported
            break;
    }

    // 19. K_GOTO
    goto end_label;

end_label:
    // 20. K_RETURN
    return;
}

// 21. K_NEW, K_DELETE, K_SIZEOF, K_TRUE, K_FALSE
int test_memory_and_operators() {
    // 22. K_BOOL, K_TRUE, K_FALSE
    bool is_valid = true;
    bool is_done = false;

    // 23. K_SIZEOF
    int size_int = sizeof(int);
    int size_struct = sizeof(TestNamespace::Point);

    // 24. K_NEW, K_DELETE
    int* ptr = new int;
    *ptr = 42;
    delete ptr;

    int* arr = new int[10];
    delete[] arr;

    // 25. K_RETURN
    return 0;
}
